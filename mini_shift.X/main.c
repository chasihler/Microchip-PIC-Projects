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
 * Project: MIDI Slave
 *
 *
 * Version:
 * 0.1  Configuration, 31.25Kbaud TX&RX
 * 0.2  SPI output works
 * 0.3
 *
 */
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 16000000 //4Mhz FRC internal osc
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

#define _XTAL_FREQ 16000000 //defined for delay

/*
 * Variables
 */

    //long int    decm;                 //long temp
    int     spi_msb, spi_lsb;           //data
    int     i, x;                          //counter
    int     itxdata, ilevel;            //int UART data
    char    buf[10];            //buff for iota
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

    //BRGH=1        31.25KHz
    //SPBRG=7

    SPBRG=7;               //

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt

         __delay_ms(10);        // give time for voltage levels on board to settle

    //  uart_xmit('R');         // transmit a character example

}

void spi_write(unsigned int data_byte) {

            LATCbits.LATC2 = 0;     //set CS low (start chip select)
            //__delay_us(0);

            SSPBUF = data_byte;
            __delay_us(2);     //!SSPSTATbits.BF does not work
}

void spi_stop(void) {
    LATCbits.LATC2 = 1;     // set CS high (end transmission)
}


void spi_init(void) {
    SSPCON1bits.WCOL = 0;        //  Just in case..
    SSPCON1bits.SSPM = 0x00;        //  FOSC/4
    SSPCON1bits.CKP = 0;         //  SPI Mode 0
    SSPSTATbits.CKE = 1;
    SSPSTATbits.SMP = 1;
    SSPCON1bits.SSPEN = 1;       //  SPI GO!
}



void init_io(void) {
    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output

    ANSEL = 0x00;         // no A/D
    ANSELH = 0x00;

    TRISBbits.TRISB4 = 1; // SPI SDI
    TRISBbits.TRISB5 = 1; // UART RX
    TRISBbits.TRISB6 = 0; // SPI SCK
    TRISBbits.TRISB7 = 0; // SPI SDO

    TRISCbits.TRISC0 = 0; // LED
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // DAC /CS CHIP SELECT
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 0; // SPI SS
    TRISCbits.TRISC7 = 0; // SPI SDO

}

void set_cmd11(void)
{
    //set an AD output
                while(uart_data==0x11)
            {
                i++; //wait for next char
            }
            ilevel = uart_data;
            uart_xmit(ilevel);
}

void check0xb1(void)
{
    if (uart_data == 0xB1)
    {
            while(uart_data==0xB1)
            {
                i++; //wait for next char
            }
            if ( uart_data == 0x11)
               set_cmd11();
    }
}

int main(void) {

    init_io();

    // set up oscillator control register, using internal OSC at 16MHz.
    OSCCONbits.IRCF = 0x07; //set OSCCON IRCF bits to select OSC frequency 16MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block

    SSPCON1bits.SSPEN = 0;       //  SPI GO!


    serial_init();
    spi_init();

    LATAbits.LATA0=0;

    LATCbits.LATC2 = 1;     //turn off CS on DAC

    spi_write(0x0E);        //comm inbetween a comm the comm doesn't work
                            //so you manually have to stop /CS spi_stop()
    spi_stop();             //CS

    while (1) {

            x++;
            i = x;
   //         __delay_ms(1);

            spi_msb = ((i >> 8) & 0xff);    //pick i apart
            spi_lsb = ((i >> 0) & 0xff);

            spi_msb = spi_msb + 0x50;       //add configuration

            //spi_msb = 0x50;

            //spi_write(0x01);        //my spi_write always sets /CS but if I drop it the
            //spi_write(0x07);        //comm inbetween a comm the comm doesn't work
                                    //so you manually have to stop /CS spi_stop()


            //spi_stop();     //CS

            if (x > 0x01FE) {
                x = 0x00;
            }

        //while (uart_data)
        //{
            //check0xb1();
            //uart_data=0;
        //}

    }
    return (EXIT_SUCCESS);
}
