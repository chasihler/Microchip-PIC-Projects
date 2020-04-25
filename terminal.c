/* 
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on March 13, 2014, 4:12 PM
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
#pragma config FOSC=INTIO67, WDTEN=OFF, PWRTEN=OFF, CP0=OFF, CP1=OFF, BOREN=ON
#pragma config STVREN=ON, LVP=OFF, HFOFST=OFF, IESO=OFF, FCMEN=OFF

//WRT=OFF, FOSC=INTOSC, MCLRE=ON

#define _XTAL_FREQ 4000000 //defined for delay

    int     an8_value, an9_value;          //value for a/d
    char    buf[10];            //buff for iota
    long int    fvar;           //long for format math
    long int    tens;           //left of decm
    long int    decm;           //decimal places
    int     tempi;              //to add leadign zeros..
    int     vtxdata;             //volts int for TX
    int     itxdata;

    volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR
/*
 * 
 */
void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        uart_data = RCREG;     // read the incoming data
        PIR1bits.RCIF = 0;      // clear interrupt flag
    }

}

void init_io(void) {
    TRISAbits.TRISA0 = 1; // mx column 1
    TRISAbits.TRISA1 = 1; // mx column 2
    TRISAbits.TRISA2 = 1; // mx colum 3
    TRISAbits.TRISA3 = 1; // mx row 1
    TRISAbits.TRISA4 = 1; // mx row 2
    TRISAbits.TRISA5 = 1; // mx row 3
    TRISAbits.TRISA6 = 1; // in
    TRISAbits.TRISA7 = 1; // in

    ANSELA = 0x00; // all port A pins are digital I/O

    TRISBbits.TRISB1 = 1; // input
    TRISBbits.TRISB2 = 0; // P1B PWM output
    TRISBbits.TRISB3 = 1; // AN9    speed control 0-5V
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

void pwm_init(){


//    CCPR1L = 0x120;
    CCPR1Lbits.CCPR1L = 0xFE;
    PR2 = 0xAE;
    CCPTMRS0bits.C1TSEL = 0;     //CCP TMR2 Selection
    CCP1CONbits.P1M = 0x02;     //half bridge
    CCP1CONbits.DC1B = 0x00;
    PWM1CONbits.P1RSEN = 0;
    PWM1CONbits.P1DC = 0x1F;    //dead band delay
    ECCP1ASbits.CCP1AS = 0x00;
    ECCP1ASbits.CCP1ASE = 0;    //Auto-shutdown off
    CCP1CONbits.CCP1M = 0x0C;
    PSTR1CONbits.STR1A = 1;
    PSTR1CONbits.STR1B = 1;

    
    T2CONbits.T2CKPS = 1;
    T2CONbits.TMR2ON = 1;



}

void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTA1bits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void serial_init(void)
{
    //9600 8N1
    // calculate values of SPBRGL and SPBRGH based on the desired baud rate
    //
    // For 8 bit Async mode with BRGH=0: Desired Baud rate = Fosc/64([SPBRGH:SPBRGL]+1)
    // For 8 bit Async mode with BRGH=1: Desired Baud rate = Fosc/16([SPBRGH:SPBRGL]+1)



    TXSTA1bits.BRGH=1;       // select low speed Baud Rate (see baud rate calcs below)
    TXSTA1bits.TX9=0;        // select 8 data bits
    TXSTA1bits.TXEN = 1;     // enable transmit


    RCSTA1bits.SPEN=1;       // serial port is enabled
    RCSTA1bits.RX9=0;        // select 8 data bits
    RCSTA1bits.CREN=1;       // receive enabled

    SPBRG1=25;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH1=0;

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt

         __delay_ms(50);        // give time for voltage levels on board to settle

    uart_xmit('R');         // transmit some data
}

int main(void) {

    init_io();
    serial_init();

    LATCbits.LATC2 = 0;
    pwm_init();


    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x05; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block

    ADCON0 = 0b00100101;                            //select AN9 and enable
    ADCON1 = 0b00000000;                  //speed Vref=AVdd, VssRef=AVss
    ADCON2 = 0b00111011;                //ledft justified, 20RAD, FRC

    __delay_us(5);


    while (1) {

        PORTAbits.RA0 = 1;
        __delay_ms(40);
        PORTAbits.RA0 = 0;
        __delay_ms(40);

        ADCON0 = 0b00100101;                            //select AN9 and enable
        __delay_us(5);
        GO = 1;
        while (GO) continue;              //wait for conversion
        an9_value = ADRESH;               //AN9 value

        fvar = an9_value;
        fvar = fvar * 10749;        //calibration
        fvar = fvar / 256;
        tens = fvar / 100;
        //tens = tens % 10;
        decm = fvar % 100;
        vtxdata = fvar / 42;
        vtxdata = vtxdata - 0x28;
        uart_xmit(vtxdata);
        CCPR1Lbits.CCPR1L = vtxdata;

    }
    return (EXIT_SUCCESS);
}
