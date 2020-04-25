/* 
 * File:   main.c
 * Author: Charles Ihler
 *
 *
 * Device: dsPIC33FJ16GS402
 *
 * Project: Hello World
 *
 * Blink an LED at 10Hz on RB15 for confirmation of OSC
 *
 * Created on March 7, 2015, 8:39 PM
 */

 #define FFRC 7372800ULL         // FRC oscillator frequency, Hz
 #define FOSC 80000000ULL        // Desired system clock frequency, Hz
 #define FCY     (FOSC/2)

#define BAUDRATE 31250
#define BRGVAL ((FP/BAUDRATE)/16)-1

#include <p33Fxxxx.h>
#include <stdio.h>
#include <stdlib.h>
#include <libpic30.h>

  _FOSCSEL(FNOSC_FRCPLL)              //set clock for internal OSC with PLL
  _FOSC(OSCIOFNC_OFF & POSCMD_NONE)   //no clock output, external OSC disabled
  _FWDT(FWDTEN_OFF)                   //disable the watchdog timer
  _FICD(JTAGEN_OFF & ICS_PGD1);       //disable JTAG, enable debugging on PGx1 pins
/*
 * 
 */

void blink(void){


    PORTBbits.RB15 = 1;         //on
    __delay_ms(50);
    PORTBbits.RB15 = 0;         //off
    __delay_ms(50);
}

void init_uart(void){
    U1MODEbits.STSEL = 0; // 1-Stop bit
U1MODEbits.PDSEL = 0; // No Parity, 8-Data bits
U1MODEbits.ABAUD = 0; // Auto-Baud disabled
U1MODEbits.BRGH = 0; // Standard-Speed mode
U1BRG = BRGVAL; // Baud Rate setting for 9600
U1STAbits.UTXISEL0 = 0; // Interrupt after one TX character is transmitted
U1STAbits.UTXISEL1 = 0;
IEC0bits.U1TXIE = 1; // Enable UART TX interrupt
U1MODEbits.UARTEN = 1; // Enable UART
U1STAbits.UTXEN = 1; // Enable UART TX
/* Wait at least 105 microseconds (1/9600) before sending first char */
DELAY_105uS
U1TXREG = 'a'; // Transmit one character
}


int main(int argc, char** argv) {

    
    // setup internal clock for 80MHz/40MIPS 
    // 7.37/2=3.685*43=158.455/2=79.2275 
    CLKDIVbits.PLLPRE=0;        // PLLPRE (N2) 0=/2
    PLLFBD=41;                  // pll multiplier (M) = +2
    CLKDIVbits.PLLPOST=0;       // PLLPOST (N1) 0=/2
    while(!OSCCONbits.LOCK);    // wait for PLL ready

    ADPCFG = 0xFFFF;            //kill all analog Pins, digital only.
    ODCBbits.ODCB15 = 0;        //normal output
    TRISBbits.TRISB15 = 0;      //output

    while(1) {
        blink();
    }


    return (EXIT_SUCCESS);
}

