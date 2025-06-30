# Semaphore Hook

A Xahau Hook that enables conditional payment acceptance based on signals from an authorized account. This hook acts as a payment gateway that can be toggled on/off based on balance thresholds.

## Overview

The Semaphore Hook allows an account to delegate payment acceptance control to a designated signalling account. The signalling account can enable or disable incoming payments by sending balance updates via `ttINVOKE` transactions.

**Author:** Richard Holland  
**Date:** June 30, 2025

## How It Works

1. **Setup Phase**: Configure the hook with a threshold amount and authorized signalling account
2. **Signal Phase**: The signalling account sends balance updates
3. **Operation Phase**: Incoming payments are automatically allowed or blocked based on the last signal

## Installation & Setup

### Initial Configuration

Send a self `ttINVOKE` transaction with the following parameters:

- **`SEMACC`** (required): 20-byte account ID of the authorized signalling account
- **`SEMAMT`** (optional): 8-byte little-endian unsigned integer representing the threshold amount (defaults to 1 if omitted)

Example setup parameters:
```
SEMACC: [20 bytes representing the signalling account]
SEMAMT: [8 bytes LE uint, e.g., 0x00E1F50500000000 for 100,000,000]
```

## Usage

### For the Signalling Account

Send a `ttINVOKE` transaction to the hook account with:

- **`BAL`**: 8-byte little-endian unsigned integer representing the current balance

The hook's behavior:
- If `BAL >= SEMAMT`: Incoming payments are **ALLOWED**
- If `BAL < SEMAMT`: Incoming payments are **BLOCKED**

### For the Hook Account

- **Incoming Payments**: Automatically allowed or blocked based on the last signal
- **Outgoing Payments**: Always allowed (hook only affects incoming `ttPAYMENT` transactions)
- **Other Transaction Types**: Pass through unaffected
- **Remit Transactions**: Can be used to refill the account's XAH reserves without triggering the hook

## State Variables

The hook maintains three state variables:

1. **`SEMACC`**: The authorized signalling account (20 bytes)
2. **`SEMAMT`**: The threshold amount (8 bytes, default: 1)
3. **`SEM`**: Current semaphore state (1 byte, 0=allow, 1=block)

## Error Messages

- `"Sem: send a self ttINVOKE with SEMACC (and optionally SEMAMT) to setup hook."` - Hook not configured
- `"Sem: payments are temporarily disabled. Please try again in 30 minutes."` - Payments currently blocked
- `"Sem: missing correctly formatted BAL param..."` - Invalid signal format
- `"Sem: specified SEMACC but wrong size, expecting 20 bytes."` - Invalid account format
- `"Sem: specified SEMAMT but wrong size, expecting 8 bytes LE int"` - Invalid amount format

## Use Cases

- **Merchant Payment Control**: Enable/disable payment acceptance based on inventory levels
- **Automated Treasury Management**: Block incoming payments when balance thresholds are reached
- **Conditional Service Accounts**: Toggle service availability based on external conditions
- **Risk Management**: Temporarily disable payments during security events

## Technical Details

- **Hook API Version**: Uses standard Xahau Hook API
- **Transaction Types Monitored**: `ttINVOKE` and `ttPAYMENT`
- **Performance**: Minimal state reads/writes for efficient operation
- **Security**: Only the designated signalling account can update the semaphore state

## Notes

- The semaphore state persists until explicitly changed by a new signal
- Setup parameters can be updated by sending new self-invoke transactions
- The hook does not monitor or restrict Remit transactions, allowing for account maintenance
