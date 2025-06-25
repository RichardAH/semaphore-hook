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
    uint64_t semamt = 0;
    uint64_t has_amt = (state(SVAR(semamt), "SEMAMT", 6) == 8);
    uint64_t is_setup = (state(SBUF(sigacc), "SEMACC", 6) == 20);

    if (BUFFER_EQUAL_20(hookacc, otxnacc))
    {    
        // self txn
        if (!is_invoke)
            DONE("Sem: passing outgoing non-invoke txn.");

        // use invoke to set signalling account

        uint64_t params_set = 0;

        int64_t ret = otxn_param(SBUF(sigacc), "SEMACC", 6);
        if (ret == 20)
        {
            // set the signal account
            if (state_set(SBUF(sigacc), "SEMACC", 6) != 20)
                NOPE("Sem: could not set SEMACC hook state.");

            params_set++;
        }
        else if (ret > 0)
            NOPE("Sem: specified SEMACC but wrong size, expecting 20 bytes.");
        
        uint64_t sigamt = 0;
        ret = otxn_param(SVAR(sigamt), "SEMAMT", 6);
        if (ret == 8)
        {
            if (state_set(SVAR(sigamt), "SEMAMT", 6) != 8)
                NOPE("Sem: could not set SEMAMT hook state.");
            
            params_set++;
        }
        else if (ret > 0)
            NOPE("Sem: specified SEMAMT but wrong size, expecting 8 bytes LE int");

        if (params_set > 0)
        {
            DONE("Sem: param(s) set.");
        }
        else
        {
            NOPE("Sem: specify either or both of SEMACC (20 bytes) and SEMAMT (8 bytes le uint).");
        }
    }

    if (!is_setup)
        NOPE("Sem: send a self ttINVOKE with SEMACC to setup hook.");

    if (is_invoke)
    {
        // check if the invoke is from the signalling account
        if (BUFFER_EQUAL_20(otxnacc, sigacc))
        {
            uint8_t dummy;
            uint64_t bal;
            uint64_t has_bal = (otxn_param(SVAR(bal), "BAL", 3) == 8);

            if (otxn_param(SVAR(dummy), "BLOCK", 5) > 0)
                semaphore = 1;
            else if (otxn_param(SVAR(dummy), "ALLOW", 5) > 0)
                semaphore = 0;
            else if (has_bal)
            {
                if (!has_amt)
                    semaphore = 0;
                else
                {
                    // flip sem based on balance threshold
                    semaphore = !(bal < semamt);
                } 
            }
            else
            {
                // blank invoke flips the semaphore
                semaphore++;
                semaphore %= 2;
            }
            

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

