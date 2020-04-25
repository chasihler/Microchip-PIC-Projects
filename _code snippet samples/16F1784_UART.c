//  Software License Agreement
//
// The software supplied herewith by Microchip Technology Incorporated (the "Company")
// for its PICmicro® Microcontroller is intended and supplied to you, the Company's
// customer, for use solely and exclusively on Microchip PICmicro Microcontroller
// products.
//
// The software is owned by the Company and/or its supplier, and is protected under
// applicable copyright laws. All rights are reserved. Any use in violation of the
// foregoing restrictions may subject the user to criminal sanctions under applicable
// laws, as well as to civil liability for the breach of the terms and conditions of
// this license.
//
// THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES, WHETHER EXPRESS,
// IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE
// COMPANY SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
// CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
//*********************************************************************************** 
//***********************************************************************************
// This very simple program shows how to set up and use the UART on the
// PIC16F1784.
//
// This program sets up the UART to transmit at 9600 Baud and transmits the word
// 'Microchip' one byte at a time.  It also sets up the uart receive buffer to
// cause an interrupt if a data byte is received.
//**********************************************************************************
// Device: PIC16F1784
// Compiler: Microchip XC8 v1.12
// IDE: MPLAB X v1.7
// Created: May 2013
//
//**********************************************************************************


#include <xc.h> // include standard header file

// set Config bits
#pragma config FOSC=INTOSC, PLLEN=OFF, MCLRE=ON, WDTE=OFF
#pragma config LVP=OFF, CLKOUTEN=OFF,


// Definitions
#define _XTAL_FREQ  16000000    // this is used by the __delay_ms(xx) and __delay_us(xx) functions
#define IncomingData_LED   LATDbits.LATD4	// Incoming Data LED shows that uart data has been received

// Globals
volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR


//*************************************************************************************
// Interrupt Service Routine
// Check uart receive bit and if it is what caused the interrupt, turn LED on and
// read the data from the uart receive buffer.
// Note there is only one ISR to handle all interrupts so you need to determine
// what caused the interrupt before taking action.
//*************************************************************************************
void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        uart_data = RC1REG;     // read the incoming data
        IncomingData_LED = 1;   // set LED to indidate byte has been received
        PIR1bits.RCIF = 0;      // clear interrupt flag
    }

}

//*************************************************************************************
// Send one byte via UART
//*************************************************************************************
void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}


