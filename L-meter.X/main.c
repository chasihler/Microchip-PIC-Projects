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
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=ON, CP=OFF, BOREN=ON, CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF

#define _XTAL_FREQ 4000000 //defined for delay

    int     an9_value;          //value for a/d
    char    buf[10];            //buff for iota
    long int    fvar;           //long for format math
    long int    ones;           //left of decm
    long int    decm;           //decimal places
    int     tempi;              //to add leadign zeros..


/*
 * LCD  RS      LATA.5
 * LCD  EN      LATA.4
 * LCD  DATA4   LATC.0
 * LCD  DATA5   LATC.1
 * LCD  DATAT6  LATC.2
 * LCD  DATA7   LATC.3
 * LED          LATA.0  for scan rate/heartbeat
 */

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
    lcd_goto(64);
    lcd_puts("                ");
    lcd_goto(64);
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

int main(void) {
        // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block

    TRISCbits.TRISC0 = 0;                   // output
    TRISCbits.TRISC1 = 0;                   // output
    TRISCbits.TRISC2 = 0;                   // output
    TRISCbits.TRISC3 = 0;                   // output
    TRISAbits.TRISA0 = 0;                   // output
    TRISAbits.TRISA4 = 0;                   // output
    TRISAbits.TRISA5 = 0;                   // output
    TRISCbits.TRISC7 = 1;                   //analog input
    ANSELCbits.ANSC7 = 1;                   //...setup on PORTC.7/AN9
    LATAbits.LATA0 = 0;                     //LED Im-Alive test

    __delay_ms(250);                        //let the power settle

     lcd_init();
    __delay_ms(10);
     lcd_clear();

                        //display test message
     lcd_goto(0);
    lcd_puts("Testing the LCD.");
    lcd_goto(64);

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

    while(1)
    {
        LATAbits.LATA0 = 0;              //debugging

        //lcd_clrline2();   //clear LCD line 2 by writting " " and return

        __delay_us(100);

        //PULSE out..
        //measure pulse back 
        //divide pulse widths in half to get peak detection
        //and again with TMR0
        //do the math
        //f=1/(2pi*sqrt(l*c))
        

        __delay_ms(1);        //LCD refresh rate
        LATAbits.LATA0 = 1; //LED Im-Alive test. I made it through conversion
        __delay_ms(1);        //LCD refresh rate
    }
    return (EXIT_SUCCESS);
}

