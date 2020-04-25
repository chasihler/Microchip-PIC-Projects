/*
 * File:   main.c
 * Author: Charles Douvier
 *
 * Created on December 21, 2013, 10:24 PM
 * Robot Test
 * Version 0.1  Motor Commands
 */

#include <htc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
__CONFIG (CLKOUTEN_OFF & FCMEN_ON & IESO_OFF & BOREN_OFF & CP_OFF & MCLRE_ON & PWRTE_ON & WDTE_OFF & FOSC_INTOSC);//XT
__CONFIG (LVP_OFF & BOREN_ON & STVREN_ON & WRT_OFF);
#define _XTAL_FREQ 4000000
#define watersensor RA5
void usrt_init (void)
{
TRISA5=1; // in
TRISA1=1; // in
TRISA0=0; // out
TRISA4=0; //out
APFCON=0x42;   //RA0/1 serial
IRCF3=1;//4MHz clock speed
IRCF2=1;
IRCF1=0;
IRCF0=1;
//TXSTA //CHECK THE DATA SHEET FOR TXSTA
CSRC=0; //Clock Source Select bit not used in async.
TX9=0; // 9-bit Transmit Enable bit
TXEN=1; // Transmit Enable bit(1)
SYNC=0; //1 = Synchronous mode 0 = Asynchronous mode
SENDB=0; //Send Break Character bit 1 = Send Sync Break on next transmission (cleared by hardware upon completion)
//0 = Sync Break transmission completed
BRGH=0; //High Baud Rate Select bit 0= low
TRMT=1; //Transmit Shift Register Status bit 1 = TSR empty 0 = TSR full
TX9D=0; //Ninth bit of Transmit Data


//RCSTA //SEE THE DATA SHEET FOR RCSTA
SPEN=1; //Serial Port Enable bit
RX9=0; // 8bit
SREN=0; // Single Receive Enable bit
CREN=1; //1 = Enables receiver
ADDEN=0; //Not Used.
FERR=0; //Framing Error bit
OERR=0; // Overrun Error bit
RX9D=0; // Ninth bit of Received Data


BRGH=1; // low baud rate
SPBRGL=0b00011001; //.25 baud rate 9600
SPBRGH=0b00000000;

}//

void M0_FWD_FULL(void){
    TXREG=0x88;     //M0 foward
    while(!TRMT)    // until empty.
    {
    }
    TXREG=0x4F;     // half speed
    while(!TRMT)    // until empty.
    {
    }
}

void M0_FWD_HALF(void){
    TXREG=0x88;     //M0 foward
    while(!TRMT)    // until empty.
    {
    }
    TXREG=0x3F;     // half speed
    while(!TRMT)    // until empty.
    {
    }
}

void M0_FWD_LOW(void){
    TXREG=0x88;     //M0 foward
    while(!TRMT)    // until empty.
    {
    }
    TXREG=0x2F;     // half speed
    while(!TRMT)    // until empty.
    {
    }
}

void M0_STOP(void){
    TXREG=0x86;     //M0 STOP
    while(!TRMT)    // until empty.
    {
    }

}

void M0_REV_HALF(void){
    TXREG=0x8A;     //M0 reverse
    while(!TRMT)    // until empty.
    {
    }
    TXREG=0x3F;     // half speed
    while(!TRMT)    // until empty.
    {
    }
}

void M1_FWD_FULL(void){
    TXREG=0x8C;     //M1 foward
    while(!TRMT)    // until empty.
    {
    }
    TXREG=0x4F;     // half speed
    while(!TRMT)    // until empty.
    {
    }
}

void M1_FWD_HALF(void){
    TXREG=0x8C;     //M1 foward
    while(!TRMT)    // until empty.
    {
    }
    TXREG=0x3F;     // half speed
    while(!TRMT)    // until empty.
    {
    }
}

void M1_FWD_LOW(void){
    TXREG=0x8C;     //M1 foward
    while(!TRMT)    // until empty.
    {
    }
    TXREG=0x2F;     // half speed
    while(!TRMT)    // until empty.
    {
    }
}

void M1_STOP(void){
    TXREG=0x87;     //M1 STOP
    while(!TRMT)    // until empty.
    {
    }

}

void M1_REV_HALF(void){
    TXREG=0x8E;     //M1 reverse
    while(!TRMT)    // until empty.
    {
    }
    TXREG=0x3F;     // half speed
    while(!TRMT)    // until empty.
    {
    }
}

/*
 *
 */
int main(int argc, char** argv) {
usrt_init();

TXREG=0xAA;     //initialize motor driver
while(!TRMT)    // until empty.
{
}

M0_FWD_HALF();  //M0 drive forward
M1_FWD_HALF();  //M1 drive forward

while(1) //loop forever
{
}

    return (EXIT_SUCCESS);
}