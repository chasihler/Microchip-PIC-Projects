/* 
 * File:   newmain.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on September 26, 2014, 2:47 PM
 *
 * Target Device:
 * 16F1509 on Tautic 20 pin dev board
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
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=ON, CP=OFF, BOREN=ON, CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF

#define _XTAL_FREQ 4000000 //defined for delay
    signed char c;

/*
 * 
 */
void init_io(void) {

    ANSELA = 0x00; // all port A pins are digital I/O
    ANSELB = 0x00; // all port B pins are digital I/O
    ANSELC = 0x00; // all port B pins are digital I/O

    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA3 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output

    TRISBbits.TRISB4 = 0; // RB4 = nc
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 = nc

    TRISCbits.TRISC0 = 0; // output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 0; //
    TRISCbits.TRISC7 = 0; // 

}

int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    //OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    while (1) {

    init_io();
        c=0;
        LATC=c;
        __delay_ms(100);
        c=1;
        LATC=c;
        __delay_ms(100);
        c=2;
        LATC=c;
        __delay_ms(100);
        c=3;
        LATC=c;
        __delay_ms(100);

    }
    return (EXIT_SUCCESS);
}
