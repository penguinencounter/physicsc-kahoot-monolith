// Copyright 2025 Optiboot contributors, PenguinEncounter
// SPDX-License-Identifier: GPL-2.0-only

#include "resetdbg.h"
#include <Arduino.h>
#include <avr/wdt.h>

// This code is based on the code in the Optiboot examples repository:
// https://github.com/Optiboot/optiboot/blob/master/optiboot/examples/demo_reset/demo_reset.ino
// Retrieved May 26, 2025.
// Optiboot is licensed with a variation of GPLv2, so this file specifically is also GPLv2.
// Have a look at resetdbg_LICENSE for the license text.

uint8_t resetFlag __attribute__((section(".noinit")));

void resetFlagsInit(void) __attribute__ ((naked))
__attribute__ ((used))
__attribute__ ((section (".init0")));
void resetFlagsInit()
{
    /*
       save the reset flags passed from the bootloader
       This is a "simple" matter of storing (STS) r2 in the special variable
       that we have created.  We use assembler to access the right variable.
    */
    __asm__ __volatile__ ("sts %0, r2\n" : "=m" (resetFlag) :);
}

void printReset(uint8_t resetFlags)
{
    Serial.print(resetFlags, HEX);
    /*
       check for the usual bits.  Note that the symnbols defined in wdt.h are
       bit numbers, so they have to be shifted before comparison.
    */
    if (resetFlags & (1 << WDRF))
    {
        Serial.print(F(" Watchdog"));
        resetFlags &= ~(1 << WDRF);
    }
    if (resetFlags & (1 << BORF))
    {
        Serial.print(F(" Brownout"));
        resetFlags &= ~(1 << BORF);
    }
    if (resetFlags & (1 << EXTRF))
    {
        Serial.print(F(" External"));
        resetFlags &= ~(1 << EXTRF);
    }
    if (resetFlags & (1 << PORF))
    {
        Serial.print(F(" PowerOn"));
        resetFlags &= ~(1 << PORF);
    }
    if (resetFlags != 0x00)
    {
        // It should never enter here
        Serial.print(" Unknown");
    }
    Serial.println("");
}

void printReset() {
    printReset(resetFlag);
}