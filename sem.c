#include <stdint.h>
#include "hookapi.h"
#define SVAR(x) &x, sizeof(x)
#define DONE(x) accept(SBUF(x), __LINE__);
#define NOPE(x) rollback(SBUF(x), __LINE__);
int64_t hook(uint32_t r)
{
    _g(1,1);

    uint64_t is_invoke = (otxn_type() == ttINVOKE);
    uint64_t is_payment = (otxn_type() == ttPAYMENT);

    if (!is_invoke && !is_payment)
        DONE("Sem: passing non-invoke, non-payment txn.");

    // first check if this is a signal from the signal account

    uint8_t hookacc[20], otxnacc[20], sigacc[20];
    hook_account(SBUF(hookacc));
    otxn_field(SBUF(otxnacc), sfAccount);
    
    uint8_t semaphore = 0;
    state(SVAR(semaphore), "SEM", 3);

    uint64_t is_setup = (state(SBUF(sigacc), "SEMACC", 6) == 20);

    if (BUFFER_EQUAL_20(hookacc, otxnacc))
    {    
        // self txn
        if (!is_invoke)
            DONE("Sem: passing outgoing non-invoke txn.");

        // use invoke to set signalling account

        if (otxn_param(SBUF(sigacc), "SEMACC", 6) != 20)
            NOPE("Sem: self ttINVOKE did not contain SEMACC parameter.");
    
        // set the signal account
        if (state_set(SBUF(sigacc), "SEMACC", 6) != 20)
            NOPE("Sem: could not set SEMACC hook state.");

        DONE("Sem: new SEMACC set.");
    }

    if (!is_setup)
        NOPE("Sem: send a self ttINVOKE with SEMACC to setup hook.");

    if (is_invoke)
    {
        // check if the invoke is from the signalling account
        if (BUFFER_EQUAL_20(otxnacc, sigacc))
        {
            // flip the semaphore
            semaphore++;
            semaphore %= 2;
            if (state_set(SVAR(semaphore), "SEM", 3) < 0)
                NOPE("Sem: could not set new semaphore value.");

            if (semaphore)
                DONE("Sem: incoming payments are now BLOCKED.");

            DONE("Sem: incoming payments are not ALLOWED.");
        }
        
        DONE("Sem: passing ttINVOKE from non-signal account."); 
    }

    if (!is_payment)
        DONE("Sem: passing non-payment transaction.");

    if (semaphore)
        NOPE("Sem: payments are temporarily disabled. Please try again in 30 minutes.");

    DONE("Sem: passing incoming payment.");
}

