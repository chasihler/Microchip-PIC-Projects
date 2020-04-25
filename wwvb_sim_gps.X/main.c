/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com / 0xEE.net / @chasxmd
 * Created on April 4th, 2014
 *
 * Target Device: 18F26K22
 *
 * Project: GPS --> WWVB simulator
 *
 * This version uses the GPRMC block
 * This is a limitation because GPRMC doesn't pass seconds
 * The time passed will always be up to 60 seconds off
 * I have to deterine DDMMYY --> Day of year, year
 * No simple leap year info in GPS :(
 *
 * TODO
 * Determine GPS lock and output to LED
 * Consider re-writing how I am writing my time code
 *
 * I'll re-write this sometime with a better (more expensive) GPS module.
 *
 * Version:
 * 0.1      First build I could prove I had GPS lock
 * 0.2      Output time/date on 232
 *
 */
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 8000000 //4Mhz FRC internal osc
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

#define _XTAL_FREQ 8000000 //defined for delay

    char    ctxt[120], wstr[120];            //buff NMEA string, working string
    char    str1[20], str2[20], c, latstr, lonstr, setstr, doych[8];
    char    hourch[3], minch[3], secch[3], daych[3], monthch[3], yearch[3];
    char buffer[32] = "none";                        //temp dump
    volatile unsigned int     ping, isrcall, index, reading, new_rx;
    int     ready, gpgga, gprmc, mode;        //gps associated vars
    int     leap_year, dayint, monthint, yearint, year4int, secondint, minuteint, hourint;
    long    doy;
    int     min_40, min_20, min_10, min_8, min_4, min_2, min_1;
    int     hour_20, hour_10, hour_8, hour_4, hour_2, hour_1;
    int     doy_200, doy_100, doy_80, doy_40, doy_20, doy_10;
    int     doy_8, doy_4, doy_2, doy_1, leapint;
    int     year_80, year_40, year_20, year_10, year_8, year_4, year_2, year_1;

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
            if(index > 63)              //thats more than enough data
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
    TRISBbits.TRISB2 = 0; // PWM1B
    TRISBbits.TRISB3 = 0; // output
    TRISBbits.TRISB4 = 0; // SCK
    TRISBbits.TRISB5 = 0; // PWM1C
    TRISBbits.TRISB6 = 0; // SCK
    TRISBbits.TRISB7 = 1; // input

    LATC = 0x00;

    TRISCbits.TRISC0 = 0; // N/W
    TRISCbits.TRISC1 = 0; // S/E
    TRISCbits.TRISC2 = 0; // PWM1A
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

    SPBRG1=51;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH1=0;

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear

    __delay_ms(50);        // give time for voltage levels on board to settle
    uart_write("RESET");         // transmit some data for testing

}


void pwm_init(){
//
//Take care if setting up the PWM pins (DISBALE A/D, etc)
//
//Select the 8-bit TimerX resource, (Timer2,Timer4 or Timer6) to be used for PWM generation
//by setting the CxTSEL<1:0> bits in the CCPTMRSx register.(1)
//
//Load the PRx register for the selected TimerX with the PWM period value.
//
//Configure the CCP module for the PWM mode by loading the CCPxCON register with the
//appropriate values.
//
//Load the CCPRxL register and the DCxB<1:0> bits of the CCPxCON register, with the PWM
//duty cycle value.
//

//    CCPR1L = 0x120;
    CCPR1Lbits.CCPR1L = 0x01;       //PWM duty cycle
    //CCPR2Lbits.CCPR2L = 0xCE;       //PWM duty cycle
    //CCPR3Lbits.CCPR3L = 0xCE;       //PWM duty cycle
    PR2 = 0x01;                     //Timer 2 Prescale
    //PR4 = 0xFF;                     //Timer 4 Prescale
    //PR6 = 0xFF;                     //Timer 6 Prescale
    CCPTMRS0bits.C1TSEL = 0x00;     //PWM1 TMR2 Selection
    //CCPTMRS0bits.C2TSEL = 0x01;     //PWM2 TMR4 Selection
    //CCPTMRS0bits.C3TSEL = 0x02;     //PWM3 TMR6 Selection
    CCP1CONbits.P1M = 0x00;         //single output mode
    CCP1CONbits.DC1B = 0x00;
    PWM1CONbits.P1RSEN = 0;
    PWM1CONbits.P1DC = 0x00;    //dead band delay
    ECCP1ASbits.CCP1AS = 0x00;
    ECCP1ASbits.CCP1ASE = 0;    //Auto-shutdown off
    CCP1CONbits.CCP1M = 0x0C;   //PWM Mode
    //CCP2CONbits.CCP2M = 0x0C;   //PWM Mode
    //CCP3CONbits.CCP3M = 0x0C;   //PWM Mode
    PSTR1CONbits.STR1A = 1;
    PSTR1CONbits.STR1B = 1;


    //T2CONbits.T2OUTPS = 0x0F;      //post scaler
    T2CONbits.T2CKPS = 2;       //16x prescaler
    //T4CONbits.T4CKPS = 2;
    //T6CONbits.T6CKPS = 2;
    T2CONbits.TMR2ON = 1;       //Turn the Timers On...
    //T4CONbits.TMR4ON = 1;
    //T6CONbits.TMR6ON = 1;



}


