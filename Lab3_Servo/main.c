/*************************************************************
 * File:	main.c
 * Author:	Ross Moon
 * Date:	04/11/2016
 * Description:	Lab 3.2 - Controls 3 servos via PWM.
 * 	TA1.1 and TA1.2 each control continious rotation servos (A and B).
 * 	TA0 controls a potition servo.
 * 	Keypad Map:
 * 		1 - rotate position servo left while pressed
 * 		2- 	rotate both continuous servos left
 * 		3 - rotate position servo right while pressed
 * 		4 - rotate servo A right and servo B left
 * 		5 - stop servos A and B
 * 		6 - rotate servo A left and servo B right
 * 		8 - rotate both continuous servos right
 * 		0 - center position servo
 *
 ************************************************************/

// Library includes
#include <msp430.h>

// Class constant variables
#define PWM_PERIOD 	20000
#define STOP		1500
#define FORWARD		2000
#define BACKWARD	1000


// Function prototypes
void initLEDs();
void initKeypad();
void initPWM_TA0();
void initPWM_TA1();
void moveServos(unsigned int cmd);

// Class variables

volatile unsigned int cmdVal;
volatile int row, col, num;
volatile unsigned int muxRow[] = {0x00, 0x08, 0x10, 0x18};	// Binary: 00, 01, 10, 11
volatile unsigned int cols[] = {0x0D, 0x25, 0x29, 0x2C};
volatile unsigned char commands[4][4] = {{0x01, 0x02, 0x03, 0x0A},	// 1, 2, 3, A -> 0001, 0010, 0011, 1010
										{0x04, 0x05, 0x06, 0x0B},	// 4, 5, 6, B -> 0100, 0101, 0110, 1011
										{0x07, 0x08, 0x09, 0x0C},	// 7, 8, 9, C -> 0111, 1000, 1001, 1100
										{0x0E, 0x00, 0x0F, 0x0D}};	// *, 0, #, D -> 1110, 0000, 1111, 1101





void main(void) {

	WDTCTL = WDTPW + WDTHOLD;           // Stop watchdog timer

	// Set both clocks to 1MHZ
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;

	// Initialize ports & hardware
	initLEDs();
	initKeypad();
	initPWM_TA0();
	initPWM_TA1();

	__bis_SR_register(GIE);				// Enable interrupts

	while(1){
		unsigned int i, j;
		for (i = 0; i < 4; i ++){
			// find row of pressed button
			P1OUT |= muxRow[i];		// set P1OUT to current test
			for (j = 0; j < 4; j ++) {
				// find column of pressed button
				if ((P2IN & 0xFF) == cols[j]){
					// yay! we found the button
					cmdVal = commands[j][i];
					moveServos(cmdVal);
				}
			}// end for(col)
			P1OUT &=~ muxRow[i];	// reset P1OUT for next test
		} // end for(row)
	} // end while(1)
} // end main()



void moveServos(unsigned int cmd){
	switch(cmd){
	case 0x00:
		TA0CCR1 = STOP;
		break;
	case 0x01:
		if(TA0CCR1 < FORWARD)
			TA0CCR1++;
		break;
	case 0x02:		// forward
		TA1CCR1 = FORWARD;
 		TA1CCR2 = FORWARD;
		break;
	case 0x03:
		if(TA0CCR1 > BACKWARD)
			TA0CCR1--;
		break;
	case 0x04:		// turn left
		TA1CCR1 = BACKWARD;
		TA1CCR2 = FORWARD;
		break;
	case 0x05:		// stop
		TA1CCR1 = STOP;
		TA1CCR2 = STOP;
		break;
	case 0x06:		// turn right
		TA1CCR1 = FORWARD;
		TA1CCR2 = BACKWARD;
		break;
	case 0x08:		// reverse
		TA1CCR1 = BACKWARD;
		TA1CCR2 = BACKWARD;
		break;
	} // end switch
} // end moveServos()


/* initLEDs()
 *  Enable LaunchPad LEDs: LED1 and LED2.
 *  LED1 = BIT0 = green LED
 *  LED2 = BIT6 = red LED
 */
void initLEDs(){
	P1DIR |= (BIT0);		// Set P1.0 high (output direction/enable LEDs)
	P1OUT &=~ BIT0;
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


/* initPWM_TA0()
 * 	Initialize timer A0 for hardware PWM
 * 	PWM signal output to 1.6
 */
void initPWM_TA0(){
	P1DIR |= BIT6;				// Set 1.6 to output
	P1SEL |= BIT6;				// Enable PWM on 1.6

	TA0CCR0 = PWM_PERIOD;       // PWM period
	TA0CCR1 = STOP;      		// PWM duty cycle,
	TA0CCTL1 = OUTMOD_7;        // CCR1 reset/set
	TA0CTL = TASSEL_2 + MC_1;   // SMCLK, up mode

} // end initPWM_TA1_TA0()

/* initPWM_TA1()
 *	Initialize timers A1 for hardware PWM
 *	PWM signal output to 2.1 and 2.4
 */
void initPWM_TA1(){
	P2DIR |= BIT1 + BIT4;		// Set 2.1 and 2.4 to output
	P2SEL |= BIT1 + BIT4;		// Enable PWM on 2.1 and 2.4

	TA1CCR0 = PWM_PERIOD;       // PWM period
	TA1CCR1 = STOP;   			// PWM duty cycle for TA1.1
	TA1CCR2 = STOP;				// PWM duty cycle for TA1.2
	TA1CCTL1 = OUTMOD_7;        // reset/set for TA1.1
	TA1CCTL2 = OUTMOD_7;		// reset/set for TA1.2
	TA1CTL = TASSEL_2 + MC_1;   // SMCLK, up mode

}
