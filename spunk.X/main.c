/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on February 8, 2014, 11:39 AM
 *
 * Target Device:
 * 18F14K22 on Tautic 20 pin dev board
 *
 * Project: MIDI Slave
 *
 *
 * Version:
 * 0.1  Configuration, 31.25Kbaud TX&RX
 * 0.2  drive test
 * 0.3  updated reverse and edge events
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

/*
 * Variables
 */
struct {
    unsigned int leftcliff :1;
    unsigned int rightcliff :1;
    unsigned int waiting :1;
    unsigned int puppy_mode :1;
} SENSORbits;
struct {
    unsigned int rev :1;
    unsigned int right_rev :1;
    unsigned int left_rev :1;
    unsigned int fwd :1;
    unsigned int right_fwd :1;
    unsigned int left_fwd :1;
    unsigned int estop :1;
    unsigned int stop :1;
    unsigned int delayonnewcycle :1;	//add delay to keep bot in current drive mode
    unsigned int nextcycle_stop :1;		//coast on next cycle (test to slow down bot)
    unsigned int nextcycle_reverse:1;	//second step reverse in backup cycle.
    unsigned int nextcycle_turn:1;	//second step turn in backup cycle.
} DRIVEbits;

    int     timer1count, count;        //count of timer1 roll overs, and counter
    int     tempi;              //temp
    int     pulse_time, i;
    int     itxdata;            //int RS232 tx data
    char    buf[10];            //buff for iota
    volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR

/*
 *  Functions
 */

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

void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
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
    TRISAbits.TRISA0 = 0; // SONAR PULSE
    TRISAbits.TRISA1 = 1; // ECHO
    TRISAbits.TRISA2 = 0; // STATUS LED
    TRISAbits.TRISA4 = 1; // MOTOR DRIVER ERROR IN
    TRISAbits.TRISA5 = 0; // MOTOR DRIVER /RESET

    ANSEL = 0x00;         // no A/D
    ANSELH = 0x00;

    TRISBbits.TRISB4 = 0; // RB4 = nc
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 MOTOR DRIVER TX

    TRISCbits.TRISC0 = 1; // left cliff sensor in
    TRISCbits.TRISC1 = 1; // right cliff sensor
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input

}

void checkleftcliffsensor(void)
{
    SENSORbits.leftcliff = 0;
    if (PORTCbits.RC0)
        SENSORbits.leftcliff = 1;
}
void checkrightcliffsensor(void)
{
    SENSORbits.rightcliff = 0;
    if (PORTCbits.RC1)
        SENSORbits.rightcliff = 1;
}

void measure_sonar(void)
{
    //Test distance = (high level time×velocity of sound (340M/S) / 2

    while (!PORTAbits.RA1);    //wait for it..

    TMR0=0;
    TMR0L=0;
    TMR0H=0;
    INTCONbits.T0IF =0;
    T0CONbits.PSA = 0;          //Turn on Timer 8 bit w/ 256 prescale;
    T0CONbits.T0PS = 2;
    T0CONbits.T0CS = 0;
    T0CONbits.T08BIT = 0;
    T0CONbits.TMR0ON = 1;

    while (PORTAbits.RA1 && !INTCONbits.T0IF);

    pulse_time = TMR0;
    T0CONbits.TMR0ON = 0;
    
    if (INTCONbits.T0IF)
        LATAbits.LATA2=1;
    
    INTCONbits.T0IF=0;
}

void pulse_sonar (void)
{
    //send 10us pulse to activate sonar

    LATAbits.LATA0 = 1;
    __delay_ms(12);
    LATAbits.LATA0 = 0;
}

void motor_controller_reset(void)
{
    LATAbits.LATA5 = 0; //motor driver reset
    __delay_ms(50);
    LATAbits.LATA5 = 1;
    __delay_ms(140);
}

// All These motor commands were pulled off my Sumo bot and 
// "ported" from ASM to C.

/*-------------------------------------------------------
*Initialize Motor Controller
*Polou Serial Controller requires 'AA' before any other
*signal to detect baud/initialize
*
*-------------------------------------------------------
*/

