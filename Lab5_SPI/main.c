/*************************************************************
 * File:	main.c
 * Author:	Ross Moon
 * Date:	04/20/2016
 * Description:	Lab 5 - SPI communication to LCD.
 *	LCD is a Newhaven NHD-C0216CZ-NSW-BBW-3V3.
 *	LCD displays input from a 4x4 keypad.
 *
 *	MSP430G2xx3 SPI Hardware Ports
 *                 -----------------
 *             /|\|              XIN|-
 *              | |                 |
 *              --|RST          XOUT|-
 *                |                 |
 *                |             P1.2|-> Data Out (UCA0SIMO)
 *                |                 |
 *          LED <-|P1.0         P1.1|<- Data In (UCA0SOMI)
 *                |                 |
 *  Slave reset <-|P1.7         P1.4|-> Serial Clock Out (UCA0CLK)
 ************************************************************/

// Library includes
#include <msp430.h>

// Constant Variables
#define BTN_DLY 50000	// timer delay for keypad input rate
#define TX_DLY 	20		// Transmit delay
#define CS	 	BIT6	// Chip Select:	0 - I'm talking to you | 1 - Not talking
#define RS		BIT7	// Register Select: 0 - Command | 1 - Data
#define CRSR_INIT 0x80	// LCD display address 0
#define CRSR 0x5F		// cursor represented by '_'
#define CLR 0x20		// Empty space value
// LCD predefined initialization instructions
#define WAKE_UP 0x30
#define FUNC_SET 0x39
#define INTR_OSC_FREQ 0x14
#define PWR_CNTR 0x56
#define FOL_CONTROL 0x6D
#define CONTRAST 0x70
#define DISP_ON 0x0C
#define CLEAR 	0x01


// Class Variables
volatile unsigned int row, col, num;
volatile unsigned int cursorPos = CRSR_INIT;
volatile unsigned int cursor = CRSR;
volatile unsigned int buttonPressed = 0;
volatile unsigned int muxRow[] = {0x00, 0x08, 0x20, 0x28};	// Binary: 00, 01, 10, 11
volatile unsigned int cols[] = {0x0D, 0x25, 0x29, 0x2C};
volatile unsigned char commands[4][4] = {{0x31, 0x32, 0x33, 0x41},	// 1, 2, 3, A
		{0x34, 0x35, 0x36, 0x42},	// 4, 5, 6, B
		{0x37, 0x38, 0x39, 0x43},	// 7, 8, 9, C	ASCII values
		{0x00, 0x30, 0x23, 0x44}};	// *, 0, #, D   * acts a a Backspace keystroke

// Function Prototypes
void initKeypad();
void initTimer();
void initSPI();
void initLED();
void keypad();
void write(int command, int data);
void writeCmd(int command);
void writeData(int data);
void updateCursor(int input);

int main(void){

	WDTCTL = WDTPW + WDTHOLD;                 // Stop watchdog timer

	// Initialize board
	initTimer();
	initKeypad();
	initSPI();
	initLED();

	while(1){
		__delay_cycles(50);
		keypad();				// Search for keypad input
		__bis_SR_register(GIE); // enable interrupts
	} // end while(1)
} // end main()

/* updateCursor()
 *  Update the position of the cursor on the LCD.
 *  Jump between row 1 and 2 without writing off the LCD.
 * @param input - current position of cursor *
 */
void updateCursor(int input){
	if(input == 0x00){
		// move cursor left
		if(cursorPos > 0x80 && cursorPos != 0xA8){
			write(cursorPos, CLR);
			cursorPos--;
		}
		else if(cursorPos == 0xA8){
			// Move cursor to start of 2nd row
			cursorPos = 0x8F;	// jump up a row
		}
	}
	else{
		// move cursor right
		if(cursorPos < 0xB7 && cursorPos != 0x8F){
			cursorPos++;
		}
		else if(cursorPos == 0x8F){
			// Move cursor to end of 1st row
			cursorPos = 0xA8;	// jump down a row
		}
	}
} // end updateCursor()


/* writeOutput()
 *	 Load output to hardware buffer.
 *	@param output - value to be send to LCD
 *	@param type - 1 output is data, 0 output is instruction
 */