//*************************************************************************************
// ******************** main **********************************************************
// this just sets up I/O ports and then transmits the word "Microchip" and then sits
// and waits for incoming data.
//
//*************************************************************************************
void main() {
    
    
    // set up oscillator control register for internal clock running at 16Mhz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    OSCCONbits.IRCF = 0x0F; //set OSCCON IRCF bits to select OSC frequency=16Mhz
    
    OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    // PORT A Assignments
    TRISAbits.TRISA0 = 0; // RA0 = nc
    TRISAbits.TRISA1 = 0; // RA1 = nc
    TRISAbits.TRISA2 = 0; // RA2 = nc
    TRISAbits.TRISA3 = 0; // RA3 = nc
    TRISAbits.TRISA4 = 0; // RA4 = nc
    TRISAbits.TRISA5 = 0; // RA5 = nc
    TRISAbits.TRISA6 = 0; // RA6 = nc
    TRISAbits.TRISA7 = 0; // RA7 = nc

    ANSELA = 0x00; // all port A pins are digital I/O

    // if you want to enable weak pullups on some individual pins...here's how
    // WPUAbits.WPUA6 = 1;     // enable weak pullup on RA6
    //WPUAbits.WPUA7 = 1;     // enable weak pullup on RA7


    // PORT B Assignments
    TRISBbits.TRISB0 = 0; // RB0 = nc
    TRISBbits.TRISB1 = 0; // RB1 = nc
    TRISBbits.TRISB2 = 0; // RB2 = nc
    TRISBbits.TRISB3 = 0; // RB3 = nc
    TRISBbits.TRISB4 = 0; // RB4 = nc
    TRISBbits.TRISB5 = 0; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 = nc

    ANSELB = 0x00; // all port B pins are digital I/O

    // if you want to enable weak pullups on some individual pins...
    //WPUBbits.WPUB2 = 1;     // enable weak pullup on RB2
    //WPUBbits.WPUB3 = 1;     // enable weak pullup on RB3

    //	PORT C Assignments
    TRISCbits.TRISC0 = 0; // RC0 = nc
    TRISCbits.TRISC1 = 0; // RC1 = nc
    TRISCbits.TRISC2 = 0; // RC2 = nc
    TRISCbits.TRISC3 = 0; // RC3 = nc
    TRISCbits.TRISC4 = 0; // RC4 = nc
    TRISCbits.TRISC5 = 0; // RC5 = nc
    TRISCbits.TRISC6 = 0; // RC6 = TX out
    TRISCbits.TRISC7 = 1; // RC7 = RX in

 
    //	PORT D Assignments
    TRISDbits.TRISD0 = 0; // RD0 = nc
    TRISDbits.TRISD1 = 0; // RD1 = nc
    TRISDbits.TRISD2 = 0; // RD2 = nc
    TRISDbits.TRISD3 = 0; // RD3 = nc
    TRISDbits.TRISD4 = 0; // RD4 = IncomingData_LED
    TRISDbits.TRISD5 = 0; // RD5 = nc
    TRISDbits.TRISD6 = 0; // RD6 = nc
    TRISDbits.TRISD7 = 0; // RD7 = nc

    ANSELD=0x00;    // all port D pins are digital I/O

    
    //	PORT E Assignments
    TRISEbits.TRISE0 = 0; // RE0 = nc
    TRISEbits.TRISE1 = 0; // RE1 = nc
    TRISEbits.TRISE2 = 0; // RE2 = nc

    ANSELE=0x00;    // all port E pins are digital I/O

    
    //*************************************************************************************
    // Set up the UART
    // Note: See the datasheet for tables with the values of SPBRGH:SPBRGL already
    // calculated based on different values of Fosc and BRGH and desired
    // baud rates.
    //*************************************************************************************
    TXSTAbits.BRGH=0;       // select low speed Baud Rate (see baud rate calcs below)
    TXSTAbits.TX9=0;        // select 8 data bits
    TXSTAbits.TXEN = 1;     // enable transmit


    RCSTAbits.SPEN=1;       // serial port is enabled
    RCSTAbits.RX9=0;        // select 8 data bits
    RCSTAbits.CREN=1;       // receive enabled

    // calculate values of SPBRGL and SPBRGH based on the desired baud rate
    //
    // For 8 bit Async mode with BRGH=0: Desired Baud rate = Fosc/64([SPBRGH:SPBRGL]+1)
    // For 8 bit Async mode with BRGH=1: Desired Baud rate = Fosc/16([SPBRGH:SPBRGL]+1)

    // For our example,we will use BRGH=0,Fosc=16Mhz and we want baud rate=9600
    //
    //  9600 = Fosc/64([SPBRGH:SPBRGL]+1)
    //  9600 = Fosc/64(X+1)
    //  9600 = Fosc/64X + 64
    //  9600(64X + 64) = Fosc
    //  X = [Fosc/(9600)(64)]-1
    //  X = [16000000/(9600)(64)] -1
    //  X = SPBRGH:SPBRGL = 25.01 (round to 25)

    SPBRGL=25;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH=0;

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt

    IncomingData_LED = 0;   // turn incoming data LED off

     __delay_ms(50);        // give time for voltage levels on board to settle

    uart_xmit('M');         // transmit some data
    uart_xmit('i');
    uart_xmit('c');
    uart_xmit('r');
    uart_xmit('o');
    uart_xmit('c');
    uart_xmit('h');
    uart_xmit('i');
    uart_xmit('p');

    
    while (1);  // just sit and wait for incoming data
   
    
    
}








