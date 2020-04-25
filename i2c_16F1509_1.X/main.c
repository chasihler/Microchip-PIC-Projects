/* 
 * File:   main.c
 * Author: Chas
 *
 * Created on January 9, 2014, 7:34 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <plib.h>
#include <i2c.h>
#include <xc.h>                            //PIC hardware mapping

//config bits
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=ON, CP=OFF, BOREN=ON, CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF

// Definitions
#define _XTAL_FREQ  16000000        // this is used by the __delay_ms(xx) and __delay_us(xx) functions

unsigned char I2C_send[20] = "MICROCHIP:MASTER";
unsigned char I2C_recv[21];

                                            //main
void main(void) {
 
      // set up oscillator control register
//    OSCCONbits.IRCF = 0x0F; //set OSCCON IRCF bits to select OSC frequency=16Mhz
//    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    unsigned char sync_mode=0, slew=0, add1,status,temp,w,length=0;

    for(w=0;w<20;w++)
    I2C_recv[w]=0;
    CloseI2C();             //close i2c if was operating earlier
    //---INITIALISE THE I2C MODULE FOR MASTER MODE WITH 100KHz ---
    sync_mode = SLAVE_7;
    slew = SLEW_OFF;
    OpenI2C(sync_mode,slew);
    SSPADD = 0xA2; //initialze slave address
    //**** Read the address sent by master from buffer ***
    while(DataRdyI2C()==0); //WAIT UNTILL THE DATA IS TRANSMITTED FROM master
    temp = ReadI2C();
    //**** Data reception from master by slave ***
    do
        {
    while(DataRdyI2C()==0); //WAIT UNTILL THE DATA IS TRANSMITTED FROM master
    I2C_recv[length++]=getcI2C(); // save byte received
    }
    while(length!=20);
    //*** write sequence from slave ***
    while(SSPSTATbits.S!=1); //wait untill STOP CONDITION

    //*** Read the address sent by master from buffer ***
    while(DataRdyI2C()==0); //WAIT UNTILL THE DATA IS TRANSMITTED FROM master
    temp = ReadI2C();
    //*** Slave transmission ***
    if(SSPSTAT & 0x04) //check if master is ready for reception
    while(putsI2C(I2C_send)); // send the data to master
    //---TERMINATE COMMUNICATION FROM MASTER SIDE---
    CloseI2C(); //close I2C module

    TRISBbits.TRISB4 = 1;                   // SCA
    TRISBbits.TRISB6 = 1;                   // SCL
    TRISCbits.TRISC0 = 0;                   //using pin as output


    __delay_ms(10); // let everything settle.

    LATC = 0;                               //clear all pins to 0
    LATCbits.LATC0 = 0;                     //turn ON the LED by writing to the latch
    while(1) continue;                      //sit here forever doing nothing, think "while(true), continue in this loop"

}