void writeOutput(int output, int type){
	while (!(IFG2 & UCA0TXIFG));
	P1OUT &=~ CS;					// CS low, signal slave to listen
	if(type == 0){
		P1OUT &=~ RS;				// Select LCD instruction register
	}
	else if(type == 1){
		P1OUT |= RS;				// Select LCD data register
	}
	else{
		// Error, Invalid type
		// Abort, do not send data
		P1OUT |= CS;				// stop talking to slave
		return;
	}
	UCA0TXBUF = output;				// Load data into buffer
	while (!(IFG2 & UCA0TXIFG));
	P1OUT |= CS;					// CS high, we are done talking
} // end writeDate()


/* write()
 *	Programmatically determines if output is instruction
 *	or data, then appropriately sends info to LCD
 * @param command - Contains LCD instruction, else -1
 * @param data - Contains LCD data, else -1
 */
void write(int command, int data){
	if(command > 0 && data > 0){
		// writing data to LED
		writeOutput(command, 0);
		__delay_cycles(TX_DLY);			// Delay to give slave
		writeOutput(data, 1);
		__delay_cycles(TX_DLY);			// a chance to keep up.
	}
	else if(command > 0 && data < 0){
		// send instruction command to LED
		writeOutput(command, 0);
		__delay_cycles(TX_DLY);
	}
	else{
		// Error invalid input
		return;
	}
} // end write()


/* initLED()
 * 	Send LED startup and initialization commands.
 * 	Defined by NHD-C0216CZ-NSW-BBW-3V3 datasheet.
 */
void initLED(){
	write(WAKE_UP, -1);			// Time to wake up LCD
	__delay_cycles(TX_DLY);		// Give slave a chance to wake up
	write(WAKE_UP, -1);
	write(WAKE_UP, -1);
	__delay_cycles(TX_DLY);		// Make sure slave had time to wake up
	write(FUNC_SET, -1);		// Start predefined initialization sequence
	write(INTR_OSC_FREQ, -1);
	write(PWR_CNTR, -1);
	write(FOL_CONTROL, -1);
	write(CONTRAST, -1);
	write(DISP_ON, -1);
	write(CLEAR, -1);
} // end initLED()


/* initSPI()
 *  Initialize MSP430 hardware SPI
 */
void initSPI(){
	P1OUT = 0x00;
	P1DIR |= RS + CS;						// RS and CS are output
	P1SEL = BIT1 + BIT2 + BIT4;
	P1SEL2 = BIT1 + BIT2 + BIT4;
	UCA0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;  // 3-pin, 8-bit SPI master
	UCA0CTL1 |= UCSSEL_2;                     // SMCLK
	UCA0BR0 |= 0x02;                          // /2
	UCA0BR1 = 0;                              //
	UCA0MCTL = 0;                             // No modulation
	UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**

	P1OUT &= ~BIT5;                           // Now with SPI signals initialized,
	P1OUT |= BIT5;                            // reset slave

	__delay_cycles(75);                 		// Wait for slave to initialize
} // end initSPI()

// Timer A0 interrupt service routine
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void){

	// Hacky solution to delay propagation of
	// keypad inputs to LED
	if(buttonPressed){
		__delay_cycles(500000);
		buttonPressed = 0;
	}
} // end Timer_A0 interrupt


/* initTimer()
 * 	Initialize MSP430 timer interrupts
 */
void initTimer(){
	CCTL0 = CCIE;						// CCR0 interrupt enabled
	TACTL = TASSEL_2 + MC_1 + ID_3;		// SMCLK/8, upmode
	CCR0 = BTN_DLY;						// Set interrupt frequency
} // end initTimer()


/* initKeypad()
 * 	Ports 1.3 and 1.5 strobe deMUX input.
 * 	Ports 2.0, 2.2, 2.3, 2.5 listen to deMUX output
 */
void initKeypad(){
	P1DIR |= (BIT3 + BIT5);					// Set as output ports
	P1OUT &=~ (BIT3 + BIT5);				// Strobe ports are low by default

	P2REN |= (BIT0 + BIT2 + BIT3 + BIT5);	// Enable resistors
	P2DIR &=~ (BIT0 + BIT2 + BIT3 + BIT5);	// Enable input for keypad
} // end initKeypad()


/* keypad()
 * Strobe the deMux in search for
 * a pressed button on the keypad
 */
void keypad(){
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
					// write button input to LCD
					write(cursorPos, commands[j][i]);
					// update cursor
					updateCursor(commands[j][i]);
					write(cursorPos, cursor);
				}
			}// end for(col)
			P1OUT &=~ muxRow[i];	// reset P1OUT for next test
		} // end for(row)
	}// end if(!button)
} // end keypad()
