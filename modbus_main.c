/* 
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on November 8, 2014, 2:37 PM
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
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=ON, CP=OFF, BOREN=ON, CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF

#define _XTAL_FREQ 8000000 //defined for delay

    char    ctxt[120];            //buff serial string
    volatile unsigned int     ping, isrcall, index, reading, new_rx;
    int address;

/*
 *  Interrupt Service
 */
void interrupt ISR() {
/*    *    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    *{
    *    parity_rx = RCSTA1bits.RX9D;
    *    uart_data = RCREG;     // read the incoming data
    *    PIR1bits.RCIF = 0;      // clear interrupt flag
    *}
        */
    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        isrcall = 0x01;
        char temp;
        temp = RCREG;     // read the incoming data
        if(temp==address && new_rx==0)      //if my address..
        {
            index = 0;                  //reset index
            reading = 1;                //from now on go to else if
            //TOODO start timeout timer
        }
        else if(reading == 1)           //in middle of GPS sentence
        {
            //TODO reset timeout timer
            ctxt[index] = temp;         //load it up
            index++;                    //increment index
            ping = 1;                   //this is for debugging
            if(index > 6)              //1+7 = master frame.
                {
                index = 0;              //reset index
                reading = 0;            //no longer storing the string
                new_rx = 1;             //"ding"
                }
        }
        //PIR3bits.RC2IF = 0;      // clear interrupt flag
    }
    //RCSTA2bits.FERR = 0;    //Clear errors
    //RCSTA2bits.OERR = 0;

    //time out timer, if tripped new_rx=0;
    if (INTCONbits.T0IF)
    {
        new_rx=0;
        INTCONbits.T0IF = 0;
    }
}

void init_io(void) {

    ANSELA = 0x00; // all port A pins are digital I/O
    ANSELB = 0x00; // all port B pins are digital I/O
    ANSELC = 0x00; // all port B pins are digital I/O

    LATC = 0x00;

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
    TRISCbits.TRISC4 = 1; // address input
    TRISCbits.TRISC5 = 1; // address input
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input    
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

void init_timer (void) {
        TMR0=0;
        TMR0L=0;
        TMR0H=0;
        INTCONbits.T0IF =0;
        T0CONbits.PSA = 0;          //Turn on Timer 8 bit w/ 256 prescale;
        T0CONbits.T0PS = 2;
        T0CONbits.T0CS = 0;
        T0CONbits.T08BIT = 0;
        //    T0CONbits.TMR0ON = 1;
        //pulse_time = TMR0;

        /*
        PIE1bits.TMR1IE=0;
        TMR1=0;
        PIR1bits.TMR1IF=0;
        T1CON=0x39; //39; //timer on prescale 8
         */
}

void check_my_address(void) {
    /*
     *  Determine what address the unit is based on DIP switches
     *  PORTCbits.RC4   Switch 1
     *  PORTCbits.RC5   Switch 2
    */

    if (PORTCbits.RC4 & PORTCbits.RC5) address = 0x04;
    if (!PORTCbits.RC4 & PORTCbits.RC5) address = 0x03;
    if (PORTCbits.RC4 & !PORTCbits.RC5) address = 0x02;
    if (!PORTCbits.RC4 & !PORTCbits.RC5) address = 0x01;

}

int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)
    new_rx=0;

    init_io();
    init_timer();       //for timeout

    check_my_address();

    while (1) {

        //If read data, not me and then if no 232 traffic for >800us then any xmit over
        //104 us (9600) x 8 quiet periods minus some for wiggle.




    }
}
