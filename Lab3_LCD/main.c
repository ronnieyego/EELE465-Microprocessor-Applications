/*************************************************************
 * File:	main.c
 * Author:	Ross Moon
 * Date:	04/11/2016
 * Description:	Lab 3.1 - PWM added to control brightness of LCD.
 * 	Input is 4x4 keypad.  Output is binary signal send to red
 * 	LED on MSP430 launchPad.  Green LED is clock.  Keypad also
 * 	controls brightness of LCD display on key press of 0-9.
 ************************************************************/

// Library includes
#include <msp430.h>

// Class constant variables
#define CLK_SPD 37250	// ~4 Hz
#define PWM_VAL 22222

// Function prototypes
void initTimer();
void initLEDs();
void initKeypad();
void initPWM();
void modDuty(unsigned int index);

// Class variables
unsigned int haveInput = 0;	// 0 - false; 1 - true
volatile int row, col, num;
volatile unsigned int displayVal;
volatile unsigned int displayCount = 0;
volatile unsigned int dutyCycle[] = {PWM_VAL * 0, PWM_VAL * .1, PWM_VAL * .2, PWM_VAL * .3,
									PWM_VAL * .4, PWM_VAL * .5, PWM_VAL * .6, PWM_VAL * .7,
									PWM_VAL * .8, PWM_VAL * .9};
volatile unsigned int muxRow[] = {0x00, 0x08, 0x10, 0x18};	// Binary: 00, 01, 10, 11
volatile unsigned int cols[] = {0x0D, 0x25, 0x29, 0x2C};
volatile unsigned char dispKey[4][4] = {{0x01, 0x02, 0x03, 0x0A},	// 1, 2, 3, A -> 0001, 0010, 0011, 1010
										{0x04, 0x05, 0x06, 0x0B},	// 4, 5, 6, B -> 0100, 0101, 0110, 1011
										{0x07, 0x08, 0x09, 0x0C},	// 7, 8, 9, C -> 0111, 1000, 1001, 1100
										{0x0E, 0x00, 0x0F, 0x0D}};	// *, 0, #, D -> 1110, 0000, 1111, 1101





void main(void) {

	// initialize hardware
	WDTCTL = WDTPW + WDTHOLD;           // Stop watchdog timer
   	initLEDs();
	initKeypad();
	initTimer();
	initPWM();
	__bis_SR_register(GIE);				// Enable interrupts

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
						modDuty(displayVal);
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

/* modDuty()
 * 	Sets the percent duty cycle of the PWM on TA1
 * 	@param: index - index of the percent duty cycle
 * 		Declared in dutyCycle array.
 */
void modDuty(unsigned int index){
	if(index < 0x0A){
		// index is 0-9
		TA1CCR1 = dutyCycle[index];		// 0% - 90% duty cycle
	}
}

/* initTimer()
 * 	Initialize MSP430 timer interrupts
 */
void initTimer(){

	TA0CCTL0 = CCIE;						// CCR0 interrupt enabled
	TA0CTL = TASSEL_2 + MC_1 + ID_3;		// SMCLK/8, upmode
	TA0CCR0 = CLK_SPD;						// Set interrupt frequency

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


/* initPWM()
 *	Initialize timers A1 for hardware PWM
 *	PWM signal output to 2.1
 */
void initPWM(){

	P2DIR |= BIT1;				// Set 2.1 to output
	P2SEL |= BIT1;				// Enable PWM on 2.1

	TA1CCR0 = PWM_VAL;         	// PWM period
	TA1CCR1 = PWM_VAL / 2;      // PWM duty cycle, 50% initially
	TA1CCTL1 = OUTMOD_7;        // CCR1 reset/set
	TA1CTL = TASSEL_2 + MC_1;   // SMCLK, up mode

}
