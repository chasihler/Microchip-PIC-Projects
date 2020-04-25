/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on Sept 1 2014, 12:12 PM
 *
 * Target Device:
 * 18F26K22 TAUTIC dev board
 *
 * Project:
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
#pragma config FOSC=INTIO67, WDTEN=OFF, PWRTEN=OFF, CP0=OFF, CP1=OFF, BOREN=ON
#pragma config STVREN=ON, LVP=OFF, HFOFST=OFF, IESO=OFF, FCMEN=OFF

//WRT=OFF, FOSC=INTOSC, MCLRE=ON

#define _XTAL_FREQ 4000000 //defined for delay

    int     an8_value, an9_value;          //value for a/d
    char    buf[10];            //buff for iota
    char    parity_rx, rx_data;
    long int    fvar;           //long for format math
    long int    tens;           //left of decm
    long int    decm;           //decimal places
    int     tempi;              //to add leadign zeros..
    int     vtxdata;             //volts int for TX
    int     itxdata;

    volatile unsigned int uart_data, parity;    // use 'volatile' qualifer as this is changed in ISR
/*
 *
 */
void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        parity_rx = RCSTA1bits.RX9D;
        uart_data = RCREG;     // read the incoming data
        PIR1bits.RCIF = 0;      // clear interrupt flag
    }

}

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

    ANSELB = 0b00001000;     //RB3, AN9

    TRISCbits.TRISC0 = 0; // output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // P1A PWM output
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

void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTA1bits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void serial_init(void)
{
    //2400 8E1
    // calculate values of SPBRGL and SPBRGH based on the desired baud rate
    //
    // For 8 bit Async mode with BRGH=0: Desired Baud rate = Fosc/64([SPBRGH:SPBRGL]+1)
    // For 8 bit Async mode with BRGH=1: Desired Baud rate = Fosc/16([SPBRGH:SPBRGL]+1)



    TXSTA1bits.BRGH=1;       // select low speed Baud Rate (see baud rate calcs below)
    TXSTA1bits.TX9=1;        // select 9 data bits
    TXSTA1bits.TXEN = 1;     // enable transmit


    RCSTA1bits.SPEN=1;       // serial port is enabled
    RCSTA1bits.RX9=1;        // select 9 data bits
    RCSTA1bits.CREN=1;       // receive enabled

    /*
     *bit 2 FERR: Framing Error bit
     *1 = Framing error (can be updated by reading RCREGx register and receive next valid byte)
     *0 = No framing error
     *bit 1 OERR: Overrun Error bit
     *1 = Overrun error (can be cleared by clearing bit CREN)
     *0 = No overrun error
     *bit 0 RX9D: Ninth bit of Received Data
     *This can be address/data bit or a parity bit and must be calculated by user firmware.
     */

    SPBRG1=103;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH1=0;

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt

         __delay_ms(50);        // give time for voltage levels on board to settle

}

int main(void) {

    init_io();
    serial_init();


    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x05; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block



    while (1) {
        PORTAbits.RA0 = 0;
        if (uart_data) {

            if (RCSTA1bits.FERR) {  // Frame Error, next read will reset
                uart_data = NULL;
            }

            if (RCSTA1bits.OERR) {  // Overrun Error
                RCSTA1bits.CREN=0;       // receive disabled
                __delay_us(1);
                RCSTA1bits.CREN=1;       // receive enabled
                uart_data = NULL;
            }

            parity = calculateparity(uart_data);
            if (parity_rx == parity) {

                rx_data = uart_data;
                if (rx_data == 'a') {
                    PORTAbits.RA0 = 1;
                    __delay_ms(1);
                    PORTAbits.RA0 = 0;
                }
            }


            uart_data = NULL;   //Done
        }


 //       PORTAbits.RA0 = 1;
 //       __delay_ms(40);
 //       PORTAbits.RA0 = 0;
 //       __delay_ms(40);


    }
    return (EXIT_SUCCESS);
}
