/* --COPYRIGHT--,BSD_EX
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *******************************************************************************
 *
 *                       MSP430 CODE EXAMPLE DISCLAIMER
 *
 * MSP430 code examples are self-contained low-level programs that typically
 * demonstrate a single peripheral function or device feature in a highly
 * concise manner. For this the code may rely on the device's power-on default
 * register values and settings such as the clock configuration and care must
 * be taken when combining code from several examples to avoid potential side
 * effects. Also see www.ti.com/grace for a GUI- and www.ti.com/msp430ware
 * for an API functional library-approach to peripheral configuration.
 *
 * --/COPYRIGHT--*/
//******************************************************************************
//  MSP430G2xx3 Demo - Software Toggle P1.0
//
//  Description; Toggle P1.0 by xor'ing P1.0 inside of a software loop.
//  ACLK = n/a, MCLK = SMCLK = default DCO
//
//                MSP430G2xx3
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//            |             P1.0|-->LED
//
//  D. Dang
//  Texas Instruments, Inc
//  December 2010
//   Built with CCS Version 4.2.0 and IAR Embedded Workbench Version: 5.10
//******************************************************************************

#include <msp430.h>

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer
  P1DIR |= BIT0 + BIT6;                     // Set P1.0 and P1.6 high (output direction/enable LEDs)

  P1OUT = 0x00;								// Clear all bits in P1OUT
  P1OUT |= BIT3;

  P1REN |= BIT3;							// enable pullup resistor for BIT3 (push button S2)
//  P1IN = BIT3;								// input is high (1)
  P1IE |= BIT3;								// enable P1.3 interrupt
  P1IFG &=~ BIT3;							// P1.3 clear IFG
  __bis_SR_register(LPM4_bits + GIE);       // Enter LPM0 w/ interrupt




  // Part 2
  /*for (;;)
  {
    volatile unsigned int i;

    P1OUT ^= BIT0 + BIT6;                   // Toggle P1.0 using exclusive-OR

    i = 50000;                              // Delay
    do (i--);
    while (i != 0);
  }*/
} // end main

// Timer A0 interrupt service routine
#pragma vector=PORT1_VECTOR
__interrupt void Port_1 (void)
{
	volatile unsigned int i;
	while(!(P1IN & BIT3))
	{
		P1OUT ^= (BIT0 + BIT6); // P1.0 = toggle
		i = 50000;
		do(i--);
		while(i != 0);
	}
	P1OUT &=~ (BIT0 + BIT6);
	P1IFG &= ~BIT3; // P1.3 IFG cleared
	P1IES ^= BIT3;	// toggle the interrupt edge
}
