/* 
 * File:   cmain.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on April 25, 2014, 2:55 PM
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
#define _XTAL_FREQ 8000000 //4Mhz FRC internal osc
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#endif

#include <xc.h>


//config bits
#pragma config OSC=INTOSC, WDTEN=OFF, CP0=OFF
#pragma config STVREN=ON, IESO=OFF, FCMEN=OFF
#pragma config XINST = OFF   // Extended Instruction Set ON, to optimise code specifically for C

#define _XTAL_FREQ 8000000 //defined for delay

/*
 * 
 */
void init_io(void) {
    TRISAbits.TRISA0 = 0; //LED1
    TRISAbits.TRISA1 = 0; //LED2
    TRISAbits.TRISA2 = 0; //AN
    ADCON0 = 0x00;           //all digial

    TRISBbits.TRISB0 = 0; // HEADER
    TRISBbits.TRISB1 = 0; // HEADER
    TRISBbits.TRISB2 = 0; // RST
    TRISBbits.TRISB3 = 0; // CS
    TRISBbits.TRISB4 = 0; // SCK
    TRISBbits.TRISB5 = 1; // MISO

    TRISCbits.TRISC0 = 0; // HEADER
    TRISCbits.TRISC1 = 0; // HEADER
    TRISCbits.TRISC2 = 0; // HEADER
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC6 = 1; // PWM
    TRISCbits.TRISC7 = 1; // MOSI

    TRISDbits.TRISD0 = 1; // SCL
    TRISDbits.TRISD1 = 1; // SCA
    TRISDbits.TRISD4 = 1; // INT
    TRISDbits.TRISD5 = 1; // RX
    TRISDbits.TRISD6 = 0; // TX

    TRISEbits.TRISE0 = 1; // HEADER
    TRISEbits.TRISE1 = 1; // HEADER
    TRISEbits.TRISE2 = 1; // HEADER

}

int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x07; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x03; //set the SCS bits to select internal oscillator block

    init_io();

    LATAbits.LA1 = 1;
    
    while (1) {
        LATAbits.LA0 = 1;
        __delay_ms(50);
        LATAbits.LA0 = 0;
        __delay_ms(50);
    }
}
