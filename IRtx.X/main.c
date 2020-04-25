/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://0xEE.net
 *
 * Created on May 18, 2014, 3:21 PM
 *
 * Target Device:
 * 16F887 on breadboard
 *
 * Project: UART to Infrared Transmitter
 *
 * Version:
 * 0.1 initial
 *
 */
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 4000000 //4Mhz FRC internal osc
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#endif

// PIC16F887 Configuration Bit Settings

// 'C' source line config statements

#include <xc.h>

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1
#pragma config FOSC = INTRC_CLKOUT  // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF           // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF          // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON           // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF             // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF            // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF          // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF           // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF          // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)
#pragma config LVP = OFF            // Low Voltage Programming Enable bit (RB3/PGM pin has PGM function, low voltage programming enabled)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

int n;                          //for write_uart

void init_io(void) {
    ANSEL = 0x00;   // NO ADC
    ANSELH = 0x00;  //
    CM1CON0 = 0x00; //No comparators
    CM2CON0 = 0x00;

    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA3 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output
    TRISAbits.TRISA6 = 0; // CLKOUT
    TRISAbits.TRISA7 = 0; // output

    TRISBbits.TRISB0 = 0; // output
    TRISBbits.TRISB1 = 1; // input
    TRISBbits.TRISB2 = 0; // output
    TRISBbits.TRISB3 = 0; // output
    TRISBbits.TRISB4 = 0; // output
    TRISBbits.TRISB5 = 1; // input
    TRISBbits.TRISB6 = 1; // ICSP DAT
    TRISBbits.TRISB7 = 1; // ICSP CLK

    TRISCbits.TRISC0 = 0; // output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 0; // UART TX
    TRISCbits.TRISC7 = 1; // UART RX

    TRISDbits.TRISD0 = 0; // output
    TRISDbits.TRISD1 = 0; // output
    TRISDbits.TRISD2 = 0; // output
    TRISDbits.TRISD3 = 0; // output
    TRISDbits.TRISD4 = 0; // output
    TRISDbits.TRISD5 = 0; // output
    TRISDbits.TRISD6 = 0; //
    TRISDbits.TRISD7 = 0; //

    TRISEbits.TRISE0 = 1; // input
    TRISEbits.TRISE1 = 1; // input
    TRISEbits.TRISE2 = 1; // input
    TRISEbits.TRISE3 = 1; // /MCLR
}

void PWM_start(void)
{
    // initialize PWM module
    
    //start PWM at 38KHz 50% duty cycle
    
}

void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void write_uart(const char *txt)
{
    while(*txt != 0) uart_xmit(*txt++);     //this send a string to the TX buffer
                                            //one character at a time
}

void serial_init(void)
{

    // calculate values of SPBRGL and SPBRGH based on the desired baud rate

    TXSTAbits.BRGH=1;       // select low speed Baud Rate (see baud rate calcs below)
    TXSTAbits.TX9=0;        // select 8 data bits
    TXSTAbits.TXEN = 1;     // enable transmit


    RCSTAbits.SPEN=1;       // serial port is enabled
    RCSTAbits.RX9=0;        // select 8 data bits
    RCSTAbits.CREN=1;       // receive enabled

    SPBRG=25;               //9600 Baud

/*
 *
 * The next section is commented out, it's there in case you want to know
 * how to use the interrupts for the UART
 *
 */

    //PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    //PIE1bits.RCIE=1;        // enable UART Receive interrupt
    //INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    //INTCONbits.GIE = 1;     // enable global interrupt

    __delay_ms(100);        // give time for voltage levels on board to settle

    // Example: uart_xmit('R');         // transmit a single character example

}



 void main(void)
 {
     OSCCONbits.IRCF = 0x06; //set OSCCON IRCF bits to select internal OSC frequency 4MHz

     init_io();             //set up ports
     serial_init();         //set up UART


     while ( 1 ) {

         write_uart("Hello, world!");       //send Hello, world! to UART
         uart_xmit(0x0A);                   //send a line feed
         __delay_ms(1000);                  //delay 1 second
         write_uart("From 0xEE.net");       //send From 0xEE.net
         uart_xmit(0x0A);                   //send line feed
         __delay_ms(1000);                  //delay 1 second
     }                                      //... wash, rinse, repeat
 }
