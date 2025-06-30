/**
 * Semaphore Hook
 *
 * Description:
 * Allows a signalling account to block or allow incoming payments to the account.
 *
 * Setup: Self ttINVOKE with the following parameters:
 *
 *        SEMAMT (8 byte little endian uint) is the threshold below which the account 
 *        will not accept incoming payments. If omitted then it is deemed to be 1.
 *      
 *        SEMACC (20 byte accid) is the signalling account allowed to send ttINVOKE signals.
 *
 * Usage: The signalling account will periodically send a ttINVOKE containing parameter:
 *
 *        BAL (8 byte little endian uint)
 *
 *        If BAL is equal to or above the SEMAMT then the Hook passes allows incoming ttPAYMENT
 *        transactions. If BAL is less than SEMAMT then the Hook disallows incoming ttPAYMENT
 *        transactions.
 *
 *        The allow/disallow state continues as toggled until a new txn changes it.
 *
 * Comment:
 *        Remit is not monitored and can be used to refill the Hook accounts XAH as needed.
 *
 * Author: Richard Holland
 * Date: 2025/6/30
 */

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

    uint64_t semamt = 1;    
    uint64_t is_setup = (state(SBUF(sigacc), "SEMACC", 6) == 20);
       
    int64_t ret = state(SVAR(semamt), "SEMAMT", 6);
    if (ret < 0 || ret == 8)
    {
        // the amount can either be unset or set correctly
    }
    else
    {
        is_setup = 0;
    }

    if (BUFFER_EQUAL_20(hookacc, otxnacc))
    {    
        // self txn
        if (!is_invoke)
            DONE("Sem: passing outgoing non-invoke txn.");

        // use invoke to set signalling account

        uint64_t accset = 0, amtset = 0;
        

        int64_t ret = otxn_param(SBUF(sigacc), "SEMACC", 6);
        if (ret == 20)
        {
            // set the signal account
            if (state_set(SBUF(sigacc), "SEMACC", 6) != 20)
                NOPE("Sem: could not set SEMACC hook state.");

            accset = 1;
        }
        else if (ret > 0)
            NOPE("Sem: specified SEMACC but wrong size, expecting 20 bytes.");
        
        uint64_t semamt = 0;
        ret = otxn_param(SVAR(semamt), "SEMAMT", 6);
        if (ret == 8)
        {
            if (state_set(SVAR(semamt), "SEMAMT", 6) != 8)
                NOPE("Sem: could not set SEMAMT hook state.");
        
            amtset = 1;    
        }
        else if (ret > 0)
            NOPE("Sem: specified SEMAMT but wrong size, expecting 8 bytes LE int");

        if (accset && amtset)
        {
            DONE("Sem: both SEMACC and SEMAMT have been updated.");
        }
        else if (accset)
        {
            DONE("Sem: SEMACC has been updated.");
        }
        else if (amtset)
        {
            DONE("Sem: SEMAMT has been updated.");
        }
        else
        {
            NOPE("Sem: specify SEMACC (20 bytes) and/or SEMAMT (8 bytes le uint) to update values.");
        }
    }

    if (!is_setup)
        NOPE("Sem: send a self ttINVOKE with SEMACC (and optionally SEMAMT) to setup hook.");

    if (is_invoke)
    {
        // check if the invoke is from the signalling account
        uint8_t dummy;
        uint64_t bal;
        uint64_t has_bal = (otxn_param(SVAR(bal), "BAL", 3) == 8);

        if (BUFFER_EQUAL_20(otxnacc, sigacc))
        {
            if (!has_bal)
                NOPE("Sem: missing correctly formatted BAL param on signalling account invoke (8 byte int LE).");

                
            
            semaphore = (bal < semamt);
            
            if (state_set(SVAR(semaphore), "SEM", 3) < 0)
                NOPE("Sem: could not set new semaphore value.");

            if (semaphore)
                DONE("Sem: incoming payments are now BLOCKED.");

            DONE("Sem: incoming payments are now ALLOWED.");
        }
        
        DONE("Sem: passing ttINVOKE from non-signal account."); 
    }

    if (!is_payment)
        DONE("Sem: passing non-payment transaction.");

    if (semaphore)
        NOPE("Sem: payments are temporarily disabled. Please try again in 30 minutes.");

    DONE("Sem: passing incoming payment.");
}

