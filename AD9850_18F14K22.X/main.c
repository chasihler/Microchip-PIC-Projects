/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on February 8, 2014, 11:39 AM
 *
 * Target Device:
 * 18F14K22 on Tautic 20 pin dev board
 *
 * Project: AD9850 Frequency Generator
 *
 *
 * Version:
 * 0.1  IO Configuration RS232/TX
 * 0.2
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
#pragma config FOSC=IRC, WDTEN=OFF, PWRTEN=OFF, MCLRE=ON, CP0=OFF, CP1=OFF, BOREN=ON
#pragma config STVREN=ON, LVP=OFF, HFOFST=OFF, IESO=OFF, FCMEN=OFF

#define _XTAL_FREQ 4000000 //defined for delay

/*
 * Variables
 */

    int     w0,w1,w2,w3,w4;     //confirguration words
    int     i;                  //temp
    //int     itxdata;            //int RS232 tx data
    //char    buf[10];            //buff for iota
    volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR

/*
 *  Functions
 */

    void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        uart_data = RCREG;     // read the incoming data
        PIR1bits.RCIF = 0;      // clear interrupt flag
                                //
    }

}

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

    //BRGH=1        31.25kbps
    //SPBRG=7

    SPBRG=25;               //9615bps

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt

         __delay_ms(50);        // give time for voltage levels on board to settle

    //  uart_xmit('R');         // transmit a character example

}


void init_io(void) {
    TRISAbits.TRISA0 = 0; // W_CLK  Word Clock
    TRISAbits.TRISA1 = 0; // FU_UD  Frequency Update
    TRISAbits.TRISA2 = 0; // LED
    TRISAbits.TRISA4 = 0; // RESET
    TRISAbits.TRISA5 = 0; // output

    ANSEL = 0x00;         // no A/D
    ANSELH = 0x00;

    TRISBbits.TRISB4 = 0; // RB4 = nc
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 = nc

    TRISCbits.TRISC0 = 0; // output D0
    TRISCbits.TRISC1 = 0; // output ..
    TRISCbits.TRISC2 = 0; // output ..
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 0; // output
    TRISCbits.TRISC7 = 0; // output D7

    LATC=0x00;

}

void RST(void)
{
    LATAbits.LATA4 = 1;
    __delay_us(10);
    LATAbits.LATA4 = 0;
    __delay_us(10);
}

void WCLK(void)
{
    LATAbits.LATA0 = 1;
    __delay_us(10);
    LATAbits.LATA0 = 0;
    __delay_us(10);
}

void FQ_UD(void)
{
    LATAbits.LATA1 = 1;
    __delay_us(10);
    LATAbits.LATA1 = 0;
    __delay_us(10);
}

int main(void) {

    init_io();

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x05; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block

    serial_init();
    __delay_ms(10);     //simma down!

    RST();

    LATC = 0x00;
    WCLK();
    __delay_us(5);
    FQ_UD();

    __delay_ms(100);     //simma down!

    //w0 <ph-4><ph3><ph2><ph1><ph0><power-down><control><control>
    //w1    <freq-31>...
    //w2
    //w3
    //w4    ...<freq-0>


    LATAbits.LATA2=1;

    w0 = 0xA0;   //01110000
    w1 = 0x1D;
    w2 = 0xFF;
    w3 = 0xFF;
    w4 = 0xF0;

    LATC = w0;
    __delay_us(1);
    WCLK();
    __delay_us(5);
    LATC = w1;
    __delay_us(1);
    WCLK();
    __delay_us(5);
    LATC = w2;
    __delay_us(1);
    WCLK();
    __delay_us(5);
    LATC = w3;
    __delay_us(1);
    WCLK();
    __delay_us(5);
    LATC = w4;
    __delay_us(1);
    WCLK();
    __delay_us(5);
    FQ_UD();

    LATAbits.LATA2=0;

    __delay_ms(149);

    while (1) {

        i++;
            LATAbits.LATA2 = 1;
            __delay_ms(149);
            LATAbits.LATA2 = 0;
            __delay_ms(149);
    }
    return (EXIT_SUCCESS);
}
