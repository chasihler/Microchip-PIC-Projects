/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
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

int count;
/*
 *
 *
 *
 * NCOxCON: NCOx CONTROL REGISTER
 * bit 7 NxEN: NCOx Enable bit
 * 1 = NCOx module is enabled
 * 0 = NCOx module is disabled
 * bit 6 NxOE: NCOx Output Enable bit
 * 1 = NCOx output pin is enabled
 * 0 = NCOx output pin is disabled
 * bit 5 NxOUT: NCOx Output bit
 * 1 = NCOx output is high
 * 0 = NCOx output is low
 * bit 4 NxPOL: NCOx Polarity bit
 * 1 = NCOx output signal is active low (inverted)
 * 0 = NCOx output signal is active high (non-inverted)
 * bit 3-1 Unimplemented: Read as ?0?
 * bit 0 NxPFM: NCOx Pulse Frequency Mode bit
 * 1 = NCOx operates in Pulse Frequency mode
 * 0 = NCOx operates in Fixed Duty Cycle mode
 *
 * NCOxCLK: NCOx INPUT CLOCK CONTROL REGISTER
 * bit 7-5 NxPWS<2:0>: NCOx Output Pulse Width Select bits(1, 2)
 * 111 = 128 NCOx clock periods
 * 110 = 64 NCOx clock periods
 * 101 = 32 NCOx clock periods
 * 100 = 16 NCOx clock periods
 * 011 = 8 NCOx clock periods
 * 010 = 4 NCOx clock periods
 * 001 = 2 NCOx clock periods
 * 000 = 1 NCOx clock periods
 * bit 4-2 Unimplemented: Read as ?0?
 * bit 1-0 NxCKS<1:0>: NCOx Clock Source Select bits
 * 11 = NCO1CLK pin
 * 10 = LC1_out
 * 01 = FOSC
 * 00 = HFINTOSC (16 MHz)
 * Note 1: NxPWS applies only when operating in Pulse Frequency mode.
 * 2: If NCOx pulse width is greater than NCO_overflow period, operation is undeterminate.
 *
 */
int main(void) {
            // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    TRISCbits.TRISC0 = 0;                   // output
    TRISCbits.TRISC1 = 0;                   // NCO1
    TRISCbits.TRISC2 = 0;                   // output
    TRISCbits.TRISC3 = 0;                   // output
    TRISCbits.TRISC6 = 1;                   // input
    TRISCbits.TRISC7 = 1;                   // input

    ANSELC = 0x00; // all port C pins are digital I/O

    NCO1INCH = 0x00;                        // has to be set before INCL
    NCO1INCL = 0x4F;                        // low accumulator register

    NCO1CLK = 0xE1; // 0xE1 = PW 128 clk 0x41 [010][000][01] 8 CLK PER, INT OSC,
    NCO1CON = 0xE1; // Enable, Ouput On, PWF Mode

    count = 0;

    while(1)
    {
            count = count+1;            //just a little test code
            if (count < 1)              //LED didn't like the low freq
                count=10;
            NCO1INCH = 0x00;                        //has to be set before INCL
            NCO1INCL = count;
            __delay_ms(50);
    }
    return (EXIT_SUCCESS);
}

