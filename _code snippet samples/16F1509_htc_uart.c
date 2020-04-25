/* 
 * File:   newmain.c
 * Author: CHAS
 *
 * Created on December 10, 2013, 8:38 PM
 */

#include <htc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

__CONFIG (CLKOUTEN_OFF & FCMEN_ON & IESO_OFF & BOREN_OFF & CP_OFF & MCLRE_OFF & PWRTE_ON & WDTE_OFF & FOSC_INTOSC);//XT
__CONFIG (LVP_ON & LPBOR_OFF & BOREN_ON & STVREN_ON & WRT_OFF);

#define _XTAL_FREQ 4000000
#define button RC1

void usrt_init()
{
    TRISB5=1; // in
	TRISC1=1; // in
	TRISB7=0; // out
    ANSELB=0; // analog off.

    //TXSTA    //CHECK THE DATA SHEET FOR TXSTA
    CSRC=0; //Clock Source Select bit not used in async.
    TX9=0; // 9-bit Transmit Enable bit
    TXEN=1; // Transmit Enable bit(1)
    SYNC=0; //1 = Synchronous mode 0 = Asynchronous mode
    SENDB=0; //Send Break Character bit 1 = Send Sync Break on next transmission (cleared by hardware upon completion)
    //0 = Sync Break transmission completed
    BRGH=0; //High Baud Rate Select bit 0= low
    TRMT=1; //Transmit Shift Register Status bit 1 = TSR empty 0 = TSR full
    TX9D=0; //Ninth bit of Transmit Data


    //RCSTA    //SEE THE DATA SHEET FOR RCSTA
    SPEN=1; //Serial Port Enable bit
    RX9=0;  // 8bit
    SREN=0; // Single Receive Enable bit
    CREN=1; //1 = Enables receiver
    ADDEN=0; //Not Used.
    FERR=0; //Framing Error bit
    OERR=0; // Overrun Error bit
    RX9D=0; // Ninth bit of Received Data


    BRGH=0;            //  low baud rate
    SPBRGL=0b11001111;      //baud rate 300
    SPBRGH=0b00000000;

}//
main()
{
    usrt_init();
    while(1); //loop
     {
      if (!button) // if pressed the button.
      {
       TXEN=1;
        TXREG = 0b11111100; //sends a number
            while(!TRMT); // until empty.
      }
     }
}
