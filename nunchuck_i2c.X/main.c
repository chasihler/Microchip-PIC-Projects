/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on January 26, 2014, 12:00 PM
 *
 * Target Device:
 * 16F1509 on Tautic 20 pin dev board
 *
 * Project:
 *  Wii Nunchuck
 *
 * Version:
 * 0.1  Start Bit, and Control Byte ... check
 * 0.2  Read_State Sucessful

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
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=ON, MCLRE=ON, CP=OFF, BOREN=OFF, CLKOUTEN=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF


#define _XTAL_FREQ 4000000 //defined for delay
#define device_address  0b1001000       // unused for now

    unsigned int ACK_bit;
    int i, x;   //garbage flags
    long int tempi, tempn, tempx, tempy, temp10, temp25;
    unsigned char byte, tempbyte1, tempbyte2;
    unsigned char StatusByte;
    unsigned char RESP1Byte, RESP2Byte, RESP3Byte, RESP4Byte, RESP5Byte;
    unsigned char RESP6Byte, RESP7Byte, RESP8Byte, RESP9Byte, RESP10Byte;
    char buf[10];

void init_io(void) {

    ANSELA = 0x00; // all port A pins are digital I/O
    ANSELB = 0x00; // all port A pins are digital I/O
    ANSELC = 0x00; // all port B pins are digital I/O

    TRISAbits.TRISA0 = 0; // keypad strobe 1
    TRISAbits.TRISA1 = 0; // keypad strobe 2
    TRISAbits.TRISA2 = 0; // RADIO /RST
    TRISAbits.TRISA3 = 1; // /MCLR
    TRISAbits.TRISA4 = 0; // LCD RS
    TRISAbits.TRISA5 = 0; // LCD EN

    TRISBbits.TRISB4 = 1; // RB4 I2C SDA, has to be set as an input
    TRISBbits.TRISB5 = 1; // RB5 NC (RESERVED RS232)
    TRISBbits.TRISB6 = 1; // RB6 I2C SCLK, has to be set as an input
    TRISBbits.TRISB7 = 0; // RB7 NC (RESERVED RS232)

    TRISCbits.TRISC0 = 0; // LCD D4
    TRISCbits.TRISC1 = 0; // LCD D5
    TRISCbits.TRISC2 = 0; // LCD D6
    TRISCbits.TRISC3 = 0; // LCD D7
    TRISCbits.TRISC4 = 1; // button col 1
    TRISCbits.TRISC5 = 1; // button col 2
    TRISCbits.TRISC6 = 1; // button col 3
    TRISCbits.TRISC7 = 1; // button col 4
}

void lcd_init(void)
{
//

    i=i;
}

/*
 *  I2C Functions
 *
 */

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

void I2C_Control_Write(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSP1BUF = 0xA4;             // send the control byte 
    while(!PIR1bits.SSP1IF)     // Wait for interrupt flag to go high indicating transmission is complete
        {
        i = 1;
          // place to add a breakpoint if needed
        }
    PIR1bits.SSP1IF=0;

}

void I2C_Control_Read(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSP1BUF = 0xA5;             // send the control byte
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

/*
 *  nunchuk commands
 *
 */

void initialize_nunchuk(void)
{
// 0xA4 0x40 0x00

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x40);
    Send_I2C_Data(0x00);                 //

    I2C_Stop_Bit();
    __delay_ms(100);
}

void start_conversion_with_nunchuck(void)
{
// 0xA4 0x00

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x00);                 //0x00

    I2C_Stop_Bit();
			
}



void read_state (void)
{
/*
 *   STATUS, 6 Bytes
 *  Joy X
 *  Joy Y
 *  Accel X
 *  Accel Y
 *  Accel Z
 *  Z, C bits and LSB Accel

*/
//0x52 RX_STATUS
//STATUS, RESP1, RESP2, RESP3, RESP4, RESP5, RESP6

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Read();

    RX_I2C_Data();                      //Joy X
	StatusByte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //Joy Y
	RESP1Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //Acc X
	RESP2Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //Acc Y
	RESP3Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //Acc Z
	RESP4Byte = byte;
    I2C_ACK();

    	RX_I2C_Data();                      //CZ Status LSB
	RESP5Byte = byte;
    I2C_ACK();

    RX_I2C_Data();                      //
	RESP6Byte = byte;
    I2C_NAK();                          //NAK

	I2C_Stop_Bit();                     // Send Stop Bit

}



int main(void) {

    OSCCONbits.IRCF = 0x0d;     //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02;
    OPTION_REGbits.nWPUEN = 0;  //enable weak pullups (each pin must be enabled individually)

    init_io();

    __delay_ms(50);            //let the power settle


    TRISBbits.TRISB6 = 1;

    SSPSTATbits.SMP = 1;
    SSPCONbits.SSPM=0x08;       // I2C Master mode, clock = Fosc/(4 * (SSPADD+1))
    SSPCONbits.SSPEN=1;         // enable MSSP port
    SSPADD = 0x27;              //figure out which one you can ditch sometime (probably either)
    SSP1ADD = 0x27;             // 100KHz
                                //0x09 = 100KHz
    // **************************************************************************************


    initialize_nunchuk();
    x = 0;
    LATAbits.LATA1 = 0;
    __delay_ms(50);


    while (1) {
        x = 0;
        LATAbits.LATA1 = 0;

        start_conversion_with_nunchuck();
        __delay_ms(5);
        read_state();
        //RESP6Byte
        if (!(RESP5Byte & 0x02) && !(RESP5Byte & 0x01)){
            x = 1;
        }
        if (x) {
            LATAbits.LATA1 = 1;
        }
        __delay_ms(100);
    }
    return;
}
