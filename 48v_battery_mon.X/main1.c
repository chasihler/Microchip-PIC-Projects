/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on January 18, 2014, 9:42 AM
 *
 * Target Device:
 * 16F1509 on Tautic 20 pin dev board
 *
 * Project:
 * A/D --> LCD Test
 * 8-bit resolution across Vdd to Vref (0-5V)
 * for 3.3V operation adjust A/D math
 *
 * LCD (44780 type) Test with XC8 compiler
 * LCD code ported from Mike Pearce's 2001 LCD code for HI-TECH C
 * as found on http://www.microchipc.com/
 *
 * Version:
 * 1.0      voltage proof of concept
 * 1.1      current and RS-232 polling added
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
#include <plib.h>

//config bits
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=ON, CP=OFF, BOREN=ON, CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF

#define _XTAL_FREQ 4000000 //defined for delay

    int     an9_value;          //value for Volts a/d
    int     an8_value;          //value for Current a/d
    char    buf[10];            //buff for iota
    long int    fvar;           //long for format math
    long int    tens;           //left of decm
    long int    decm;           //decimal places
    int     tempi;              //to add leadign zeros..
    int     vtxdata;             //volts int for TX
    int     itxdata;
    int     batt_status;             //normal=0 alarm=code


volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR

/*
 * LCD  RS      LATA.5
 * LCD  EN      LATA.4
 * LCD  DATA4   LATC.0
 * LCD  DATA5   LATC.1
 * LCD  DATAT6  LATC.2
 * LCD  DATA7   LATC.3
 * LED          LATA.0  for scan rate/heartbeat
 */
void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        uart_data = RCREG;     // read the incoming data
        PIR1bits.RCIF = 0;      // clear interrupt flag
    }

}

void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}


void lcd_strobe (void)  //TOGGLE LCD_EN
{
    LATAbits.LATA4 = 0;
    __delay_ms(20);
    LATAbits.LATA4 = 1;
}

/* write a byte to the LCD in 4 bit mode */

void lcd_write(unsigned char c)
{
	LATC = c >> 4;
	lcd_strobe();
	LATC = c;
	lcd_strobe();
        __delay_us(100);
}

/*
 * 	Clear and home the LCD
 */

void lcd_clear(void)
{
	LATAbits.LATA5 = 0;
	lcd_write(0x1);
        __delay_ms(2);
}


/* write a string of chars to the LCD */

void lcd_puts(const char * s)
{
	LATAbits.LATA5 = 1;	// write characters
	while(*s)
		lcd_write(*s++);
}

/*
 * Go to the specified position
 */

void lcd_goto(unsigned char pos)
{
	LATAbits.LATA5 = 0;
	lcd_write(0x80+pos);
}

/*
 *      Write 16 spaces on LCD 2 to avoid blanking, (ugly CLEAR effect)
 *      this is slow but work for my needs
 */

void    lcd_clrline2(void)
{
    lcd_goto(40);
    lcd_puts("                ");
    lcd_goto(40);
}

void blinkLEDfast(void)
{

        LATAbits.LATA0 = 1;
        __delay_ms(50);
        LATAbits.LATA0 = 0;              //debugging
        batt_status = 1;
}
void blinkLEDslow(void)
{

        LATAbits.LATA0 = 1;
        __delay_ms(500);
        LATAbits.LATA0 = 0;              //debugging
        batt_status = 0;
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

    SPBRGL=25;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH=0;

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt

         __delay_ms(50);        // give time for voltage levels on board to settle

    uart_xmit('R');         // transmit some data
    uart_xmit('S');
    uart_xmit('T');
    uart_xmit('\n');


}

/* initialise the LCD - put into 4 bit mode */

void lcd_init(void)
{
	LATAbits.LATA5 = 0;	// write control bytes
        LATC = 0x03;
        __delay_ms(150);         //power on delay
	lcd_strobe();
        __delay_ms(5);
	lcd_strobe();
        __delay_ms(5);
	lcd_strobe();
        __delay_ms(5);
	LATC = 0x02;             // set 4 bit mode
        __delay_ms(5);
	lcd_strobe();
        __delay_ms(5);
	lcd_write(0x28);	// 4 bit mode, 1/16 duty, 5x8 font
	lcd_write(0x08);	// display off
	lcd_write(0x0C);	// display on cursor+blink off
	lcd_write(0x06);	// entry mode
}

