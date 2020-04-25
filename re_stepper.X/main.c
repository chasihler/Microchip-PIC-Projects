/*
 * File:   main.c
 * Author: Charles M Douvier  Contact at: http://iradan.com
 * Core Driver Code by Adam F. of http://www.therengineer.com/
 *
 * Created on April 4th, 2014
 *
 * Target Device:
 * TAUTIC PIC 18F26K22 Dev Board
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
#define MIN_STEPS_LEFT 23
#define MAX_ACCEL_INDEX 6
#define MAX_STEP 945 /* motor can move 945 steps from stop to stop*/

    int     an8_value, an9_value;          //value for a/d
    char    buf[10];            //buff for iota
    long int    fvar;           //long for format math
    long int    tens;           //left of decm
    long int    decm;           //decimal places
    int     tempi;              //to add leadign zeros..
    int     vtxdata;             //volts int for TX
    int     itxdata;

    unsigned short defaultAccelTable[][2] =
{
  {   1750, 3},
  {   1149, 3},
  {  926,   3},
  {  794,   3},
  {  709,   3},
  {  666,   4},
  {  /*629*/450,   4},
};
unsigned int currentStep;
unsigned char currentState;
unsigned char stateMap[] = {0x09, 0x01, 0x07, 0x06, 0x0E, 0x08};
unsigned char serialBuffer[10];
unsigned char serialByteCount;
 static const unsigned char stateCount = 6;

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
    TRISAbits.TRISA6 = 0; // output
    TRISAbits.TRISA7 = 0; // output

    ANSELA = 0x00; // all port A pins are digital I/O

    LATAbits.LATA0 = 0;
    PORTAbits.RA0 = 0;

    TRISBbits.TRISB1 = 0;   //P1C output
    TRISBbits.TRISB2 = 0;  // P1B output
    TRISBbits.TRISB3 = 1;  // AN9    speed control 0-5V
    TRISBbits.TRISB4 = 0;  // P1D output
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 = nc

    ANSELB = 0b00001000;     //RB3, AN9

    TRISCbits.TRISC0 = 0; // output to B2 .. reversed to stoke the right direction
    TRISCbits.TRISC1 = 0; // output to B1
    TRISCbits.TRISC2 = 0; // output to A2
    TRISCbits.TRISC3 = 0; // output to A1
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 0; // output
    TRISCbits.TRISC7 = 0; // output
    ANSELC = 0x00; // all port B pins are digital I/O
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


// All this motor and timer code is from Adam with very minor changes to fit the processor


void t0Delay(unsigned int usec)
{
    unsigned int t0ticks; //16 microsecond timer0 ticks
    unsigned char t0Preload;
    if(usec<16)
    {
        t0ticks=1;
    }
    else
    {
        t0ticks = usec/16;
    }
    t0Preload = 0xFF - t0ticks;
    INTCONbits.TMR0IF=0; //clear the flag
    TMR0 = t0Preload;
    while(INTCONbits.TMR0IF==0)
    {
        ;
    }
}


void zeroMotor()
{
    unsigned int i;
    for (i=0; i < MAX_STEP; i++)
    {
        LATC=stateMap[currentState];
        currentState = (currentState + 5) % stateCount;
        t0Delay(1900);  //2200 in datasheet
    }
    //now the motor is zeroed, reset our state variables.
    currentStep = 0;
    currentState = 0;
    LATC=0; //turn off coils
}



void moveMotor(unsigned int targetStep)
{
    unsigned int dir;
    unsigned int curDelay;
    unsigned char speedIndex=0;
    unsigned char stepsAtThisSpeed=0;
    unsigned int stepsLeft;
    if(currentStep<targetStep)
    {
        dir = 1;
        stepsLeft = targetStep-currentStep;
    }
    else
    {
        dir = -1;
        stepsLeft = currentStep - targetStep;
    }
    while(stepsLeft>0)
    {
        if(stepsLeft<=MIN_STEPS_LEFT)
        {
            //decellerating
            if(stepsAtThisSpeed==0)
            {
                if(speedIndex>0)
                    speedIndex--;
                curDelay=defaultAccelTable[speedIndex][0];
                stepsAtThisSpeed=defaultAccelTable[speedIndex][1];
            }
        }
        else
        {

            //accellerating or steady state
            if(stepsAtThisSpeed==0)
            {
                if(speedIndex<MAX_ACCEL_INDEX)
                {
                    speedIndex++;
                    curDelay=defaultAccelTable[speedIndex][0];
                    stepsAtThisSpeed=defaultAccelTable[speedIndex][1];
                }
                //else we're at steady state - do nothing.
            }
        }

        //write step

        LATC=stateMap[currentState];

        if(dir==1)
        {
            currentState = (currentState + 1) % stateCount;
        }
        else
        {
            currentState = (currentState + 5) % stateCount;
        }
        t0Delay(curDelay);
        if(stepsAtThisSpeed>0)
        {
            stepsAtThisSpeed--;
        }
        stepsLeft--;
        currentStep+=dir;
    }
}

int main(void) {

    init_io();
    serial_init();

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x05; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block

    ADCON0 = 0b00100101;                            //select AN9 and enable
    ADCON1 = 0b00000000;                  //speed Vref=AVdd, VssRef=AVss
    ADCON2 = 0b00111011;                //ledft justified, 20RAD, FRC

    INTCONbits.TMR0IE = 0;

    TMR0=0;

    T0CONbits.T08BIT = 1;
    T0CONbits.T0CS = 0;
    T0CONbits.PSA = 0;
    T0CONbits.T0PS = 0x04;
    INTCONbits.TMR0IF = 0;

        T0CONbits.TMR0ON = 1;

    __delay_us(5);

    currentStep = 0;
    currentState = 0;



    zeroMotor();         
    __delay_ms(149);        //this could be less messy
    __delay_ms(149);
    __delay_ms(149);
    __delay_ms(149);
    __delay_ms(149);
    __delay_ms(149);
    moveMotor(20);
    __delay_ms(149);
    __delay_ms(149);
    __delay_ms(149);
    __delay_ms(149);
    __delay_ms(149);
    __delay_ms(149);

    moveMotor(940);
    moveMotor(5);

    while (1) {

        //PORTAbits.RA0 = 1;      //heart beat
        //__delay_ms(50);
        //PORTAbits.RA0 = 0;
        //__delay_ms(50);

        ADCON0 = 0b00100101;    //select AN9 and enable
        __delay_us(5);
        GO = 1;
        while (GO) continue;    //wait for conversion
        an9_value = ADRESH;     //AN9 value

        fvar = an9_value;
        fvar = fvar * 10749;    //calibration.. change to meet your needs
        fvar = fvar / 256;
        tens = fvar / 100;
        //tens = tens % 10;
        decm = fvar % 100;
        vtxdata = fvar / 20;
        uart_xmit(vtxdata);    // -->RS232

        moveMotor(vtxdata);
        //moveMotor(5); //from sample code
    }
    return (EXIT_SUCCESS);
}
