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
 *  I2C Testing with the TCN75A
 *
 * Version:
 * 0.1  Start Bit, and Control Byte ... check
 * 0.2  /ACK NAK and Stop ... check!
 * 0.3  works+232
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
//IESO=OFF

#define _XTAL_FREQ 4000000 //defined for delay
#define device_address  0b1001000 // TCN75A Address (A012 =0)

    unsigned int ACK_bit;
    int i;
    long int tempi, tempn, tempx, tempy, temp10, temp25;
    unsigned char byte, tempbyte1, tempbyte2;
    unsigned char StatusByte;
    unsigned char RESP1Byte, RESP2Byte, RESP3Byte, RESP4Byte, RESP5Byte;
    unsigned char RESP6Byte, RESP7Byte, RESP8Byte, RESP9Byte, RESP10Byte;
    unsigned char RESP11Byte, RESP12Byte, RESP13Byte, RESP14Byte;
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


    LATC = 0x00;


    TRISCbits.TRISC0 = 0; // LCD D4
    TRISCbits.TRISC1 = 0; // LCD D5
    TRISCbits.TRISC2 = 0; // LCD D6
    TRISCbits.TRISC3 = 0; // LCD D7
    TRISCbits.TRISC4 = 1; // button col 1
    TRISCbits.TRISC5 = 1; // button col 2
    TRISCbits.TRISC6 = 1; // button col 3
    TRISCbits.TRISC7 = 1; // button col 4

    LATCbits.LATC0 = 1;
    LATCbits.LATC1 = 0;
    LATCbits.LATC2 = 0;
}

/*
 *  LCD Interface Functions
 *  standard 44780 format 2 lines
 */

void lcd_strobe (void)  //TOGGLE LCD_EN
{
    LATAbits.LATA5 = 0;
    __delay_ms(20);
    LATAbits.LATA5 = 1;
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
	LATAbits.LATA4 = 0;
	lcd_write(0x1);
        __delay_ms(2);
}


/* write a string of chars to the LCD */

void lcd_puts(const char * s)
{
	LATAbits.LATA4 = 1;	// write characters
	while(*s)
		lcd_write(*s++);
}

/*
 * Go to the specified position
 */

void lcd_goto(unsigned char pos)
{
	LATAbits.LATA4 = 0;
	lcd_write(0x80+pos);
}

/*
 *      Write 16 spaces on LCD 2 to avoid blanking, (ugly CLEAR effect)
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
    lcd_goto(40);
    lcd_puts("                ");
    lcd_goto(40);
}

/* initialise the LCD - put into 4 bit mode */

void lcd_init(void)
{
	LATAbits.LATA4 = 0;	// write control bytes
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
    SSP1BUF = 0xC6;             // send the control byte 4707 Addr w/ SEN=1
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
    SSP1BUF = 0xC7;             // send the control byte
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
 *  Si4707 WB RX Functions
 *
 */

void Tune400(void)
{
//162400	64960	FDC0

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x50);                 //Tune Frequency
    Send_I2C_Data(0x00);                 //0x00
    Send_I2C_Data(0xFD);                 //
    Send_I2C_Data(0xC0);                 //...

    I2C_Stop_Bit();

    __delay_ms(1000);					//tune delay
}

void Tune425(void)
{
//162425	64970	FDCA

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x50);                 //Tune Frequency
    Send_I2C_Data(0x00);                 //0x00
    Send_I2C_Data(0xFD);                 //
    Send_I2C_Data(0xCA);                 //...

    I2C_Stop_Bit();

    __delay_ms(1000);					//tune delay
}

void Tune450(void)
{
//162450	64980	FDD4

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x50);                 //Tune Frequency
    Send_I2C_Data(0x00);                 //0x00
    Send_I2C_Data(0xFD);                 //
    Send_I2C_Data(0xD4);                 //...

    I2C_Stop_Bit();

    __delay_ms(1000);					//tune delay
}

void Tune475(void)
{
//162475	64990	FDDE

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x50);                 //Tune Frequency
    Send_I2C_Data(0x00);                 //0x00
    Send_I2C_Data(0xFD);                 //
    Send_I2C_Data(0xDE);                 //...

    I2C_Stop_Bit();

    __delay_ms(1000);					//tune delay
}

void Tune500(void)
{
//162500	65000	FDE8

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x50);                 //Tune Frequency
    Send_I2C_Data(0x00);                 //0x00
    Send_I2C_Data(0xFD);                 //
    Send_I2C_Data(0xE8);                 //...

    I2C_Stop_Bit();

    __delay_ms(1000);					//tune delay
}