void motor_rev_timeron (void)
{
    if (!timer1count)
    {
        PIE1bits.TMR1IE=0;
        TMR1=0;
        PIR1bits.TMR1IF=0;
        T1CON=0x39; //39; //timer on prescale 8
        timer1count++;
    }
    else
        if (PIR1bits.TMR1IF)
        {
            TMR1=0;
            PIR1bits.TMR1IF=0;
            T1CON=0x39; //39; //timer on prescale 8
            timer1count++;
        }
}



void motor_rev_timeroff(void)
{
    TMR1=0;
    T1CON=0x00;

}

void stop_and_wait (void)
{
    count++;
    __delay_ms(1);
}


void init_motor_controller(void)
{

    uart_xmit(0xAA);     //send AA to controller
    __delay_ms(50);
    //Change PWM
    //0x84, 0x01, 0x02, 0x55, 0x2A = 7.8KHz PWM
    uart_xmit(0x84);
    uart_xmit(0x01);
    uart_xmit(0x02);
    uart_xmit(0x55);
    uart_xmit(0x2A);
    __delay_ms(50);
}

/*
*;-----------------------------------------------------
*;Motor Commands
*;
*;M0 = Left Motor
*;M1 = Right Motor
*;
*;Command 0x88: Motor M0 Forward
*;Compact protocol: 0x88, motor speed
*;
*;Command 0x8A: Motor M0 Reverse
*;Compact protocol: 0x8A, motor speed
*;
*;Motor M1 Commands;
*;
*;Command 0x8C: Motor M1 Forward/140
*;Compact protocol: 0x8C, motor speed
*;
*;
*;
*;Command 0x8E: Motor M1 Reverse
*;Compact protocol: 0x8E, motor speed
*;
 */

void motor_stop(void)
{
    uart_xmit(0x86); //right
    uart_xmit(0x87); //left
}
/*
*;-------------------------------------------------------
*;Right Forward Quarter--ISH
*;Polou Serial Controller requires 'AA' before any other
*;signal to detect baud/initialize
*;
*;-------------------------------------------------------
*/
void motor_right_fwd_quarter(void)		//M0 0x88-0x__
{
    uart_xmit(0x88);
    uart_xmit(0x30);

}

void motor_right_rev_quarter(void)
{
    uart_xmit(0x8A);
    uart_xmit(0x30);
    motor_rev_timeron();
}

void motor_left_fwd_quarter(void)		//M1 0x8C-0x7F
{
    uart_xmit(0x8E);
    uart_xmit(0x30);

}

void motor_left_rev_quarter(void)
{
    uart_xmit(0x8C);
    uart_xmit(0x30);
    motor_rev_timeron();
}

void clear_drive(void)
{
    DRIVEbits.fwd=0;
    DRIVEbits.rev=0;
    DRIVEbits.left_fwd=0;
    DRIVEbits.left_rev=0;
    DRIVEbits.right_fwd=0;
    DRIVEbits.right_rev=0;
    DRIVEbits.stop=0;
    //  DRIVEbits.estop=0;
}

void determine_drive_by_cliff(void)		//
{

   if (SENSORbits.leftcliff)
   {
       	DRIVEbits.left_rev=1;
	DRIVEbits.delayonnewcycle=1;
	DRIVEbits.nextcycle_reverse=0;
   }
   if (SENSORbits.rightcliff || DRIVEbits.nextcycle_turn)
   {
       DRIVEbits.right_rev=1;
	DRIVEbits.delayonnewcycle=1;
	DRIVEbits.nextcycle_reverse=0;
        DRIVEbits.nextcycle_turn=0;
   }
   if (SENSORbits.leftcliff && SENSORbits.rightcliff && !DRIVEbits.estop)
   {
       	DRIVEbits.rev=1;
	DRIVEbits.delayonnewcycle=1;
	DRIVEbits.nextcycle_turn=1;
   }
}


