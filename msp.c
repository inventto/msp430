/*
 * |----------------------------------------------------------------|
 * |The base code can be found here:                                |
 * |http://homepages.ius.edu/RWISMAN/C335/HTML/msp430SerialComm.htm |
 * |This is my first MSP430 project.                                |
 * |																|
 * |----------------------------------------------------------------|
 *
 */

#include "msp430g2553.h"
// Hardware-related definitions
#define UART_TXD 0x02                                  // TXD on P1.1 (Timer0_A.OUT0)
#define UART_RXD 0x04                                  // RXD on P1.2 (Timer0_A.CCI1A)

#define UART_TBIT_DIV_2 (1000000 / (9600 * 2))         // Conditions for 9600 Baud SW UART, SMCLK = 1MHz
#define UART_TBIT (1000000 / 9600)
// Globals for full-duplex UART communication
unsigned int txData;                            // UART internal variable for TX
unsigned char rxBuffer;                               // Received UART character

void TimerA_UART_tx(unsigned char byte);               // Function prototypes
void TimerA_UART_print(char *string);

void main(void) {

	WDTCTL = WDTPW + WDTHOLD;                            // Stop watchdog timer
	DCOCTL = 0x00;                                       // Set DCOCLK to 1MHz
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;
	P1OUT = 0x00;                                        // Initialize all GPIO

	P1SEL = UART_TXD + UART_RXD;              // Timer function for TXD/RXD pins
	P1DIR = 0xFF & ~UART_RXD;                  // Set all pins but RXD to output

	// Configures Timer_A for full-duplex UART operation
	TA0CCTL0 = OUT;                                // Set TXD Idle as Mark = '1'
	TA0CCTL1 = SCS + CM1 + CAP + CCIE;           // Sync, Neg Edge, Capture, Int
	TA0CTL = TASSEL_2 + MC_2;                 // SMCLK, start in continuous mode

	_BIS_SR(GIE);                                       // Enable CPU interrupts

	TimerA_UART_print("G2553 TimerA UART\r\n");          // Send test message
	TimerA_UART_print("READY.\r\n");

	while (1) {                                   // Wait for incoming character

		_BIS_SR(LPM0_bits);                              // Enter low poser mode

		TimerA_UART_tx(rxBuffer);                  // Transmit the received data
	}
}

void TimerA_UART_tx(unsigned char byte) { // Outputs one byte using the Timer_A UART

	while (TACCTL0 & CCIE)
		;                              // Ensure last char got TX'd

	TA0CCR0 = TAR;                                // Current state of TA counter

	TA0CCR0 += UART_TBIT;                         // One bit time till first bit

	TA0CCTL0 = OUTMOD0 + CCIE;                           // Set TXD on EQU0, Int

	txData = byte;                                       // Load global variable

	txData |= 0x100;                              // Add mark stop bit to TXData

	txData <<= 1;                                        // Add space start bit
}

void TimerA_UART_print(char *string) { // Prints a string using the Timer_A UART

	while (*string)
		TimerA_UART_tx(*string++);
}

#pragma vector = TIMER0_A0_VECTOR                      // Timer_A UART - Transmit Interrupt Handler

__interrupt void Timer_A0_ISR(void) {

	static unsigned char txBitCnt = 10;

	TA0CCR0 += UART_TBIT;                                // Add Offset to CCRx

	if (txBitCnt == 0) {                                 // All bits TXed?

		TA0CCTL0 &= ~CCIE;                   // All bits TXed, disable interrupt

		txBitCnt = 10;                                    // Re-load bit counter
	} else {
		if (txData & 0x01)
			TA0CCTL0 &= ~OUTMOD2;                            // TX Mark '1'
		else
			TA0CCTL0 |= OUTMOD2;                             // TX Space '0'
	}
	txData >>= 1;                                        // Shift right 1 bit
	txBitCnt--;
}

#pragma vector = TIMER0_A1_VECTOR                      // Timer_A UART - Receive Interrupt Handler

__interrupt void Timer_A1_ISR(void) {

	static unsigned char rxBitCnt = 8;

	static unsigned char rxData = 0;

	switch (__even_in_range(TA0IV, TA0IV_TAIFG)) {   // Use calculated branching

	case TA0IV_TACCR1:                                 // TACCR1 CCIFG - UART RX

		TA0CCR1 += UART_TBIT;                         // Add Offset to CCRx

		if (TA0CCTL1 & CAP) {                   // Capture mode = start bit edge

			TA0CCTL1 &= ~CAP;                  // Switch capture to compare mode

			TA0CCR1 += UART_TBIT_DIV_2;            // Point CCRx to middle of D0
		} else {
			rxData >>= 1;

			if (TA0CCTL1 & SCCI)             // Get bit waiting in receive latch
				rxData |= 0x80;
			rxBitCnt--;

			if (rxBitCnt == 0) {                        // All bits RXed?

				rxBuffer = rxData;                   // Store in global variable

				rxBitCnt = 8;                             // Re-load bit counter

				TA0CCTL1 |= CAP;               // Switch compare to capture mode

			}
		}
		break;
	}
}
