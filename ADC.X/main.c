/* 
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on April 13, 2014, 1:14 PM
 *
 * Target Device:
 * 16F1509 on Tautic 20 pin dev board
 *
 * Project: DAC ramp test
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

int x;  //DAC counter
    int     itxdata;            //int RS232 tx data
    char    buf[10];            //buff for iota
    volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR
/*
 * 
 */
void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void serial_init(void)
{

    // calculate values of SPBRGL and SPBRGH based on the desired baud rate
    //
    // For 8 bit Async mode with BRGH=0: Desired Baud rate = Fosc/64([SPBRGH:SPBRGL]+1)
    // For 8 bit Async mode with BRGH=1: Desired Baud rate = Fosc/16([SPBRGH:SPBRGL]+1)



    TXSTAbits.BRGH=1;       // select low speed Baud Rate (see baud rate calcs below)
    TXSTAbits.TX9=0;        // select 8 data bits
    TXSTAbits.TXEN = 1;     // enable transmit


    RCSTAbits.SPEN=1;       // serial port is enabled
    RCSTAbits.RX9=0;        // select 8 data bits
    RCSTAbits.CREN=1;       // receive enabled

    SPBRG=25;               //9600

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt
    INTCONbits.T0IE = 0;

         __delay_ms(50);        // give time for voltage levels on board to settle
}


void init_io(void) {
    TRISAbits.TRISA0 = 0; // SONAR PULSE
    TRISAbits.TRISA1 = 1; // ECHO
    TRISAbits.TRISA2 = 0; // STATUS LED
    TRISAbits.TRISA4 = 1; // MOTOR DRIVER ERROR IN
    TRISAbits.TRISA5 = 0; // MOTOR DRIVER /RESET

    ANSEL = 0x00;         // no A/D
    ANSELH = 0x00;

    TRISBbits.TRISB4 = 0; // RB4 = nc
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 MOTOR DRIVER TX

    TRISCbits.TRISC0 = 1; // left cliff sensor in
    TRISCbits.TRISC1 = 1; // right cliff sensor
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input

}

    void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        uart_data = RCREG;     // read the incoming data
        PIR1bits.RCIF = 0;      // clear interrupt flag
                                //
    }
    if (INTCONbits.T0IF)
    {
        LATAbits.LATA0 = 1;
        INTCONbits.T0IF = 0;
    }

}

int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    init_io();
    serial_init();

    x = 0;

    while (1) {



                    if (PORTBbits.RB7==1)
                uart_xmit('A');


    }
    return (EXIT_SUCCESS);
}
