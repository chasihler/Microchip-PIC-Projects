/* 
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on September 14, 2014, 8:06 AM
 *
 * Target Device:
 * 16F1509 on Tautic 20 pin dev board
 *
 * Project: Digital Resistor Test
 *
 *
 * Version:
 * 1.0  Initial Code to Test Wiper
 *
 *
 *  AD5246BKSZ10-RL7
 *  I2C Address 0x5C (Write)
 *  Resistor I2C
 *  Write S 0 1 0 1 1 1 0 W A X D6 D5 D4 D3 D2 D1 D0 A P
 *  Read  S 0 1 0 1 1 1 0 R A 0 D6 D5 D4 D3 D2 D1 D0 A P
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
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=ON, MCLRE=ON, CP=OFF, BOREN=OFF, CLKOUTEN=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF

#define _XTAL_FREQ 4000000 //defined for delay

    unsigned int ACK_bit;
    int i;
    unsigned char byte, tempbyte1, tempbyte2;

/*
 * 
 */
void init_io(void) {

    ANSELA = 0x00; // all port A pins are digital I/O
    ANSELB = 0x00; // all port A pins are digital I/O
    ANSELC = 0x00; // all port B pins are digital I/O

    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA3 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output

    TRISBbits.TRISB4 = 1; // RB4 I2C SDA, has to be set as an input
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 1; // RB6 I2C SCLK, has to be set as an input
    TRISBbits.TRISB7 = 0; // RB7 = nc

    TRISCbits.TRISC0 = 0; // output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input
}


void I2C_ACK(void)
{
   PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
   SSP1CON2bits.ACKDT=0;        // clear the Acknowledge Data Bit - this means we are sending an Acknowledge or 'ACK'
   SSP1CON2bits.ACKEN=1;        // set the ACK enable bit to initiate transmission of the ACK bit to the serial eeprom
   while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}

void Send_I2C_Data(unsigned int databyte)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSPBUF = databyte;              // send databyte
    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}

unsigned char RX_I2C_Data (void)
{

    RCEN = 1;               //
    while( RCEN ) continue;
    while( !BF ) continue;
    byte = SSPBUF;
   return byte;
}

void I2C_Control_Write0(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSP1BUF = 0x5C;             // send the control byte (90 TCN75, EF BMP085)
    while(!PIR1bits.SSP1IF)     // Wait for interrupt flag to go high indicating transmission is complete
        {
        i = 1;
          // place to add a breakpoint if needed
        }
    PIR1bits.SSP1IF=0;

}
void I2C_Control_Write1(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSP1BUF = 0x92;             // send the control byte (90 TCN75, EF BMP085, change this)
    while(!PIR1bits.SSP1IF)     // Wait for interrupt flag to go high indicating transmission is complete
        {
        i = 1;
          // place to add a breakpoint if needed
        }
    PIR1bits.SSP1IF=0;

}

void I2C_Control_Read0(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSP1BUF = 0x91;             // send the control byte (90 TCN75, EF BMP085, change this)
    while(!PIR1bits.SSP1IF)     // Wait for interrupt flag to go high indicating transmission is complete
        {
        i = 1;
          // place to add a breakpoint if needed
        }
    PIR1bits.SSP1IF=0;
   }



void I2C_Start_Bit(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSPCON2bits.SEN=1;          // send start bit
    while(!PIR1bits.SSP1IF)    // Wait for the SSPIF bit to go back high before we load the data buffer
        {
        i = 1;
        }
    PIR1bits.SSP1IF=0;
}

void I2C_check_idle()
{
    unsigned char byte1; // R/W status: Is a transfer in progress?
    unsigned char byte2; // Lower 5 bits: Acknowledge Sequence, Receive, STOP, Repeated START, START

    do
    {
        byte1 = SSPSTAT & 0x04;
        byte2 = SSPCON2 & 0x1F;
    } while( byte1 | byte2 );
}
/*
 * Send the repeated start message and wait repeated start to finish.
 */
void I2C_restart()
{
    I2C_check_idle();
    RSEN = 1; // Reinitiate start
    while( RSEN ) continue;
}

void I2C_Stop_Bit(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSPCON2bits.PEN=1;          // send stop bit
    while(!PIR1bits.SSP1IF)
    {
        i = 1;
        // Wait for interrupt flag to go high indicating transmission is complete
    }
}

void I2C_NAK(void)
{
    PIR1bits.SSP1IF=0;           // clear SSP interrupt bit
    SSP1CON2bits.ACKDT=1;        // set the Acknowledge Data Bit- this means we are sending a No-Ack or 'NAK'
    SSP1CON2bits.ACKEN=1;        // set the ACK enable bit to initiate transmission of the ACK bit to the serial eeprom
    while(!PIR1bits.SSP1IF)     // Wait for interrupt flag to go high indicating transmission is complete
    {
        i = 1;
    }
}

void set_resistor(void)
{

    I2C_Start_Bit();                    // send start bit
    I2C_Control_Write0();               // send control byte with read set
    Send_I2C_Data(0x7F);                // Send Resistance Value Data
                                        // 0x00 - 0x7F
                                        // 0 - 10KOhm
    I2C_Stop_Bit();                     // Stop

}




int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    SSPCONbits.SSPM=0x08;       // I2C Master mode, clock = Fosc/(4 * (SSPADD+1))
    SSPCONbits.SSPEN=1;         // enable MSSP port
    SSPADD = 0x09;              // 100KHz
    // **************************************************************************************

    init_io();

    __delay_ms(100);             // let everything settle.

    set_resistor();

    while (1) {
        //do something
        PORTAbits.RA0 = 0;
        __delay_ms(100);
        PORTAbits.RA0 = 1;
        __delay_ms(100);
    }
    return (EXIT_SUCCESS);
}
