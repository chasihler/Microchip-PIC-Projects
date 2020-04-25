/*
 * File:   main.c
 * Author: Charles M Douvier  
 * Contact at: http://iradan.com / 0xEE.net / @chasxmd
 * Created on April 4th, 2014
 *
 * Target Device: PIC Click / 18F47J53
 *
 * Project: Mikroe.com GPS2 test
 * Using a PIC Click Dev board and GPS2 click module I am reading the NMEA
 * string and declaring a "lock" by turning on the LED on RA1
 *
 *
 * Version:
 * 0.1      First build I could prove I had GPS lock
 *
 *
 *
 */
#ifndef _XTAL_FREQ                  //blah blah not the right way I don't care
#define _XTAL_FREQ 8000000          //8Mhz FRC internal osc
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#endif

#include <xc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//config bits
#pragma config OSC=INTOSC, WDTEN=OFF, CP0=OFF  //Internal OSC, No WDT and No code protect
#pragma config IESO=OFF, FCMEN=OFF
#pragma config XINST = OFF
#pragma config PLLDIV = 1           //Divide by 1
#pragma config STVREN = ON          //stack overflow/underflow reset enabled
#pragma config XINST = OFF          //Extended instruction set disabled
#pragma config CPUDIV = OSC1        //No CPU system clock divide


#define _XTAL_FREQ 8000000 //defined for delay

    char    ctxt[120];            //buff NMEA string
    volatile unsigned int     ping, isrcall, index, reading, new_rx;
    int     ready, gpgga, gprmc;        //gps associated vars
    //char    *rxdata;
    //volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR

/*
 *  Interrupt Service
 */
