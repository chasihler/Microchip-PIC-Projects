/* 
 * File:   main.c
 * Author: Adam
 *
 * Created on February 23, 2013, 10:19 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "motor.h"

/*
 * 
 */
#define _XTAL_FREQ  16000000   //tell compiler that the clock is 16 MHz


__CONFIG(FOSC_INTOSC & WDTE_OFF & PWRTE_OFF & MCLRE_ON & CP_OFF & BOREN_ON & CLKOUTEN_ON & IESO_ON & FCMEN_ON);
__CONFIG(WRT_OFF & STVREN_ON & BORV_LO & LPBOR_OFF & LVP_ON);

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
#define MIN_STEPS_LEFT 23
#define MAX_ACCEL_INDEX 6
#define MAX_STEP 945 /* motor can move 945 steps from stop to stop*/


 int main()
{
    unsigned int i;
    unsigned char rxByte=0;
    OSCCONbits.IRCF = 0b1111;  //Set clock to 16 MHz
    while (!OSCSTATbits.HFIOFS); //Wait for clock to stabalize
    OPTION_REGbits.PSA= 0;  //timer0 prescale assigned to timer0.
    OPTION_REGbits.TMR0CS=0;
    OPTION_REGbits.PS = 0b101; //prescale to 1:64
    LATC = 0;
    TRISC = 0;                 //Set all PORTC pins as outputs
    //make sure analog is off for RB4 and RB5 
    ANSELBbits.ANSB4 = 0;
    ANSELBbits.ANSB5 = 0;
    
    TRISB5 = 1;                     //set RB5 as input (serial RX)
    // automagiclly done by SPEN TRISB7 = 0;      //set RB7 as output(serial TX)
    
    


    //Configure for 9600 baud
    BRGH = 0;
    BRG16 = 0;
    SPBRGL = 25;
    SPBRGH = 0;

        SYNC = 0;                       //Set for Async operation
    SPEN = 1;                       //Enable UART and set TX pin as output
    TXEN = 1;                       //Enable TX circuitry of EUSART
    CREN = 1;                       //Enable RX on EUSART
    while (!TRMT){}    TXREG = 'H';  //Send special value.
    while (!TRMT){}  TXREG = 'E';  //Send special value.
    while (!TRMT) {} TXREG = 'L';  //Send special value.
    while (!TRMT) {} TXREG = 'L';  //Send special value.
    while (!TRMT) {} TXREG = 'O';  //Send special value.
#if 0
    while(!RCIF){} rxByte = RCREG;  //get a char
    while (!TRMT) {} TXREG = rxByte;  //send it back
    while (!TRMT) {} TXREG = rxByte+1;  //send it back +1
    while (!TRMT) {} TXREG = rxByte+2;  //send it back +2
#endif
        TMR0=0;
    INTCONbits.T0IF=0;




    currentStep = 0;
    currentState = 0;
    
    zeroMotor();
    __delay_ms(1000);
    moveMotor(20);
    __delay_ms(1000);
    while(1)
    {
   //     for(i=0;i<900;i+=5)
  //      {
  //          moveMotor(i);
 //       }
#if 0
      __delay_ms(100);
      moveMotor(500);
      __delay_ms(100);
      moveMotor(700);
      //__delay_ms(200);
      moveMotor(300);
      __delay_ms(100);
      moveMotor(20);
      __delay_ms(100);
      moveMotor(600);
      __delay_ms(100);
      moveMotor(900);
#endif

      moveMotor(940);
      moveMotor(5);
    //  __delay_ms(200);
    }
}


/*This routine moves the motor at a slow speed for the full 945 steps */
/*the motor is capable of moving.  This guarentees the motor will hit the */
/* left stop and places it in a 'known zero' state */

/*Possible future mods - move 5 steps off 0 to avoid hitting the stops every*/
/*time a treturn to zero is commanded. */
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
    INTCONbits.T0IF=0; //clear the flag
    TMR0 = t0Preload;
    while(INTCONbits.T0IF==0)
    {
        ;
    }
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

/*Interupt Service Routine*/
void interrupt isr(void)
{
    
}