void read_volts(void)
{
        lcd_clrline2();   //clear LCD line 2 by writting " " and return

           if ( uart_data && batt_status)
           lcd_puts("!");


        ADCON0 = 0b00100101;                            //select AN9 and enable

/*  ADCON1
 *  bit 7 ADFM: ADC Result Format Select bit
 *   0 = Left justified. Six Least Significant bits of ADRESL are set to ?0? when the conversion result is loaded.
 *  bit 6-4 ADCS<2:0>: ADC Conversion Clock Select bits
 *   110 = FOSC/64
 *  bit 3-2 Unimplemented: Read as ?0?
 *  bit 1-0 ADPREF<1:0>: ADC Positive Voltage Reference Configuration bits
 *   00 = VREF+ is connected to VDD
 */
        ADCON1 = 0b01100000;                  //left justified, FOSC/64 speed Vref=Vdd

        __delay_us(5);
        GO = 1;
        while (GO) continue;              //wait for conversion
        an9_value = ADRESH;               //AN9 value


        //format value for LCD read out
        //value = AD_value * multiplier)
        //value = value / 256 (8 bit number)
        fvar = an9_value;
        fvar = fvar * 10749;        //calibration
        fvar = fvar / 256;
        tens = fvar / 100;
        //tens = tens % 10;
        decm = fvar % 100;
        vtxdata = fvar / 100;
        uart_xmit('v');
        uart_xmit(vtxdata);
        uart_xmit('.');

        //page 366 of XC8 user guide
        itoa(buf,tens,10);  //int conv to buffer
        lcd_puts(buf);      //outputs "1s" place to LCD.
        lcd_puts(".");
        //page 374 of XC8 user guide
        ltoa(buf,decm,10);  //long conversion to buffer
        tempi=strlen(buf);  //uh, adding leading zeros..
        tempi=2-tempi;      //probably a better way of doing thing
        while (tempi)       //first figure out how many zeros
        {
            lcd_puts("0");  //missed 3-string length
            tempi=tempi-1;  //then send them until done
        }
        lcd_puts(buf);      //output buffer to LCD
        lcd_puts(" V");     //attach some units for display

}

void read_current(void)
{
        ADCON0 = 0b00100001;                            //select AN8 and enable

/*  ADCON1
 *  bit 7 ADFM: ADC Result Format Select bit
 *   0 = Left justified. Six Least Significant bits of ADRESL are set to ?0? when the conversion result is loaded.
 *  bit 6-4 ADCS<2:0>: ADC Conversion Clock Select bits
 *   110 = FOSC/64
 *  bit 3-2 Unimplemented: Read as ?0?
 *  bit 1-0 ADPREF<1:0>: ADC Positive Voltage Reference Configuration bits
 *   00 = VREF+ is connected to VDD
 */
        ADCON1 = 0b01100000;                  //left justified, FOSC/64 speed Vref=Vdd

        __delay_us(5);
        GO = 1;
        while (GO) continue;              //wait for conversion
        an8_value = ADRESH;               //AN9 value


        //format value for LCD read out
        //value = AD_value * multiplier)
        //value = value / 256 (8 bit number)
        fvar = an8_value;
        fvar = fvar * 5172;         //calibration
        fvar = fvar - 656844;         //zeroing midspan
        fvar = fvar / 256;
        tens = fvar / 100;
        //tens = tens % 10;
        decm = fvar % 100;
        itxdata = fvar / 25;
        uart_xmit('i');
        uart_xmit(itxdata);
        uart_xmit('.');

        //page 366 of XC8 user guide
        itoa(buf,tens,10);  //int conv to buffer
        lcd_puts(buf);      //outputs "1s" place to LCD.
        lcd_puts(".");
        decm=abs(decm);     //strip out the -, if there is one
        //page 374 of XC8 user guide
        ltoa(buf,decm,10);  //long conversion to buffer

        tempi=strlen(buf);  //uh, adding leading zeros..
        tempi=2-tempi;      //probably a better way of doing thing
        while (tempi)       //first figure out how many zeros
        {
            lcd_puts("0");  //missed 3-string length
            tempi=tempi-1;  //then send them until done
        }
        lcd_puts(buf);      //output buffer to LCD
        lcd_puts(" A");     //attach some units for display

}

void print_status(void)
{
    lcd_clrline2();
    if ( batt_status )
        lcd_puts("Alarm");
    else
        lcd_puts("Normal");
}

void read_all(void)
{
    read_volts();
    __delay_ms(50);
    read_current();
}

int main(void) {
        // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)

    TRISAbits.TRISA0 = 0;                   // output
    TRISAbits.TRISA4 = 0;                   // output
    TRISAbits.TRISA5 = 0;                   // output

    ANSELA = 0x00; // all port A pins are digital I/O

    TRISBbits.TRISB4 = 0;                   // RB4 = nc
    TRISBbits.TRISB5 = 1;                   // RB5 = nc
    TRISBbits.TRISB6 = 0;                   // RB6 = nc
    TRISBbits.TRISB7 = 0;                   // RB7 = nc

    ANSELB = 0x00; // all port B pins are digital I/O

    TRISCbits.TRISC0 = 0;                   // output
    TRISCbits.TRISC1 = 0;                   // output
    TRISCbits.TRISC2 = 0;                   // output
    TRISCbits.TRISC3 = 0;                   // output
    TRISCbits.TRISC6 = 1;                   //analog input/i AN8
    TRISCbits.TRISC7 = 1;                   //analog input/v AN9

    ANSELCbits.ANSC6 = 1;                   //...setup on PORTC.6/AN8
    ANSELCbits.ANSC7 = 1;                   //...setup on PORTC.7/AN9

    LATAbits.LATA0 = 0;                     //LED Im-Alive test

    __delay_ms(250);                        //let the power settle

    serial_init();

    lcd_init();

    __delay_ms(10);
     lcd_clear();

                        //display test message
    lcd_puts("Battery Status");

    read_volts();

    while(1)
    {
       uart_data=0;
        __delay_ms(1000);


       if ( !uart_data )
           print_status();

       if ( uart_data )
        read_all();


        if ( vtxdata < 41 )
            blinkLEDfast();
        if ( vtxdata > 40 )
            blinkLEDslow();

        if ( uart_data )
            lcd_puts("*");
       if ( uart_data )
           __delay_ms(1000);
                   
    }
    return (EXIT_SUCCESS);
}

