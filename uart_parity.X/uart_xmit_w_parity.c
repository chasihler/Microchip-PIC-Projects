/* 
 * File:   uart_xmit_w_parity.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on August 31, 2014, 9:41 AM
 *
 * Target Device:
 * 16F1509 on Tautic 20 pin dev board
 *
 * Project: RF-UART test on 434MHz module
 *
 *
 * Version:
 * 1.0
 *
 */
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 4000000 //4Mhz FRC internal osc
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#endif

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//config bits
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=ON, CP=OFF, BOREN=ON, CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF

#define _XTAL_FREQ 4000000 //defined for delay

char    cdata;

/*
 * 
 */
void init_io(void) {
    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA3 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output
    ANSELA = 0x00; // all port A pins are digital I/O

    TRISBbits.TRISB4 = 0; // RB4 = nc
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 = nc

    ANSELB = 0x00; // all port B pins are digital I/O
    TRISCbits.TRISC0 = 0; // output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input
    ANSELC = 0x00; // all port B pins are digital I/O
}

unsigned char calculateparity(unsigned char scancode)
{
	unsigned char parity = 0;

	while(scancode > 0)          // if it is 0 there are no more 1's to count
	{
		if(scancode & 0x01)    //see if LSB is 1
		{
			parity++;                // why yes it is
		}
		scancode = scancode >> 1; //shift to next bit
	}

	return (parity & 0x01);  // only need the low bit to determine odd / even }
}

void uart_xmit(unsigned int mydata_byte) {  //send a character to the UART
    /*
*    22.1.1.6 Transmitting 9-Bit Characters
*   The EUSART supports 9-bit character transmissions.
*   When the TX9 bit of the TXSTA register is set, the
*   EUSART will shift nine bits out for each character transmitted.
*   The TX9D bit of the TXSTA register is the ninth,
*   and Most Significant, data bit. When transmitting 9-bit
*   data, the TX9D data bit must be written before writing
*   the eight Least Significant bits into the TXREG. All nine
*   bits of data will be transferred to the TSR shift register
*   immediately after the TXREG is written.
    */
    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXSTAbits.TX9D = calculateparity(mydata_byte);
    TXREG = mydata_byte;       // transmit data

}

void uart_write(const char *txt)            //sent a multiple characters
{
    while(*txt != 0) uart_xmit(*txt++);     //this send a string to the TX buffer
                                            //one character at a time
}


void init_uart(void)
{
    //TX 1200 8E1
    //RX 1200 8N1


    TXSTAbits.BRGH=1;       // select low speed Baud Rate
    TXSTAbits.TX9=1;        // select 9 bit mode
    TXSTAbits.TXEN = 1;     // enable transmit
    BAUDCONbits.BRG16 = 0;
    BAUDCONbits.SCKP = 1;   //inverted TX


    RCSTAbits.SPEN=1;       // serial port is enabled
    RCSTAbits.RX9=0;        // select 8 data bits
    RCSTAbits.CREN=1;       // receive enabled

    SPBRG=103;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH=0;

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear

    __delay_ms(50);        // give time for voltage levels on board to settle
    uart_write("RESET");         // transmit some data for testing

}


int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    init_io();
    init_uart();


    while (1) {


    __delay_ms(149);    //add a little dead time


    uart_write("__ ac0gd - test. ");    //transmit my Station ID for legality in the US

    }
    return (EXIT_SUCCESS);
}
