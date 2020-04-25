/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://0xEE.net
 *
 * Created on May 18, 2014, 3:21 PM
 *
 * Target Device:
 * 16F887 on breadboard
 *
 * Project: UART Rx Example
 * Code Ported from Francesco Vannini
 *
 * Version:
 * 0.1 initial
 *
 */
#ifndef _XTAL_FREQ
#define _XTAL_FREQ 4000000 //4Mhz FRC internal osc
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#endif

// PIC16F887 Configuration Bit Settings

// 'C' source line config statements

#include <xc.h>

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1
#pragma config FOSC = INTRC_CLKOUT  // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF           // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF          // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON           // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF             // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF            // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF          // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF           // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF          // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)
#pragma config LVP = OFF            // Low Voltage Programming Enable bit (RB3/PGM pin has PGM function, low voltage programming enabled)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

int meaning_of_life, l, n, newline;                    // kinda_NOP
volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR
char    rxtxt[21], rx;
volatile unsigned int index, new_rx;

void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        char temp;
        temp = RCREG;     // read the incoming data
        PIR1bits.RCIF = 0;      // clear interrupt flag
        if(temp=='\n')      //if EOL
        {
            index = 0;
            new_rx = 1;             //"ding"
        }
        else if(temp!=1)           //in middle of GPS sentence
        {
            rxtxt[index] = temp;         //load it up
            index++;                    //increment index
            if(index > 19)              //thats more than enough data
                {
                index = 0;              //reset index
                new_rx = 1;             //"ding"
                }
        }
    }
}


void init_io(void) {
    ANSEL = 0x00;   // NO ADC
    ANSELH = 0x00;  //
    CM1CON0 = 0x00; //No comparators
    CM2CON0 = 0x00;

    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA3 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output
    TRISAbits.TRISA6 = 0; // CLKOUT
    TRISAbits.TRISA7 = 0; // output

    PORTB = 0x00;
    TRISBbits.TRISB0 = 0; // LCD B4
    TRISBbits.TRISB1 = 0; // LCD B5
    TRISBbits.TRISB2 = 0; // LCD B6
    TRISBbits.TRISB3 = 0; // LCD B7
    TRISBbits.TRISB4 = 0; // 
    TRISBbits.TRISB5 = 0; // 
    TRISBbits.TRISB6 = 1; // ICSP DAT
    TRISBbits.TRISB7 = 1; // ICSP CLK


    TRISCbits.TRISC0 = 0; // output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 0; // output
    TRISCbits.TRISC5 = 0; // output
    TRISCbits.TRISC6 = 0; // UART TX
    TRISCbits.TRISC7 = 1; // UART RX

    PORTD=0x00;
    TRISDbits.TRISD0 = 0; // output
    TRISDbits.TRISD1 = 0; // output
    TRISDbits.TRISD2 = 0; // output
    TRISDbits.TRISD3 = 0; // output
    TRISDbits.TRISD4 = 0; // output
    TRISDbits.TRISD5 = 0; // output
    TRISDbits.TRISD6 = 0; // LCD RS (*Tie R/W to Vss)
    TRISDbits.TRISD7 = 0; // LCD EN

    TRISEbits.TRISE0 = 1; // input
    TRISEbits.TRISE1 = 1; // input
    TRISEbits.TRISE2 = 1; // input
    TRISEbits.TRISE3 = 1; // /MCLR
}

void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void write_uart(const char *txt)
{
    while(*txt != 0) uart_xmit(*txt++);     //this send a string to the TX buffer
                                            //one character at a time
}

void serial_init(void)
{

    // calculate values of SPBRGL and SPBRGH based on the desired baud rate

    TXSTAbits.BRGH=1;       // select low speed Baud Rate (see baud rate calcs below)
    TXSTAbits.TX9=0;        // select 8 data bits
    TXSTAbits.TXEN = 1;     // enable transmit


    RCSTAbits.SPEN=1;       // serial port is enabled
    RCSTAbits.RX9=0;        // select 8 data bits
    RCSTAbits.CREN=1;       // receive enabled

    SPBRG=25;               //9600 Baud


    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt

    __delay_ms(100);        // give time for voltage levels on board to settle

}