void Tune525(void)
{
//162525	65010	FDF2

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x50);                 //Tune Frequency
    Send_I2C_Data(0x00);                 //0x00
    Send_I2C_Data(0xFD);                 //
    Send_I2C_Data(0xF2);                 //...

    I2C_Stop_Bit();

    __delay_ms(1000);					//tune delay
}

void Tune550(void)
{
//162550	65020	FDFC

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x50);                 //Tune Frequency
    Send_I2C_Data(0x00);                 //0x00
    Send_I2C_Data(0xFD);                 //
    Send_I2C_Data(0xFC);                 //...

    I2C_Stop_Bit();

    __delay_ms(1000);					//tune delay
}

void VolumeUp(void)
{
//future use
}

void VolumeDown(void)
{
//future use
}

void VolumeMAX (void)
{
    // Reset Volume
}
void VolumeMUTE (void)
{
    //Mute Sound

}

void CheckStatus (void)
{
/*
 * STATUS BYTE
 *	[7] CTS	Clear to Send.
 *	0 = Wait before sending next command.
 *	1 = Clear to send next command.

 *	[6] ERR	Error.
 *	0 = No error
 *	1 = Error

 *	5:4 Reserved Values may vary.

 *	[3] RSQINT	Received Signal Quality Interrupt.
 *	0 = Received Signal Quality measurement has not been triggered.
 *	1 = Received Signal Quality measurement has been triggered.

 *	[2] SAMEINT	SAME Interrupt (Si4707 Only).
 *	0 = SAME interrupt has not been triggered.
 *	1 = SAME interrupt has been triggered.

 *	[1] ASQINT	Audio Signal Quality Interrupt.
 *	0 = Audio Signal Quality measurement has not been triggered.
 *	1 = Audio Signal Quality measurement has been triggered.

 *	[0] STCINT	Seek/Tune Complete Interrupt.
 *	0 = Tune complete has not been triggered.
 *	1 = Tune complete interrupt has been triggered.

*/
//0x52 RX_STATUS
//STATUS, RESP1 (VALID), RESP2 FREQ_H, RESP3 FREQ_L, RESP4 RSSI, RESP5 SNR

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();
    Send_I2C_Data(0x52);                //TUNE STATUS
    Send_I2C_Data(0x00);                //DONT CLEAR INT

    I2C_restart();
    I2C_Control_Read();

    RX_I2C_Data();                      //STATUS
	StatusByte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //VALID
	RESP1Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //FREQ1
	RESP2Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //FREQ2
	RESP3Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //RSSI
	RESP4Byte = byte;
    I2C_ACK();

    RX_I2C_Data();                      //SNR
	RESP5Byte = byte;
    I2C_NAK();                          //NAK

	I2C_Stop_Bit();                     // Send Stop Bit

//Update Freq Display
//Update RSSI
}



CheckFlag(unsigned value, unsigned bitindex)
{
//1=0x000000_0
//CheckFlag(BYTE,1);

    return (value & (1 << bitindex)) != 0;
}


void CheckSAME(void)
{
//0x54   SAME_STATUS
//STATUS, ARG1, ARG2, STATUS, RESP1-RESP13.
//
//ARG1 0:INTACK 1:CLRBUF
//ARG2 READ_ADDR
//3:RSQINT 2:SAMEINT 1:ASQINT 0:STCINT

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();
    Send_I2C_Data(0x54);                //SAME STATUS
    Send_I2C_Data(0x00);                //DONT CLEAR INT
	Send_I2C_Data(0x00);                //Start location

    I2C_restart();
    I2C_Control_Read();

    RX_I2C_Data();                      //STATUS
	StatusByte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP1Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP2Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP3Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP4Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP5Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP6Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP7Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP8Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP9Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP10Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP11Byte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //
	RESP12Byte = byte;
    I2C_ACK();

    RX_I2C_Data();                      //
	RESP13Byte = byte;
    I2C_NAK();                          //NAK

	I2C_Stop_Bit();                     // Send Stop Bit

        if (CheckFlag(StatusByte,2))
            i=1;    //TODO
}


void ResetSAME(void)
{
//0x54   SAME_STATUS
//STATUS, ARG1, ARG2, STATUS, RESP1-RESP13.
//ARG1 0:INTACK 1:CLRBUF
//ARG2 READ_ADDR
//3:RSQINT 2:SAMEINT 1:ASQINT 0:STCINT

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();
    Send_I2C_Data(0x54);                //SAME STATUS
    Send_I2C_Data(0x03);                //Dump Buffer and Reset SAME Int.
	Send_I2C_Data(0x00);                //Start location

    I2C_restart();
    I2C_Control_Read();

    RX_I2C_Data();                      //STATUS
	StatusByte = byte;
    I2C_ACK();

	RX_I2C_Data();                      //don't care about the rest..
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

	RX_I2C_Data();                      //
    I2C_ACK();

    RX_I2C_Data();                      //
    I2C_NAK();                          //NAK

	I2C_Stop_Bit();                     // Send Stop Bit
}


