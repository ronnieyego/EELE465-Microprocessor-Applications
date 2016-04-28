/*************************************************************
 * File:	main.c
 * Author:	Ross Moon
 * Date:	04/11/2016
 * Description:	Lab 4 - I2C communication protocol to a
 * 	SAA1064 IC LED Driver.  A 4x4 keypad determines the display
 * 	values of the LED.  Each keypress shifts the LED values
 * 	one to the right, with the current input represented in the
 * 	leftmost LED.
 ************************************************************/

// Library includes
#include <msp430.h>

// Class Constant Variables
#define I2C_DELAY 100
#define SDA BIT7
#define SCL BIT6
#define LED_ADDR 0x76	// defined address for the SAA1064 IC LED Driver


// Class Variables
volatile unsigned int row, col, num;
volatile unsigned int buttonPressed = 0;
volatile unsigned int muxRow[] = {0x00, 0x08, 0x10, 0x18};	// Binary: 00, 01, 10, 11
volatile unsigned int cols[] = {0x0D, 0x25, 0x29, 0x2C};
volatile unsigned char commands[4][4] = {{0x30, 0x6D, 0x79, 0x77},	// 1, 2, 3, A
										{0x33, 0x5B, 0x5F, 0x1F},	// 4, 5, 6, B
										{0x70, 0x7F, 0x7B, 0x4E},	// 7, 8, 9, C
										{0x47, 0x7E, 0x4F, 0x3D}};	// *, 0, #, D


// Function Prototypes
void initTimer();
void i2c_init();
void initKeypad();
int i2c_bb_tx(char *buf, int numBytes);
int ix2_bb_rx(char addr, char *buf, int numBytes);


void main(void) {
	WDTCTL = WDTPW + WDTHOLD;           // Stop watchdog timer

	P1DIR |= (BIT0);					// Set P1.0 high (output direction/enable LEDs)
	P1OUT &=~ (BIT0);					// LED0 used for error notification on failed transmit

	// Initialize I2C and keypad
	char i2c_buf[16];
	i2c_init();
	initKeypad();
	initTimer();

	__delay_cycles(I2C_DELAY * 100);

	while(1){
		__bis_SR_register(GIE);				// Enable interrupt
		unsigned int i, j;
		if(!buttonPressed){
			for (i = 0; i < 4; i ++){
				// find row of pressed button
				P1OUT |= muxRow[i];		// set P1OUT to current test
				for (j = 0; j < 4; j ++) {
					// find column of pressed button
					if ((P2IN & 0xFF) == cols[j]){
						// yay! we found the button
						buttonPressed = 1;
						i2c_buf[0] = LED_ADDR;
						i2c_buf[1] = 0x00;
						i2c_buf[2] = 0x37;
						i2c_buf[6] = i2c_buf[5];
						i2c_buf[5] = i2c_buf[4];
						i2c_buf[4] = i2c_buf[3];
						i2c_buf[3] = commands[j][i];

						if(!i2c_bb_tx(i2c_buf, 7)){
							// error in transmit.
							P1OUT ^= BIT0;
							__delay_cycles(I2C_DELAY * 100);
						}
					}
				}// end for(col)
				P1OUT &=~ muxRow[i];	// reset P1OUT for next test
			} // end for(row)
		}

	} // end while(1)
} // end main()


// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void){

	// Hacky solution to delay propagation of
	// keypad inputs to LED
	if(buttonPressed){
		buttonPressed = 0;
	}
} // end Timer_A0 interrupt


/* initTimer()
 * 	Initialize MSP430 timer interrupts
 */
void initTimer(){
	CCTL0 = CCIE;						// CCR0 interrupt enabled
	TACTL = TASSEL_2 + MC_1 + ID_3;		// SMCLK/8, upmode
	CCR0 = 55000;						// Set interrupt frequency

} // end initTimer()


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


/* i2c_init()
 * 	Initialize hardware for I2C protocol.
 */
void i2c_init(){
	P1DIR |= SCL + SDA;
	P1OUT |= SCL + SDA;
}

/* i2c_bb_tx() - transmit function
 * @param: buf char[] - buffer array
 * @param: numBytes int - number of bytes
 * @return: success conditions - 1 success, 0 fail
 */
int i2c_bb_tx(char *buf, int numBytes){
	//char addr = buf[0];
	int k, l;
	char outVal;

	P1OUT &=~ SDA;
	__delay_cycles(I2C_DELAY);
	P1OUT &=~ SCL;
	__delay_cycles(I2C_DELAY);

	for(k = 0; k < numBytes; k++){
		outVal = buf[k];
		for(l = 0; l < 8; l++){
			if(outVal & SDA){
				P1OUT |= SDA;
			}
			else{
				P1OUT &=~ SDA;
			}
			P1OUT |= SCL;
			__delay_cycles(I2C_DELAY);
			P1OUT &=~ SCL;
			__delay_cycles(I2C_DELAY);
			outVal <<= 1;
		} // end for(l)
		P1DIR &=~ SDA;
		// strobe clock and check
		P1OUT |= SCL;
		if(P1IN & SDA){
			P1DIR &=~ SCL;
			P1DIR |= SDA;
			P1OUT |= SDA + SCL;
			return 0;
		}
		__delay_cycles(I2C_DELAY);
		P1OUT &=~ SCL;
		__delay_cycles(I2C_DELAY);
		P1DIR |= SDA;
	}// end for(k)
	
	P1OUT |= SCL;
	__delay_cycles(I2C_DELAY);
	P1OUT |= SDA;
	return 1;
} // end i2c_bb_tx()


int i2c_bb_rx(char addr, char *buf, int numBytes){
	int i, j, k, inVal;
	for(i = 0; i < 8; i++){
		if(addr & SDA){
			P1OUT |= SDA;
		}
		else{
			P1OUT &=~ SDA;
		}
	} // end for(i)
	for(j = 0; j < 8; j++){
		inVal = 0;
		for(k= 0; k < 8; k++){
			inVal <<= 1;
			// strobe clock
			P1OUT |= SCL;
			__delay_cycles(I2C_DELAY);
			P1OUT &=~ SCL;
			__delay_cycles(I2C_DELAY);
			if(P1IN & SDA){
				inVal++;
			}
			buf[j] = inVal;
			P1DIR |= SDA;
			if(k == (numBytes-1)){
				P1OUT |= SDA;	// master no ack
			}
			else{
				P1OUT &=~ SDA;	// ACK
			}
			// strobe clock
			P1OUT |= SCL;
			__delay_cycles(I2C_DELAY);
			P1OUT &=~ SCL;
			__delay_cycles(I2C_DELAY);

			P1DIR &=~ SDA;		// input
		} // end for(k)
		P1DIR |= SDA;			// output
	} // end for(j)
	return 1;
} // end i2c_bb_rx()





