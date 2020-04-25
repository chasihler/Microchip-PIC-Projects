/* 
 * File:   newmain.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on January 26, 2014, 12:00 PM
 *
 * Target Device:
 * 16F1509 on Tautic 20 pin dev board
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
//#include <plib.h>

//config bits
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=ON, MCLRE=ON, CP=OFF, BOREN=ON, CLKOUTEN=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF
//IESO=OFF

#define _XTAL_FREQ 4000000 //defined for delay
#define device_address  0b1001000 // TCN75A Address (A012 =0)

    unsigned int ACK_bit;
/*
 * 
 */
void init_io(void) {
    TRISAbits.TRISA0 = 1; // output
    TRISAbits.TRISA1 = 1; // output
    TRISAbits.TRISA2 = 1; // output
    TRISAbits.TRISA3 = 1; // output
    TRISAbits.TRISA4 = 1; // output
    TRISAbits.TRISA5 = 1; // output

    ANSELA = 0x00; // all port A pins are digital I/O

    TRISBbits.TRISB4 = 1; // RB4 I2C SDA, has to be set as an input
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 1; // RB6 I2C SCLK, has to be set as an input
    TRISBbits.TRISB7 = 1; // RB7 = nc


    LATCbits.LATC0 = 0;
    LATCbits.LATC1 = 0;
    LATCbits.LATC2 = 0;
    ANSELB = 0x00; // all port B pins are digital I/O
    TRISCbits.TRISC0 = 0; // output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 0; // input
    TRISCbits.TRISC7 = 0; // input

    ANSELC = 0x00; // all port B pins are digital I/O

}

//**************************************************************************************
// Send one byte to SEE
//**************************************************************************************
//void Send_I2C_Data(unsigned int databyte)
//{
//    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
//    SSPBUF = databyte;              // send databyte
//    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
//}


//**************************************************************************************
// Read one byte from SEE
//**************************************************************************************
//unsigned int Read_I2C_Data(void)
//{
//    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
//    SSPCON2bits.RCEN=1;         // set the receive enable bit to initiate a read of 8 bits from the serial eeprom
//    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
//    return (SSPBUF);            // Data from eeprom is now in the SSPBUF so return that value
//}

//**************************************************************************************
// Send control byte to SEE (this includes 4 bits of device code, block select bits and the R/W bit)
//**************************************************************************************
// Notes:
// 1) The device code for serial eeproms is defined as '1010' which we are using in this example
// 2) RW_bit can only be a one or zero
// 3) Block address is only used for SEE devices larger than 4K, however on
// some other devices these bits may become the hardware address bits that allow you
// to put multiple devices of the same type on the same bus.  Read the datasheet
// on your particular serial eeprom device to be sure.
//**************************************************************************************
void Send_I2C_ControlByteR(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit

    SSPBUF = 0x91;  // send the control byte
    while(!PIR1bits.SSP1IF)    // Wait for interrupt flag to go high indicating transmission is complete
        {
        LATCbits.LATC1 = 0;
        LATCbits.LATC1 = 1;
          // place to add a breakpoint if needed
        }
}

void Send_I2C_ControlByteW(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit

    SSPBUF = 0x90;  // send the control byte
    while(!PIR1bits.SSP1IF)    // Wait for interrupt flag to go high indicating transmission is complete
        {
         // place to add a breakpoint if needed
        }
}

//**************************************************************************************
// Send start bit to SEE
//**************************************************************************************
void Send_I2C_StartBit(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSPCON2bits.SEN=1;          // send start bit
    while(!PIR1bits.SSP1IF)    // Wait for the SSPIF bit to go back high before we load the data buffer
        {
        LATCbits.LATC0 = 0;
//        LATCbits.LATC0 = 1;
          // place to add a breakpoint if needed
        }
}

//**************************************************************************************
// Send stop bit to SEE
//**************************************************************************************
void Send_I2C_StopBit(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSPCON2bits.PEN=1;          // send stop bit
    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}


//**************************************************************************************
// Send ACK bit to SEE
//**************************************************************************************
void Send_I2C_ACK(void)
{
   PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
   SSPCON2bits.ACKDT=0;        // clear the Acknowledge Data Bit - this means we are sending an Acknowledge or 'ACK'
   SSPCON2bits.ACKEN=1;        // set the ACK enable bit to initiate transmission of the ACK bit to the serial eeprom
   while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}

//**************************************************************************************
// Send NAK bit to SEE
//**************************************************************************************
void Send_I2C_NAK(void)
{
    PIR1bits.SSP1IF=0;           // clear SSP interrupt bit
    SSPCON2bits.ACKDT=1;        // set the Acknowledge Data Bit- this means we are sending a No-Ack or 'NAK'
    SSPCON2bits.ACKEN=1;        // set the ACK enable bit to initiate transmission of the ACK bit to the serial eeprom
    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}


int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    init_io();

    //INTCONbits.PEIE = 1;
    
    //**********************************************************************************
    // Setup MSSP as I2C Master mode, clock rate of 100Khz
    //**********************************************************************************
    // Note: current version of the XC8 compiler (v1.12)  uses the designator "SSPCON" for the
    // first MSSP control register, however, future versions of the compiler may use
    // "SSPCON1" or another variant. If you get errors for this register below
    // this is probably the reason.
    //**********************************************************************************

    SSPCONbits.SSPM=0x08;       // I2C Master mode, clock = Fosc/(4 * (SSPADD+1))
    SSPCONbits.SSPEN=1;         // enable MSSP port
//SSPCON1 = 0b00111000;


    // **************************************************************************************
    // The SSPADD register value  is used to determine the clock rate for I2C communication.

    SSPADD = 0x09;                // 100KHz
    // **************************************************************************************


    __delay_ms(100); // let everything settle.


    // ******************************************************************************
    // ********* Now read back a single byte of data from address 0x00   ************
    //*******************************************************************************

 //   block_address = 0x00;   // Set the eeprom block address that we will read from
 //   word_address = 0x00;    // Set the eeprom word address that we will read from

//    Send_I2C_StartBit();                    // send start bit
//    Send_I2C_ControlByteW;                  // send control byte with R/W bit set low
//    Send_I2C_Data(word_address);            // send word address

    LATCbits.LATC0 = 1;
    Send_I2C_StartBit();                    // send start bit
    LATCbits.LATC1 = 1;
    Send_I2C_ControlByteR();                  // send control byte with R/W bit set high

    while (!SSP1CON2bits.ACKSTAT);

    LATAbits.LATA2 = 1;
    //incoming_data = Read_I2C_Data();        // now we read the data coming back from the eeprom
    Send_I2C_NAK();                         // send a the NAK to tell the eeprom we don't want any more data
    Send_I2C_StopBit();                     // and then send the stop bit


    


    while (1) {
        __delay_ms(500);
        LATAbits.LATA1 = 1;
        __delay_ms(500);
        LATAbits.LATA1 = 0;
    }
    return (EXIT_SUCCESS);
}
