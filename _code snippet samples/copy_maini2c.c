/* 
 * File:   main.c
 * Author: Chas
 *
 * Created on January 9, 2014, 7:34 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <plib.h>
#include <xc.h>                            //PIC hardware mapping

//config bits
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=ON, CP=OFF, BOREN=ON, CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF

// Definitions
#define _XTAL_FREQ  16000000        // this is used by the __delay_ms(xx) and __delay_us(xx) functions

unsigned char I2C_send[20] = "MICROCHIP:MASTER";
unsigned char I2C_recv[21];

                                            //main
void main(void) {
 
      // set up oscillator control register
//    OSCCONbits.IRCF = 0x0F; //set OSCCON IRCF bits to select OSC frequency=16Mhz
//    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block


    TRISBbits.TRISB4 = 1;                   // SCA
    TRISBbits.TRISB6 = 1;                   // SCL
    TRISCbits.TRISC0 = 0;                   //using pin as output


    __delay_ms(10); // let everything settle.

    LATC = 0;                               //clear all pins to 0
    LATCbits.LATC0 = 0;                     //turn ON the LED by writing to the latch
    while(1) continue;                      //sit here forever doing nothing, think "while(true), continue in this loop"

}