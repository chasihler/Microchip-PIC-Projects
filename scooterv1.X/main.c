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

 *
 */
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 4000000 //4Mhz FRC internal osc
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#endif

#include <xc.h>

//config bits
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=ON, MCLRE=ON, CP=OFF, BOREN=OFF, CLKOUTEN=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF
//IESO=OFF

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
    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA3 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output


    ANSELA = 0x00; // all port A pins are digital I/O


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
    TRISCbits.TRISC7 = 1; // AN9
    ANSELC = 0x00; // all port B pins are digital I/O
}

void pwm_init(){




    PWM1CON = 0x00;
    PR2 = 0xAE;
    PWM1DCHbits.PWM1DCH6 = 0;
    PWM1DCHbits.PWM1DCH7 = 0;
    PIR1bits.TMR2IF = 0;
    T2CONbits.T2CKPS = 1;
    T2CONbits.TMR2ON = 1;

    PWM1CONbits.PWM1EN = 1;
    PWM1CONbits.PWM1OE = 1;






}

void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void serial_init(void)
{
    //9600 8N1
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

    SPBRG=25;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH=0;

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
    OSCCONbits.IRCF = 0x0D; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block

    ADCON0 = 0b00100101;                            //select AN9 and enable
    ADCON1 = 0b00000000;                  //speed Vref=AVdd, VssRef=AVss
    ADCON2 = 0b00111011;                //ledft justified, 20RAD, FRC

    __delay_us(5);


    while (1) {

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
        PWM1DCH = vtxdata;

    }
    return;
}
