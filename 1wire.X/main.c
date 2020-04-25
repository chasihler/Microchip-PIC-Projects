/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on Janurary 1, 2015
 *
 * Target Device:
 * 18F14K22 on Tautic 20 pin dev board
 *
 * Project: Maxim 1-Wire Testing
 *
 * Details of 1-wire protocol using Microchip AN1199
 * Field device http://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
 * The 1-Wire Protocol is registered trade mark of Dallas/Maxim semiconductor.
 *
 * Some code was use by the AN1199 App Note Source code; I got stuck looking for a fast way of txing by bit-bang (yes never did this before)
 * The agreement below mentions a license agreement accompaning this software; There was none. I'll note where this software was used if you
 * want to re-write without the Microchip bits.
 * The Microchip licensing as follows:
 *
 *  * FileName:        1wire.c
 * Dependencies:
 * Processor:       PIC18
 * Complier:        MCC18 v3.13
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * Copyright © 2004-2007 Microchip Technology Inc.  All rights reserved.
 *
 * Microchip licenses to you the right to use, copy and distribute Software
 * only when embedded on a Microchip microcontroller or digital signal
 * controller and used with a Microchip radio frequency transceiver, which
 * are integrated into your product or third party product (pursuant to the
 * sublicense terms in the accompanying license agreement).  You may NOT
 * modify or create derivative works of the Software.
 *
 *
 * You should refer to the license agreement accompanying this Software for
 * additional information regarding your rights and obligations.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED ?AS IS? WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION, ANY WARRANTY
 * OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR
 * PURPOSE. IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED
 * UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF
 * WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR
 * EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT,
 * PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF
 * PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY
 * THIRD PARTIES (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER
 * SIMILAR COSTS.
 *
 *
 *
 * Version:
 * 0.1  Configuration, with reset test
 * 0.2
 *
 */
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 16000000 //4Mhz FRC internal osc
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

#define _XTAL_FREQ 16000000 //defined for delay

/*
 * Variables
 */

    int     device_present;             // 1 = 1-wire device on 1-wire bus
    int     i, x, y, int_temp, an4_value;               //
    long int    decm;
    int     itxdata, txdata;            //int RS232 tx data
    char    rxbuff[10], z[1], buf[4];                 //buffer for T-sense 1-wire device
    float    temperature, f, d;
    volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR

/*
 *  Functions
 */

    void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data .. unused currently
    {
        uart_data = RCREG;     // read the incoming data
        PIR1bits.RCIF = 0;      // clear interrupt flag
                                //
    }
    // I left this timer interrupt if I needed it later. This is unused.
    if (PIR1bits.TMR1IF)
    {
        //T1CONbits.TMR1ON = 0;
        PIR1bits.TMR1IF = 0;
        //T1CONbits.TMR1ON = 1;

    }
}


     void __delay_10ms(unsigned char n)     //__delay functions built-in can't be used for much at this speed... so!
 {
     while (n-- != 0) {
         __delay_ms(10);
     }
 }