void interrupt ISR() {

    if (PIR3bits.RC2IF)          // see if interrupt caused by incoming data
    {
        isrcall = 0x01;
        char temp;
        temp = RCREG2;     // read the incoming data
        if(temp=='$' && new_rx==0)      //if first char of a GPS string..
        {
            index = 0;                  //reset index
            reading = 1;                //from now on go to else if
        }
        else if(reading == 1)           //in middle of GPS sentence
        {
            ctxt[index] = temp;         //load it up
            index++;                    //increment index
            ping = 1;                   //this is for debugging
            if(index > 50)              //thats more than enough data
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
}


/*
 * Set up my ports
 */
void init_io(void) {
    // This code before the TRIS setup is for switching the RX2/TX2 to proper pins for the dev board
    INTCONbits.GIE = 0;         //no interruptions please
    EECON2 = 0x55;
    EECON2 = 0xAA;
    PPSCONbits.IOLOCK = 0;      //turn off PPS write protect

    //PPS Info:
    //RX2DT2R<4:0>: EUSART2 Synchronous/Asynchronous Receive (RX2/DT2) to the Corresponding RPn Pin bits
    //RP22 000[RX2DT2R4 RX2DT2R3 RX2DT2R2 RX2DT2R1 RX2DT2R0]
    //TX2/CK2 6 EUSART2 Asynchronous Transmit/Asynchronous Clock Output
    //RP23 000 [RP6R4 RP6R3 RP6R2 RP6R1 RP6R0]
    //RD5 RX
    //RD6 TX
    //sample:
    // Assign RX2 To Pin RP0
    //MOVLW 0x00 MOVWF RPINR16, BANKED
    // Assign TX2 To Pin RP1
    //MOVLW 0x06 MOVWF RPOR1, BANKED

    RPINR16 = 0x16;     //Pin 22 / RD5
    RPOR23 = 0x06;      //Pin 23 / RD6

    EECON2 = 0x55;
    EECON2 = 0xAA;
    PPSCONbits.IOLOCK = 1;      //write protect PPS

    LATA = 0x00;

    TRISAbits.TRISA0 = 0; //LED1
    TRISAbits.TRISA1 = 0; //LED2
    TRISAbits.TRISA2 = 0; //POWER ON

    TRISBbits.TRISB0 = 0; // HEADER
    TRISBbits.TRISB1 = 0; // HEADER
    TRISBbits.TRISB2 = 0; // RST
    TRISBbits.TRISB3 = 0; // CS
    TRISBbits.TRISB4 = 0; // SCK
    TRISBbits.TRISB5 = 1; // MISO
    
    LATC = 0x00;
    
    TRISCbits.TRISC0 = 0; // HEADER/DEBUGGING PIN
    TRISCbits.TRISC1 = 0; // HEADER/DEBUGGING PIN
    TRISCbits.TRISC2 = 0; // HEADER
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC6 = 1; // PWM (WAKE UP)
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
void uart_xmit(unsigned int mydata_byte) {
    while(!TXSTA2bits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG2 = mydata_byte;       // transmit data
}

void serial_init(void)
{
    //4800 8N1
    // calculate values of SPBRGL and SPBRGH based on the desired baud rate
    //- SPEN bit (RCSTA2<7>) must be set (= 1)
    //- TRIS bit for RPn2/RX2/DT2 = 1
    //- TRIS bit for RPn1/TX2/CK2 = 0 for
    //Asynchronous and Synchronous Master modes

    PIR3bits.RC2IF=0;        // make sure receive interrupt flag is clear

    TXSTA2bits.BRGH=1;       // select low speed Baud Rate (see baud rate calcs below)
    TXSTA2bits.TX9=0;        // select 8 data bits
    TXSTA2bits.TXEN = 1;     // enable transmit
    RCSTA2bits.SPEN=1;       // serial port is enabled
    RCSTA2bits.RX9=0;        // select 8 data bits
    RCSTA2bits.CREN=1;       // receive enabled
    SPBRG2=104;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH2=0;


    __delay_ms(50);        // give time for voltage levels on board to settle
    uart_xmit('R');         // transmit some data for testing
}

int main(void) {
    ping = 0;
    new_rx = 0;
    isrcall = 0;
    ready = 0;
    gpgga = 0;

    init_io();
    serial_init();

    // set up oscillator control register, using internal OSC at 8MHz.
    OSCCONbits.IRCF = 0x07; //set OSCCON IRCF bits to select OSC frequency 8MHz
    OSCCONbits.SCS = 0x02;  //set the SCS bits to select internal oscillator block
    __delay_ms(50);         //lets think about life a bit before proceeding..

    RCONbits.IPEN = 0;
    PIE3bits.RC2IE = 1;         //Enable RX2 Interrupt
    INTCONbits.PEIE = 1;        // Enable peripheral interrupt
    INTCONbits.GIE = 1;         // enable global interrupt


    LATBbits.LATB2 = 0;         //GPS Reset
    __delay_ms(74);
    LATBbits.LATB2 = 1;         //pull out of reset

    LATAbits.LATA0 = 1;         //startup heartbeat LED
    __delay_ms(50);
    LATAbits.LATA0 = 0;

    LATCbits.LATC1 = 1;         //proves my ISR RC1 output works
    __delay_us(35);
    LATCbits.LATC1 = 0;

    LATCbits.LATC2 = 1;         //proves my New Byte RC2 output works
    __delay_us(35);
    LATCbits.LATC2 = 0;

    ADCON0 = 0b00000000;        //don't need any ADC
    ADCON1 = 0b00000000;        //speed Vref=AVdd, VssRef=AVss

    /*      Disable for the time being
     * This is TIMER code, untested.
    INTCONbits.TMR0IE = 0;
    TMR0=0;
    T0CONbits.T08BIT = 1;
    T0CONbits.T0CS = 0;
    T0CONbits.PSA = 0;
    T0CONbits.T0PS = 0x04;
    INTCONbits.TMR0IF = 0;
    T0CONbits.TMR0ON = 1;
    */


    while (!PORTCbits.RC6) {    //in warmup?
        LATAbits.LA2 = 0;
        __delay_ms(3);
        LATAbits.LA2 = 1;      //Turn GPS On
        __delay_ms(10);
    }

    LATAbits.LA2 = 1;          //Ensure GPS is On



    while (1) {
    isrcall = 0;
    ping = 0;
    gpgga = 0;

    if (RCSTA2bits.OERR)
    {
            RCSTA2bits.CREN=0; //DS39964B-page 347
            __delay_us(2);
            RCSTA2bits.CREN=1; //Overrun error (can be cleared by clearing bit, CREN)
    }

        if (new_rx == 1)            //got our string...
        {
            if (strstr(ctxt, "GPGGA"))
            {
            gpgga = 1;
            }
         new_rx=0;              //finished with GPS string
        }
        // uart_xmit('x');    // this was a test, it works.
        if (isrcall) {          //testing bits
        LATAbits.LATA0 = 1;     // $ Detect!
        __delay_us(10);         //
        LATAbits.LATA0 = 0;
        }

        if (gpgga) {
            LATCbits.LATC2 = 1;     //GPGGA detect
            __delay_us(1);          //
            LATCbits.LATC2 = 0;
            if(ctxt[42] == '1')     //this is the 43rd bit but we didn't drop the $ into the buffer
            {                       //If "$GPGGA" NMEA message has '1' sign in the 43rd
                                    //position it means that tha GPS receiver has a position fix
                                    //
            ready = 1;              //This is my "locked" variable for future code
            LATAbits.LATA1 = 1;     //LOCK LED
            LATCbits.LATC1 = 1;     //DEBUGGING
            __delay_us(1);          //
            LATCbits.LATC1 = 0;
            }
            else if (ctxt[42] != '1')     //this is the 43rd bit but we didn't drop the $ into the buffer
            {                       
            ready = 0;              //
            LATAbits.LATA1 = 0;     //LOCK LED
            }
        }
    }
}
