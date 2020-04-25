/* 
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on April 13, 2014, 1:14 PM
 *
 * Target Device:
 * 18F14K22 on Homemade TIA-485 Dev Board
 *
 * Project: TIA-485 testing
 *
 * Version:
 * 0.1  Hello World
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
#pragma config FOSC=IRC, WDTEN=OFF, PWRTEN=OFF, MCLRE=ON, CP0=OFF, CP1=OFF, BOREN=ON
#pragma config STVREN=ON, LVP=OFF, HFOFST=OFF, IESO=OFF, FCMEN=OFF

#define _XTAL_FREQ 4000000 //defined for delay

int x;  //DAC counter
    int     itxdata;            //int RS232 tx data
    char    buf[10];            //buff for iota
    char    keyp;                //key pressed
    int    noncharkey;          //non character key press see mapping
    int     count1, count2;                   //keypress PORT value

    volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR

struct {
    unsigned int keypress :1;
    unsigned int nonchar :1;
    unsigned int waiting :1;
    unsigned int puppy_mode :1;
} FLAGbits;

 /*
  *
  *  noncharkey     1:down arrow    2:left arrow    3:
  *                 4:-6
  *                 7:-9
  *                 10:DEL          11:HOME         12:RETURN
  *
  *
  *
 */
void uart_xmit(unsigned int mydata_byte) {
    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void write_uart(const char *txt){
    while(*txt != 0) uart_xmit(*txt++);     //this send a string to the TX buffer
                                            //one character at a time
}

void serial_init(void)
{

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

    SPBRG=25;               //9600

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt
    INTCONbits.T0IE = 0;

         __delay_ms(50);        // give time for voltage levels on board to settle
}


void init_io(void) {
    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 1; // input
    TRISAbits.TRISA2 = 1; // mx column 1 input
    TRISAbits.TRISA4 = 1; // mx column 2 input
    TRISAbits.TRISA5 = 1; // mx column 3 input

    ANSEL = 0x00;         // no A/D
    ANSELH = 0x00;

    TRISBbits.TRISB4 = 0; // RB4 = nc
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 VGA RS232 TX

    TRISCbits.TRISC0 = 1; // VGA /RTS
    TRISCbits.TRISC1 = 1; // input
    TRISCbits.TRISC2 = 1; // VGA Chip Select (so we can re-use the bypass RS-232)
    TRISCbits.TRISC3 = 0; // mx row 1 output
    TRISCbits.TRISC4 = 0; // mx row 2 output
    TRISCbits.TRISC5 = 0; // mx row 3 output
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input

}

    void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        uart_data = RCREG;     // read the incoming data
        PIR1bits.RCIF = 0;      // clear interrupt flag
                                //
    }
    if (INTCONbits.T0IF)
    {
        LATAbits.LATA0 = 1;
        INTCONbits.T0IF = 0;
    }

}


int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x05; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    //OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    init_io();
    serial_init();

    x = 0;
    
    __delay_ms(100);

    while (1) {
        //LATCbits.LATC3 = 0;
        //__delay_ms(149);
        //LATCbits.LATC3 = 1;     //start at row 1, future 74HC138 or CPLD equiv logic here..
        //__delay_ms(149);          //pin rise settling
         write_uart("Hello, world!");       //send Hello, world! to UART
         uart_xmit(0x0A);                   //send an ASCII character for line feed
         __delay_ms(149);                   //delay 149 milliseconds
         write_uart("From 0xEE.net");       //send From 0xEE.net
         uart_xmit(0x0A);                   //send an ASCII character for line feed
         //uart_xmit('A');
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
         __delay_ms(100);                   //delay 149 milliseconds
     }                                      //... wash, rinse, repeat
    return (EXIT_SUCCESS);
}
