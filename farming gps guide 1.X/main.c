/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com / 0xEE.net / @chasxmd
 * Created on April 4th, 2014
 *
 * Target Device: PIC Click / 18F26K22
 *
 * Project: AG GPS Indicator
 * Using a PIC indicate if you're staying in lat/long "groove" by GPS.
 * *** This is not complete ***
 *
 *
 * Version:
 * 0.1      First build I could prove I had GPS lock
 * 0.2      Output GPS on 232 and position set, no debounce, etc.. abandonded
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

    char    ctxt[120], wstr[120];            //buff NMEA string, working string
    char    str[60], c, latstr, lonstr, setstr;
    char buffer[32] = "none";                        //temp dump
    volatile unsigned int     ping, isrcall, index, reading, new_rx;
    int     ready, gpgga, gprmc, mode;        //gps associated vars
    long long    position_set, position_now;
    //char    *rxdata;
    //volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR

/*
 *  Interrupt Service
 */
void interrupt ISR() {

   if (PIR1bits.RCIF){          // see if interrupt caused by incoming data

        isrcall = 0x01;
        char temp;
        temp = RCREG1;     // read the incoming data
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


   }
}


/*
 * Set up my ports
 */
void init_io(void) {
    // This code before the TRIS setup is for switching the RX2/TX2 to proper pins for the dev board
    INTCONbits.GIE = 0;         //no interruptions please

    LATA = 0x00;

    TRISAbits.TRISA0 = 0; //Onboard LED
    TRISAbits.TRISA1 = 0; //LED
    TRISAbits.TRISA2 = 0; //MCU (ON)
    TRISAbits.TRISA3 = 1; // input
    TRISAbits.TRISA4 = 1; // input
    TRISAbits.TRISA5 = 1; // input
    TRISAbits.TRISA6 = 1; // input
    TRISAbits.TRISA7 = 1; // input


    TRISBbits.TRISB0 = 0; // output
    TRISBbits.TRISB1 = 0; // output
    TRISBbits.TRISB2 = 0; // output
    TRISBbits.TRISB3 = 0; // output
    TRISBbits.TRISB4 = 0; // SCK
    TRISBbits.TRISB5 = 1; // input
    TRISBbits.TRISB6 = 0; // SCK
    TRISBbits.TRISB7 = 1; // input

    LATC = 0x00;

    TRISCbits.TRISC0 = 0; // N/W
    TRISCbits.TRISC1 = 0; // S/E
    TRISCbits.TRISC2 = 0; // GPGGA DETECT
    TRISCbits.TRISC3 = 1; // MODE SELECT (LAT/LONG)
    TRISCbits.TRISC4 = 1; // SET INPUT
    TRISCbits.TRISC5 = 1; // input
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input
    
    ADCON0 = 0b00000000;        //don't need any ADC
    ADCON1 = 0b00000000;        //speed Vref=AVdd, VssRef=AVss

    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
}

void uart_xmit(unsigned int mydata_byte) {  //send a character to the UART
    while(!TXSTA1bits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG1 = mydata_byte;       // transmit data
}

void uart_write(const char *txt)            //sent a multiple characters
{
    while(*txt != 0) uart_xmit(*txt++);     //this send a string to the TX buffer
                                            //one character at a time
}


void serial_init(void)
{
    //9600 8N1

    
    TXSTA1bits.BRGH=1;       // select low speed Baud Rate 
    TXSTA1bits.TX9=0;        // select 8 data bits
    TXSTA1bits.TXEN = 1;     // enable transmit


    RCSTA1bits.SPEN=1;       // serial port is enabled
    RCSTA1bits.RX9=0;        // select 8 data bits
    RCSTA1bits.CREN=1;       // receive enabled

    SPBRG1=25;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH1=0;

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear

    __delay_ms(50);        // give time for voltage levels on board to settle
    uart_write("RESET");         // transmit some data for testing

}


/*
 *  Append a string with a character
 *  append(str, c);
 */


void append(char* s, char c)
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}



/*
 *
 * convert the decminal bits of lat or long to integer
 * send over RS-232 for review
 *
 *
 */



void lat(void){
    str[0] = wstr[17];
    str[1] = wstr[18];
    str[2] = wstr[19];
    str[3] = wstr[20];
    str[4] = wstr[22];
    str[5] = wstr[23];
    str[6] = wstr[24];
    str[7] = wstr[25];

    position_now = atol(str);

    uart_write(str);
    uart_write(" ");

        //check to set position
    if (PORTCbits.RC4 && ready) {
        position_set = position_now;
    }

    if (position_set){
        if (position_now > (position_set + 3)) {
            LATCbits.LATC1 = 1;
        }   else {
            LATCbits.LATC1 = 0;
        }
        
        if (position_now < (position_set - 3)) {
            LATCbits.LATC0 = 1;
        }   else {
            LATCbits.LATC0 = 0;
        }
    }
        sprintf(buffer, "%lld", position_now);
        uart_write(buffer);
        uart_write(" ");
        sprintf(buffer, "%lld", position_set);
        uart_write(buffer);
    uart_write("\r");
}

void lon(void){

}

void determine_mode(void){  //determine lat or long mode
    
}


int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x05; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    __delay_ms(50);         //lets think about life a bit before proceeding..


    ping = 0;
    new_rx = 0;
    isrcall = 0;
    
    init_io();
    serial_init();

    //RCONbits.IPEN = 0;
    PIE1bits.RC1IE = 1;         //Enable RX Interrupt
    INTCONbits.PEIE = 1;        // Enable peripheral interrupt
    INTCONbits.GIE = 1;         // enable global interrupt

    
    ready = 0;

    while (1) {
    isrcall = 0;
    ping = 0;
    if (ready){
        LATCbits.LATC2 = 1;
    }

    LATAbits.LATA2 = 1;         //startup heartbeat LED
    __delay_ms(1);
    LATAbits.LATA2 = 0;

        if (new_rx == 1)            //got our string...
        {
            if (strstr(ctxt, "GPGGA"))
            {
                gpgga = 1;
                strncpy((char*)wstr, (char*)ctxt, sizeof(ctxt));
            }
            new_rx=0;              //finished with GPS string
        }

        if (gpgga) {
            LATAbits.LA1 = 0;
            if(ctxt[42] == '1')     //this is the 43rd bit but we didn't drop the $ into the buffer
            {                       //If "$GPGGA" NMEA message has '1' sign in the 43rd
                                    //position it means that tha GPS receiver has a position fix
                                    //
            ready = 1;              //This is my "locked" variable for future code
            LATAbits.LATA1 = 1;     //LOCK LED
            }
        }

    if (ready) {

        if (mode){

        }
        if (!mode) {
            lat();
        }
        
    }
    __delay_ms(149);
    }
}