void Monitor(void)
{
    //TODO

}

void Standby(void)
{
    //TODO

}

MultiFunction()
{
LATAbits.LATA0=0;
LATAbits.LATA1=1;

__delay_ms(1000);

	if (LATCbits.LATC3)
	Monitor();
	else
	Standby();

}

poll_buttons()
{
//check buttons for switch. no need for debounce, polling is long in the case of keydown.

LATAbits.LATA0=1;
LATAbits.LATA1=0;

if (LATCbits.LATC4)
	Tune400();
if (LATCbits.LATC5)
	Tune425();
if (LATCbits.LATC6)
	Tune450();
if (LATCbits.LATC7)
	Tune475();

LATAbits.LATA0=0;
LATAbits.LATA1=1;

if (LATCbits.LATC4)
	Tune500();
if (LATCbits.LATC1)
	Tune525();
if (LATCbits.LATC2)
	Tune550();
if (LATCbits.LATC3)
	MultiFunction();
}

void RST_4707(void)
{
        LATAbits.LATA2 = 0;
        __delay_ms(50);
        LATAbits.LATA2 = 1;
}


int main(void) {

    OSCCONbits.IRCF = 0x0d; //set OSCCON IRCF bits to select OSC frequency 4MHz
    OSCCONbits.SCS = 0x02;
    OPTION_REGbits.nWPUEN = 0;  //enable weak pullups (each pin must be enabled individually)

    init_io();

    __delay_ms(250);                        //let the power settle

     lcd_init();
    __delay_ms(10);
     lcd_clear();

                        //display test message
    lcd_puts("iradan.com");
    lcd_goto(40);

    TRISBbits.TRISB6 = 1;

    SSPSTATbits.SMP = 1;
    SSPCONbits.SSPM=0x08;       // I2C Master mode, clock = Fosc/(4 * (SSPADD+1))
    SSPCONbits.SSPEN=1;         // enable MSSP port
    SSPADD = 0x27;              //figure out which one you can ditch sometime (probably either)
    SSP1ADD = 0x27;             // 100KHz
                                //0x09 = 100KHz
    // **************************************************************************************

    RST_4707();

    __delay_ms(100);             // let everything settle.

    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                  // send control byte with read set

    if (!SSP1CON2bits.ACKSTAT)
    LATCbits.LATC0 = 0;                   //device /ACked?

    Send_I2C_Data(0x01);                //power up
    Send_I2C_Data(0x53);                //
    Send_I2C_Data(0x05);
    I2C_Stop_Bit();

    __delay_ms(1000);                     //wait


    I2C_Start_Bit();                     // send start bit
    I2C_Control_Write();                 // send control byte

    Send_I2C_Data(0x50);                 //Tune Frequency
    Send_I2C_Data(0x00);                 //0x00
    Send_I2C_Data(0xFD);                 //65020 (162.550)
    Send_I2C_Data(0xFC);                 //... FDFC
    
    I2C_Stop_Bit();

    __delay_ms(1000);

//    I2C_Start_Bit();                     // send start bit
//    I2C_Control_Write();
//
//    Send_I2C_Data(0x52);                //TUNE STATUS
//    Send_I2C_Data(0x00);                //DONT CLEAR INT
//
//    I2C_restart();
//    I2C_Control_Read();
//
//
//    RX_I2C_Data();                      //STATUS
//    I2C_ACK();
//    RX_I2C_Data();                      //VALID
//    I2C_ACK();
//    RX_I2C_Data();                      //FREQ1
//    I2C_ACK();
//    RX_I2C_Data();                      //FREQ2
//    I2C_ACK();
//    RX_I2C_Data();                      //RSSI
//    I2C_ACK();
//    RX_I2C_Data();                      //SNR
//    I2C_NAK();                          //NAK
//    I2C_Stop_Bit();                     // Send Stop Bit

    lcd_clrline1();

    CheckStatus();
    //RESP4Byte
    temp25 = 25;
    temp10 = 10;
    tempn = RESP2Byte;
    tempy = 256;
    tempx = RESP3Byte;
    tempi = tempn*tempy;
    tempi = tempi+tempx;
    tempi = tempi*temp25;
    tempi = tempi/temp10;

    ltoa(buf,tempi,10);  //long conversion to buffer
    lcd_puts(buf);
    lcd_puts(" KHz");
    
    lcd_clrline2();   //clear LCD line 2 by writting " " and return
    lcd_puts(" RSSI:");
    tempi = RESP4Byte;
    ltoa(buf,tempi,10);  //long conversion to buffer
    lcd_puts(buf);




   __delay_ms(1);                      // delay.. just because


    while (1) {
        i=1;
    }
    return;
}