/*
 *  Append a string with a character
 *  append(str, c);
 */


//taken from http://stackoverflow.com/questions/19377396/c-get-day-of-year-from-date

int yisleap(int year)
{
    return (year % 4 == 0 && year % 100 != 0);
}

int get_yday(int mon, int day, int year)
//use: int day = get_yday(1, 31, 2013);
{
    static const int days[2][13] = {
        {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
    };
    int leap = yisleap(year);
    leapint = leap;

    return days[leap][mon] + day;
}

void append(char* s, char c)
{
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

void marker(void){
    //send a marker frame - 800ms off, 200 ms on
    LATBbits.LATB0 = 0;
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);

    LATBbits.LATB0 = 1;
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    LATBbits.LATB0 = 0;
}

void one(void){
    //send a one - 500 ms off, 500 ms on
    LATBbits.LATB0 = 0;
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    LATBbits.LATB0 = 1;
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
}

void zero(void){
    //send a zero - 200ms off, 800ms on
    LATBbits.LATB0 = 0;
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    LATBbits.LATB0 = 1;
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
    __delay_ms(50);
}



void sendtime(void){

    /*
    *;0
    xCALL    MARKER                      ;MARKER FRAME REFERENCE BIT
    xCALL    ONE                         ;40min
    xCALL    ZERO                        ;20min
    xCALL    ZERO                        ;10min
    xCALL    ZERO                        ;Reserved
    xCALL    ZERO                        ;8mins
    xCALL    ZERO                        ;4mins
    xCALL    ONE                         ;2mins
    xCALL    ZERO                        ;1mins
    xCALL    MARKER                      ;MARKER 1
     */

    marker();                       //marker, frame reference

    if (minuteint >= 40){           //40min
        minuteint = minuteint - 40;
        one();
    }else {
        zero();
    }
    if (minuteint >= 20){           //20min
        minuteint = minuteint - 20;
        one();
    }else {
        zero();
    }
    if (minuteint >= 10){           //10min
        minuteint = minuteint - 10;
        one();
    }else {
        zero();
    }    
    zero();                        //reserved (zero)
    if (minuteint >= 8){           //8min
        minuteint = minuteint - 8;
        one();
    }else {
        zero();
    }    
    if (minuteint >= 4){           //4min
        minuteint = minuteint - 4;
        one();
    }else {
        zero();
    }    
    if (minuteint >= 2){           //2min
        minuteint = minuteint - 2;
        one();
    }else {
        zero();
    }    
    if (minuteint >= 1){           //1min
        minuteint = minuteint - 1;
        one();
    }else {
        zero();
    }    
    marker();                       //marker 1
    
    /*
    ;10
    xCALL    ZERO                        ;RESERVED
    xCALL    ZERO                        ;RESERVED
    xCALL    ZERO                        ;20hours
    CALL    ZERO                        ;10hours
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;8hours
    CALL    ONE                         ;4hours
    CALL    ONE                         ;2hours
    CALL    ZERO                        ;1hour
    CALL    MARKER                      ;MARKER 2     
     */
    zero();                         //reserved
    zero();                         //reserved
    if (hourint >= 20){           //20 hours
        hourint = hourint - 20;
        one();
    }else {
        zero();
    }       
    if (hourint >= 10){           //10 hours
        hourint = hourint - 10;
        one();
    }else {
        zero();
    }  
    zero();                         //reserved    
    if (hourint >= 8){           //8 hours
        hourint = hourint - 8;
        one();
    }else {
        zero();
    }  
    if (hourint >= 4){           //4 hours
        hourint = hourint - 4;
        one();
    }else {
        zero();
    }  
    if (hourint >= 2){           //2 hours
        hourint = hourint - 2;
        one();
    }else {
        zero();
    }  
    if (hourint >= 1){           //1 hours
        hourint = hourint - 1;
        one();
    }else {
        zero();
    }  
    marker();                       //marker 2    
    /*
     * 
;20
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;200 day of year
    CALL    ONE                         ;100 day of year
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;80 day of year
    CALL    ONE                         ;40 day of year
    CALL    ZERO                        ;20 day of year
    CALL    ONE                         ;10 day of year
    CALL    MARKER                      ;MARKER 3
     */
    zero();                         //reserved
    zero();                         //reserved
    if (doy >= 200){           //200th day
        doy = doy - 200;
        one();
    }else {
        zero();
    }           
    if (doy >= 100){           //100th day
        doy = doy - 100;
        one();
    }else {
        zero();
    }   
    zero();                         //reserved
    if (doy >= 80){           //80th day
        doy = doy - 80;
        one();
    }else {
        zero();
    }      
    if (doy >= 40){           //40th day
        doy = doy - 40;
        one();
    }else {
        zero();
    }   
    if (doy >= 20){           //20th day
        doy = doy - 20;
        one();
    }else {
        zero();
    } 
    if (doy >= 10){           //10th day
        doy = doy - 10;
        one();
    }else {
        zero();
    }     
    marker();                       //marker 3

/*
 ;30
    CALL    ONE                         ;8 day of year
    CALL    ZERO                        ;4 day of year
    CALL    ZERO                        ;2 day of year
    CALL    ZERO                        ;1 day of year
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;UTI Sign +
    CALL    ZERO                        ;UTI Sign -
    CALL    ZERO                        ;UTI Sign +
    CALL    MARKER                      ;MARKER 4
 */

     if (doy >= 8){           //8th day
        doy = doy - 8;
        one();
    }else {
        zero();
    }
    if (doy >= 4){           //4th day
        doy = doy - 4;
        one();
    }else {
        zero();
    }
    if (doy >= 2){           //2nd day
        doy = doy - 2;
        one();
    }else {
        zero();
    }
    if (doy >= 1){           //1st day
        doy = doy - 1;
        one();
    }else {
        zero();
    }
    zero();                         //reserved
    zero();                         //reserved
    zero();                         //reserved
    zero();                         //reserved
    zero();                         //reserved
    marker();                       //marker 4

    /*
   ;40
    CALL    ZERO                        ;UTI Corr 0.8s
    CALL    ZERO                        ;UTI Corr 0.4s
    CALL    ZERO                        ;UTI Corr 0.2s
    CALL    ZERO                        ;UTI Corr 0.1s
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;80 year yearint
    CALL    ZERO                        ;40 year
    CALL    ZERO                        ;20 year
    CALL    ONE                         ;10 year
    CALL    MARKER                      ;MARKER 5
     */

    zero();                         //reserved
    zero();                         //reserved
    zero();                         //reserved
    zero();                         //reserved
    zero();                         //reserved

    if (yearint >= 80){           //80th year
        yearint = yearint - 80;
        one();
    }else {
        zero();
    }
    if (yearint >= 40){           //40th year
        yearint = yearint - 40;
        one();
    }else {
        zero();
    }
    if (yearint >= 20){           //20th year
        yearint = yearint - 20;
        one();
    }else {
        zero();
    }
    if (yearint >= 10){           //10th year
        yearint = yearint - 10;
        one();
    }else {
        zero();
    }
    marker();                       //marker 5

/*
     *
;50
    CALL    ZERO                        ;8 year
    CALL    ONE                         ;4 year
    CALL    ONE                         ;2 year
    CALL    ZERO                        ;1 year
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;LEAP YEAR TRUE
    CALL    ZERO                        ;LEAP SEC WARN
    CALL    ONE                        ;DST
    CALL    ONE                        ;DST
    CALL    MARKER                        ;FRAME BIT P0
    *
*/

    if (yearint >= 8){           //8th year
        yearint = yearint - 8;
        one();
    }else {
        zero();
    }
    if (yearint >= 4){           //4th day
        yearint = yearint - 4;
        one();
    }else {
        zero();
    }
    if (yearint >= 2){           //2nd day
        yearint = yearint - 2;
        one();
    }else {
        zero();
    }
    if (yearint >= 1){           //1st day
        yearint = yearint - 1;
        one();
    }else {
        zero();
    }
    zero();                         //reserved

                                     //leap year
    if (leapint){
        one();
    }else {
        zero();
    }
    zero();
    zero();                         //leap sec warn
    zero();                         //dst??
    zero();                         //dst
    marker();                       //frame bit P0
}

void gettime(void){
    
    hourch[0] = wstr[6];      //HHMMSS
    hourch[1] = wstr[7];

    minch[0] = wstr[8];
    minch[1] = wstr[9];

    secch[0] = wstr[10];
    secch[1] = wstr[11];

    daych[0] = wstr[56];     //DAY1  DDMMYY
    daych[1] = wstr[57];     //DAY2

    monthch[0] = wstr[58];     //MONTH1
    monthch[1] = wstr[59];     //MONTH2

    yearch[0] = wstr[60];     //YEAR1
    yearch[1] = wstr[61];     //YEAR2

    uart_write(wstr);

    uart_write(" ");

   hourint = atoi(hourch);
   minuteint = atoi(minch);
   secondint= atoi(secch);

   dayint = atoi(daych);
   monthint = atoi(monthch);
   yearint = atoi(yearch);

   year4int = yearint + 2000;

   doy = get_yday(monthint, dayint, yearint);

   ltoa(doych,doy,10);

   uart_write(doych);

    uart_write("\r");
}

void lon(void){

}

void determine_mode(void){  //determine lat or long mode

}


int main(void) {

    // set up oscillator control register, using internal OSC at 8MHz.
    OSCCONbits.IRCF = 0x06; //set OSCCON IRCF bits to select OSC frequency 8MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    __delay_ms(70);         //lets think about life a bit before proceeding..
    __delay_ms(70);
    __delay_ms(70);

    ping = 0;
    new_rx = 0;
    isrcall = 0;

    init_io();
    serial_init();

    //RCONbits.IPEN = 0;
    PIE1bits.RC1IE = 1;         //Enable RX Interrupt
    INTCONbits.PEIE = 1;        // Enable peripheral interrupt
    INTCONbits.GIE = 1;         // enable global interrupt

    pwm_init();

    ready = 0;

    while (1) {

    LATCbits.LATC0 = 1;
    __delay_ms(74);
    isrcall = 0;
    ping = 0;
    if (ready){
        LATCbits.LATC2 = 1;
    }

        if (new_rx == 1)            //got our string...
        {
            if (strstr(ctxt, "GPRMC"))  // && ready
            {
                gprmc = 1;
                strncpy((char*)wstr, (char*)ctxt, sizeof(ctxt));
                gettime();
            }
            new_rx=0;              //finished with GPS string
        }


    if (gprmc){
        sendtime();
    }
    gpgga = 0;
    gprmc = 0;
    LATCbits.LATC0 = 0;
    __delay_ms(74);
    }
}
