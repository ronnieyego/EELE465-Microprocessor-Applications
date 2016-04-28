/*************************************************************
 * File:	main.c
 * Author:	Ross Moon
 * Date:	03/20/2016
 * Description:	Lab 2 - Input is 4x4 keypad.  Output is binary
 * 	code send to red LED on MSP430 launchPad.  Green LED is
 * 	clock.
 ************************************************************/

// Library includes
#include <msp430.h>

#define CLK_SPD 37250	// ~4 Hz

// Function prototypes
void initTimer();
void initLEDs();
void initKeypad();

// Class variables
unsigned int haveInput = 0;	// 0 - false; 1 - true
volatile int row, col, num;
volatile unsigned int displayVal;
volatile unsigned int displayCount = 0;
volatile unsigned int muxRow[] = {0x00, 0x08, 0x10, 0x18};	// Binary: 00, 01, 10, 11
volatile unsigned int cols[] = {0x0D, 0x25, 0x29, 0x2C};
volatile unsigned char dispKey[4][4] = {{0x01, 0x02, 0x03, 0x0A},	// 1, 2, 3, A
										{0x04, 0x05, 0x06, 0x0B},	// 4, 5, 6, B
										{0x07, 0x08, 0x09, 0x0C},	// 7, 8, 9, C
										{0x0E, 0x00, 0x0F, 0x0D}};	// *, 0, #, D




void main(void) {
	// initialize hardware
	initLEDs();
	initKeypad();
	initTimer();

	while(1){
		if(!haveInput){
			unsigned int i, j;
			for (i = 0; i < 4; i ++){
				// find row of pressed button
				P1OUT |= muxRow[i];		// set P1OUT to current test
                    for (j = 0; j < 4; j ++) {
					// find column of pressed button
					if ((P2IN & 0xFF) == cols[j]){
						// yay! we found the button
						displayVal = dispKey[j][i];
						haveInput = 1;
					}
				}// end for(col)
				P1OUT &=~ muxRow[i];	// reset P1OUT for next test
			} // end for(row)
		} // end if(!haveInput)
	} // end while(1)

} // end main()

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void){

	// print keypad value
	if(haveInput){
		displayCount++;
		P1OUT ^= BIT6;				// toggle CLK LED
		if(P1OUT & BIT6){
			if(BIT3 & displayVal){
				P1OUT |= BIT0;		// set display high
			}

			displayVal <<= 1;		// shift display value to the left 1
		    displayVal &=~ 0xF0;	// clear bits greater than BIT3

		}
		else{
			P1OUT &=~ BIT0;			// set display low
		}
		if(displayCount >= 7){
			// display complete
			haveInput = 0;			// reset input flag
			displayCount = 0;		// reset count
		}
	}
	else
	{
		P1OUT &=~ (BIT0 + BIT6);	// LEDs are off by default
	}
} // end Timer_A0 interrupt


/* initTimer()
 * 	Initialize MSP430 timer interrupts
 */
void initTimer(){
	WDTCTL = WDTPW + WDTHOLD;           // Stop watchdog timer
	CCTL0 = CCIE;						// CCR0 interrupt enabled
	TACTL = TASSEL_2 + MC_1 + ID_3;		// SMCLK/8, upmode
	CCR0 = CLK_SPD;						// Set interrupt frequency
	__bis_SR_register(GIE);				// Enter LPM0 w/ interrupt
} // end initTimer()


/* initLEDs()
 *  Enable LaunchPad LEDs: LED1 and LED2.
 *  LED1 = BIT0 = green LED
 *  LED2 = BIT6 = red LED
 */
void initLEDs(){
	P1DIR |= (BIT0 + BIT6);		// Set P1.0 and P1.6 high (output direction/enable LEDs)
	P1OUT &=~ (BIT0 + BIT6);	// LEDs are off by default
} // end initLEDs()


/* initKeypad()
 * 	Ports 1.3 and 1.4 strobe deMUX input.
 * 	Ports 2.0, 2.2, 2.3, 2.5 listen to deMUX output
 */
void initKeypad(){
	P1DIR |= (BIT3 + BIT4);					// Set as output ports
	P1OUT &=~ (BIT3 + BIT4);				// Strobe ports are low by default

	P2REN |= (BIT0 + BIT2 + BIT3 + BIT5);	// Enable resistors
	P2DIR &=~ (BIT0 + BIT2 + BIT3 + BIT5);	// Enable input for keypad
} // end initKeypad()

