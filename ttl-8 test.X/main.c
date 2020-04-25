/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on September 26, 2014, 2:47 PM
 *
 * Target Device:
 * 16F1509 on Tautic 20 pin dev board
 *
 * Project: ttl-8 test
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
 

/*
 *
 */
int r;
unsigned char n;

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

    TRISBbits.TRISB4 = 0; // output
    TRISBbits.TRISB5 = 1; // input
    TRISBbits.TRISB6 = 0; // output
    TRISBbits.TRISB7 = 0; // output

    TRISCbits.TRISC0 = 0; // output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // DATA OUT
    TRISCbits.TRISC4 = 0; // CLOCK
    TRISCbits.TRISC5 = 0; // LATCH
    TRISCbits.TRISC6 = 0; // output
    TRISCbits.TRISC7 = 0; // output

}

void latch(void) {
    PORTCbits.RC5 = 1;  //latch bump
    //__delay_us(1);      //this is slow.. that's okay for me
    PORTCbits.RC5 = 0;
}

void clk(void){
    PORTCbits.RC4 = 1;  //set clock
    //__delay_us(1);      //this is slow.. that's okay for me
    PORTCbits.RC4 = 0;
}

void shift_out (unsigned int x){
    r = 8;

    while(r){
        if (x & 0b10000000){
            LATCbits.LATC3 = 1;

        }
        else{
            LATCbits.LATC3 = 0;
        }

        clk();
        x = x << 1;
        --r;
        LATCbits.LATC3 = 0;
    }

    latch();
}

int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    //OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    init_io();

    latch();

    while (1) {

        n = n+1;;

        shift_out(n);

        //__delay_ms(50);

    }
    return (EXIT_SUCCESS);
}