void lcd_strobe (void)  //Toggle the LCD Enable Pin (for loading info)
{
    PORTDbits.RD7 = 0;
    __delay_ms(10);
    PORTDbits.RD7 = 1;
}

void lcd_write(unsigned char c)  // write a byte to the LCD in 4 bit mode
{
	PORTB = c >> 4;
	lcd_strobe();
	PORTB = c;
	lcd_strobe();
        __delay_ms(1);
}

void lcd_clear(void)    //Clear and home the LCD
{
	PORTDbits.RD6 = 0;
	lcd_write(0x1);
        __delay_ms(5);
}

void lcd_puts(const char * s)   //write a string of chars to the LCD
{
	PORTDbits.RD6 = 1;	// write characters
	while(*s)
		lcd_write(*s++);
}

void lcd_goto(unsigned char pos)    //Go to the specified position on LCD
{
	PORTDbits.RD6 = 0;
	lcd_write(0x80+pos);
}

/*
 *      Write 16 spaces on LCD 1/2 to avoid blanking, (ugly CLEAR effect)
 *      this is slow but work for my needs
 */
void    lcd_clrline1(void)
{
    lcd_goto(0);
    lcd_puts("                ");
    lcd_goto(0);
}

void    lcd_clrline2(void)
{
    lcd_goto(64);
    lcd_puts("                ");
    lcd_goto(64);
}


void lcd_init(void) //initialise the LCD - put into 4 bit mode
{
                            //Check you LCD specifications, not all LCDs are
                            //initialized the same way.. there are even minor
                            //differences on 44780 compatible LCDs sometimes.
    PORTDbits.RD6 = 0;      // write control bytes
    PORTBbits.RB0 = 1;      // Send 0x03
    PORTBbits.RB1 = 1;      // there was a reason for doing this like this..
    PORTBbits.RB2 = 0;      //
    PORTBbits.RB3 = 0;
    __delay_ms(150);        // power on delay
    lcd_strobe();
    __delay_ms(5);
    lcd_strobe();
    __delay_ms(5);
    lcd_strobe();           // Three times
    __delay_ms(5);
    PORTBbits.RB0 = 0;      // Send 0x02;
    PORTBbits.RB1 = 1;
    PORTBbits.RB2 = 0;
    PORTBbits.RB3 = 0;
    __delay_ms(5);
    lcd_strobe();
    __delay_ms(5);
    lcd_write(0x38);        // 4 bit mode, 5x8 font
    lcd_write(0x08);        // display off
    lcd_write(0x0C);        // display on, no cursor
    lcd_write(0x06);        // entry mode (increment)
}
 void main(void)
 {
     OSCCONbits.IRCF = 0x06; //set OSCCON IRCF bits to select internal OSC frequency 4MHz

     init_io();             //set up ports
     serial_init();         //set up UART
     lcd_init();            //setup LCD

    __delay_ms(10);
                            //display Welcome message
    lcd_goto(0);
    lcd_puts("0xEE.net        ");
    lcd_goto(64);
    lcd_puts("Welcomes You!   ");
    __delay_ms(1000);
    lcd_clear();                //Clear Display
    lcd_goto(0);                //Reset to 0 position
    l = 0;
    newline = 0;

     while ( 1 ) {

         meaning_of_life = 42;              //ponder the ultimate question

         if (new_rx==1)
         {
            lcd_puts(rxtxt);            //dump rx to LCD
            n = 0;
            while (n < 20){
                rxtxt[n] = ' ';         //cover up any old characters
                n++;
            }
            if (newline) {              //select which line to start on next string
                lcd_goto(64);           //this LCD's second line is adddr 64, I've seen 40 and 80 as well.
                newline = 0;
            }
            else {
                lcd_goto(0);            //goto first line
                newline=1;
            }
                    
            new_rx = 0;
         }
     }                                      //... wash, rinse, repeat
 }