void uart_send (unsigned int mydata_byte) {      //bytes

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void write_uart(const char *txt)                //strings
{
                                //this send a string to the TX buffer
                                //one character at a time
       while(*txt)
       uart_send(*txt++);
}

//This code if from Microchip but is unused currently.
void uart_send_hex_ascii(unsigned char display_data)
{

	//unsigned char temp;
	//temp = ((display_data & 0xF0)>>4);
	//if (temp <= 0x09)
	//	Putchar(temp+'0');
	//else
	//	Putchar(temp+'0'+0x07);
        //
	//temp = display_data & 0x0F;
	//if (temp <= 0x09)
	//	Putchar(temp+'0');
	//else
	//	Putchar(temp+'0'+0x07);

	//Putchar('\r');
	//Putchar('\n');
}

void serial_init(void)
{

    // calculate values of SPBRGL and SPBRGH based on the desired baud rate
    //
    // For 8 bit Async mode with BRGH=0: Desired Baud rate = Fosc/64([SPBRGH:SPBRGL]+1)
    // For 8 bit Async mode with BRGH=1: Desired Baud rate = Fosc/16([SPBRGH:SPBRGL]+1)



    TXSTAbits.BRGH=1;       // select low speed Baud Rate (see baud rate calcs below)
    TXSTAbits.TX9=0;        // select 8 data bits
    TXSTAbits.TXEN=1;     // enable transmit
    BAUDCONbits.BRG16=0;

    RCSTAbits.SPEN=1;       // serial port is enabled
    RCSTAbits.RX9=0;        // select 8 data bits
    RCSTAbits.CREN=1;       // receive enabled


    SPBRG=25;               //38,400bps-ish
                            //BRG16=0, 7=31.25k, 25=9.615k

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt


         __delay_ms(10);        // give time for voltage levels on board to settle

}


void init_io(void) {
    ANSEL = 0x00;         
    ANSELH = 0x00;

    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output



    TRISBbits.TRISB4 = 0; // output
    TRISBbits.TRISB5 = 1; // input (RX UART)
    TRISBbits.TRISB6 = 0; // output
    TRISBbits.TRISB7 = 0; // output (TX UART)

    LATC = 0x00;

    TRISCbits.TRISC0 = 1; // AN4
    TRISCbits.TRISC1 = 1; // 1-wire data
    TRISCbits.TRISC2 = 0; // 
    TRISCbits.TRISC3 = 0; // 
    TRISCbits.TRISC4 = 0; // 
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input

}


void init_adc (void)
{
    ANSELbits.ANSEL4=1;         //PORTC.0
    ADCON2bits.ADCS = 0x02;     //Fosc/32
    ADCON2bits.ADFM=0;          //left oriented
    ADCON1=0x00;
}

void read_adc (void)
{
    ADCON0bits.CHS0 = 0;        // AD4
    ADCON0bits.CHS1 = 0;
    ADCON0bits.CHS2 = 1;
    ADCON0bits.CHS3 = 0;
    ADCON0bits.ADON = 1;        // A/D ON
    __delay_us(5);

    ADCON0bits.GO   = 1;        // ..GO!

    __delay_us(5);

        while (ADCON0bits.GO) continue;              //wait for conversion
        an4_value = ADRESH;                          //AN4 value
}

void one_wire_reset(void) {
    device_present = 0x00;
    TRISCbits.TRISC1 = 0;
    LATCbits.LATC1 = 0;
    __delay_us(240);        //delay 480 us
    __delay_us(240);
    TRISCbits.TRISC1 = 1;
    __delay_us(70);
    if (!PORTCbits.RC1) {
            device_present = 0x01;
    }
    __delay_us(205);        //delay 410 us
    __delay_us(205);
}

//this looks a lot like the Microchip code, it was not I just happened to be on the right track.
void one_wire_tx_bit(unsigned char txbit) {         // write a bit
    if (txbit) {
    TRISCbits.TRISC1 = 0;
    LATCbits.LATC1 = 0;
    __delay_us(6);    
    TRISCbits.TRISC1 = 1;
    __delay_us(64);      
    }
    else {      
    TRISCbits.TRISC1 = 0;
    LATCbits.LATC1 = 0;
    __delay_us(60);    
    TRISCbits.TRISC1 = 1;
    __delay_us(10);    
    }
}

//from Microchip AN1199 code, renamed and slightly modified to match my software
/**********************************************************************
* Function:        void OW_write_byte (unsigned char write_data)
* PreCondition:    None
* Input:		   Send byte to 1-wire slave device
* Output:		   None
* Overview:		   This function used to transmit a complete byte to slave device.
*				   
***********************************************************************/
void one_wire_tx_byte (unsigned char write_data)
{
	unsigned char loop;
	
	for (loop = 0; loop < 8; loop++)
	{
		one_wire_tx_bit(write_data & 0x01); 	//Sending LS-bit first
		write_data >>= 1;					// shift the data byte for the next bit to send
	}	
}	


//from Microchip AN1199 code: I gathered the essence of this but seeing as I am not using most of the AN1199 code
//and this would not work with XC8 I had to re-write this.
/**********************************************************************
* Function:        unsigned char OW_read_bit (void)
* PreCondition:    None
* Input:		   None
* Output:		   Return the status of the OW PIN
* Overview:		   This function used to read a single bit from the slave device.
*				   
***********************************************************************/

unsigned char one_wire_rx_bit (void)
{
	unsigned char read_data; 
        read_data = 0x00;
	//reading a bit 
	TRISCbits.TRISC1 = 0;
        LATCbits.LATC1 = 0; 						// Drive the bus low
	__delay_us(6);						// delay 6 microsecond (us)
	TRISCbits.TRISC1 = 1;  						// Release the bus
	__delay_us(9);						// delay 9 microsecond (us)

        if (PORTCbits.RC1) {                                    //read 1 or 0
            read_data = 0x01;
        }

	__delay_us(55);						// delay 55 microsecond (us)	
	return read_data;
}


/**********************************************************************
* Function:        unsigned char OW_read_byte (void)
* PreCondition:    None
* Input:		   None
* Output:		   Return the read byte from slave device
* Overview:		   This function used to read a complete byte from the slave device.
*				   
***********************************************************************/

unsigned char one_wire_rx_byte (void)
{
	unsigned char loop, result=0;
	
	for (loop = 0; loop < 8; loop++)                // here we are reading 8 bits (1 byte)
	{
		
		result >>= 1; 				// shift the result to get it ready for the next bit to receive
		if (one_wire_rx_bit())
		result |= 0x80;				// if result is one, then set MS-bit
	}
	return result;					
}	

void one_wire_conversion_pulse(void) {
    	TRISCbits.TRISC1 = 0;
        LATCbits.LATC1 = 1; 		 //For T conv we drive the DQ line high for 750ms (12bit)
	__delay_us(250);                 // delay 
	__delay_us(250);                  
        __delay_us(250);                  
	TRISCbits.TRISC1 = 1; 
        LATCbits.LATC1 = 0;             //just in case this causes problems elsewhere                              
}

int main(void) {

    init_io();

    // set up oscillator control register, using internal OSC at 16MHz.
    OSCCONbits.IRCF = 0x07; //set OSCCON IRCF bits to select OSC frequency 16MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block

    //RCONbits.IPEN = 0;          //dsiable priority levels

    INTCONbits.PEIE = 1;        // Enable peripheral interrupt
    INTCONbits.GIE = 1;         // enable global interrupt


    init_adc();                 //unused but AN4 is there if I need it
    serial_init();

    uart_send ('x');

        LATAbits.LATA0 = 0; //this is just for debugging with an LA..
        __delay_us(1);
        LATAbits.LATA0 = 1; //also confirms oscillator setup is correct.. 1us width
        __delay_us(1);
        LATAbits.LATA0 = 0;

    while (1) {

        one_wire_reset();

        if (device_present) {
            LATCbits.LATC2 = 1;             //this is a 1-wire device out there for debugging

            one_wire_tx_byte(0xCC);         //skip-rom (similar to a broadcast)

            one_wire_tx_byte(0x44);         //do a temp conversion

            one_wire_conversion_pulse();    // hold DQ line high for 750ms

            one_wire_reset();

            //add additional check here later

            one_wire_tx_byte(0xCC);         //skip-rom (similar to a broadcast)

            one_wire_tx_byte(0xBE);         //read scratch pad

            for(i = 0; i<9; i++)            //reading all 9 bytes on the T-Sense
   		rxbuff[i] = one_wire_rx_byte();
            // T-Sense
            //  Byte 0 LSB of Temp
            //  Byte 1 MSB of Temp and sign

            // LSB
            //  2^3 2^2 2^1 2^0 2^-1 2^-2 2^-3 s^-4
            // MSB
            // S S S S S 2^6 2^5 2 ^ 4



            temperature = 0;
            f = 0.0625;

            //z[0] = rxbuff[1];
            //x = atoi(z);
            x = rxbuff[1];

            if (x & 0b10000000) {
                uart_send('-');
            } else {
                uart_send('+');
            }
            
            x = x & 0b00000111;

            int_temp = 0;

            int_temp = rxbuff[0];

            if (x & 0b00000001)
                int_temp = int_temp + 0x100;
            if (x & 0b00000010)
                int_temp = int_temp + 0x200;
            if (x & 0b00000100)
                int_temp = int_temp + 0x400;

            temperature = int_temp * f;

            //this is the only point where the temperature is actually in one piece.

            //convert C to F.
            
            temperature = (temperature * 1.8) + 32;
            
            int_temp = temperature;         //conv to int, kills decimal places.

            itoa(z, int_temp, 10);          //long to ascii string
            write_uart(z);
            uart_send('.');

            d = temperature - int_temp;

            decm = d * 1000;

            //page 374 of XC8 user guide
            ltoa(buf,decm,10);  //long conversion to buffer
            y=strlen(buf);  //uh, adding leading zeros..
            y=3-y;      //probably a better way of doing thing
            while (y)       //first figure out how many zeros
            {
                uart_send('0');  //missed 3-string length
                y=y-1;  //then send them until done
            }   
            write_uart (buf);
        
            uart_send(0x0A);        //LF
            uart_send(0x0D);        //CR

            //temperature   float temperature
            //int_temp      interger value of temperature

            __delay_10ms(254);
        }

    }
    return (EXIT_SUCCESS);
}