void determine_drive_by_sonar(void)
{
    if (pulse_time >= 0xAF && !DRIVEbits.estop)
    {
        SENSORbits.waiting=0;
        DRIVEbits.fwd=1;
    }
    if (pulse_time < 0xAF && pulse_time > 0x40)
    {
        SENSORbits.waiting=1;
        DRIVEbits.stop=1;
    }

    if (pulse_time < 0x40 && !DRIVEbits.estop && pulse_time > 0x01)
    {
        SENSORbits.waiting=0;
        DRIVEbits.rev=1;
    }
    if (pulse_time > 0x7F)
    {
            //LATAbits.LATA2=1;
            //__delay_us(20);
            //LATAbits.LATA2=0;
            //__delay_us(20);
        i++;
    }
}

void set_drive(void)		//replace
{
   if  (DRIVEbits.estop)
       motor_stop();
   else if (DRIVEbits.left_rev && !DRIVEbits.rev && !DRIVEbits.nextcycle_reverse)
       motor_left_rev_quarter();
   else if (DRIVEbits.right_rev && !DRIVEbits.rev && !DRIVEbits.nextcycle_reverse)
       motor_right_rev_quarter();
   else if (DRIVEbits.rev)
   {
       motor_right_rev_quarter();
       motor_left_rev_quarter();
   }
   else if (DRIVEbits.stop)
       motor_stop();
   else if (DRIVEbits.left_fwd && !DRIVEbits.nextcycle_reverse)
       {
           motor_right_rev_quarter();
           motor_left_fwd_quarter();
       }
       else if (DRIVEbits.right_fwd)
       {
           motor_right_fwd_quarter();
           motor_left_rev_quarter();
       }
   else if (DRIVEbits.fwd && !DRIVEbits.nextcycle_reverse)
       {
           motor_right_fwd_quarter();	//drive forward
           motor_left_fwd_quarter();
       }

   else if (DRIVEbits.nextcycle_turn)
	{
	       motor_left_rev_quarter();
	}

   else if (DRIVEbits.nextcycle_reverse)
	{
	motor_right_rev_quarter();
        motor_left_rev_quarter();
        DRIVEbits.nextcycle_reverse=0;
	}
}


int main(void) {

    init_io();

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x05; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block

    serial_init();

    LATAbits.LATA2=0;

    motor_controller_reset();
    init_motor_controller();


    while (1)
    {
        //if delayed for extra drive time from last command...
        if (DRIVEbits.delayonnewcycle)	//Insert into upper main loop
        {
            i=0x5;
            while (i>1)
            {
                __delay_ms(100);
                i=i-1;
            }
            DRIVEbits.delayonnewcycle=0;
        }

        clear_drive();
        checkleftcliffsensor();
        checkrightcliffsensor();
        determine_drive_by_cliff();

        // check sonar distance
        pulse_sonar();
        __delay_us(10);
        measure_sonar();
        determine_drive_by_sonar();

        if (SENSORbits.waiting)
        {
            stop_and_wait();
            __delay_ms(10);
            if (count > 0xE9)
            {
            DRIVEbits.left_fwd=1;
            LATAbits.LATA2=1;
            DRIVEbits.stop=0;
            }
            if (count > 0xFC)
            {
                count = 0;
                DRIVEbits.stop=1;
            }
        }
        else
        {
            count = 0;
            LATAbits.LATA2=0;
        }


        set_drive();


        if (!timer1count)
        {
            if (DRIVEbits.right_rev)
                motor_rev_timeron();
            else if (DRIVEbits.rev)
                motor_rev_timeron();
            else if (DRIVEbits.left_rev)
                motor_rev_timeron();
            else
              motor_rev_timeroff();
        }
        else
            if (timer1count > 0x4F)
            {
            DRIVEbits.estop = 1;
            }
        if (DRIVEbits.estop)
        {
            __delay_ms(149);
            LATAbits.LATA2=1;
            __delay_ms(149);
            LATAbits.LATA2=0;
        }
     DRIVEbits.rev=0;
    }
    return (EXIT_SUCCESS);
}
