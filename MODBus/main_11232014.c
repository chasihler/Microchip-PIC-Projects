/* 
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on November 8, 2014, 2:37 PM
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
#define _XTAL_FREQ 8000000 //8Mhz FRC internal osc
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#endif

#include <xc.h>

//config bits
#pragma config FOSC=INTOSC, WDTE=OFF, PWRTE=OFF, MCLRE=ON, CP=OFF, BOREN=ON, CLKOUTEN=OFF, IESO=OFF, FCMEN=OFF
#pragma config WRT=OFF, STVREN=OFF, LVP=OFF

#define _XTAL_FREQ 8000000 //defined for delay

    char    ctxt[10], buf1, testcrc[4];    //buff serial string
    volatile unsigned int     ping, isrcall, index, reading, new_rx;
    int address, crc_high, crc_low, i;
    unsigned int result;
    int AV1, AV2, AV3, AV4, BV1, BI1;               //fake inputs for testing

/*
 *  Interrupt Service
 */
void interrupt ISR() {
/*    *    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    *{
    *    parity_rx = RCSTA1bits.RX9D;
    *    uart_data = RCREG;     // read the incoming data
    *    PIR1bits.RCIF = 0;      // clear interrupt flag
    *}
        */
    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        isrcall = 0x01;
        char temp;
        temp = RCREG;     // read the incoming data
        buf1 = temp;
        if(temp==address && reading==0)      //if my address..
        {
            index = 0;                  //reset index
            reading = 1;                //from now on go to else if
            LATCbits.LATC1 = 1;
            new_rx = 0;
            ping = 1;
        }
        else if(reading == 1)           //in middle of GPS sentence
        {
            //TODO reset timeout timer
            ctxt[index] = temp;         //load it up
            index++;                    //increment index
            ping = 1;                   //this is for debugging
            if(index > 6)              //1+7 = master frame.
                {
                index = 0;              //reset index
                reading = 0;            //no longer storing the string
                new_rx = 1;             //"ding"
                T1CONbits.TMR1ON = 0;
                }
        }

        //PIR3bits.RC2IF = 0;      // clear interrupt flag
    }
    //RCSTA2bits.FERR = 0;    //Clear errors
    //RCSTA2bits.OERR = 0;

    //time out timer, if tripped new_rx=0;
    if (PIR1bits.TMR1IF)
    {
        new_rx=0;
        ping = 0;
        T1CONbits.TMR1ON = 0;
        PIR1bits.TMR1IF = 0;
        if (reading) {
        reading = 0;
        LATCbits.LATC0 = 1;
        }
    }
}

void init_io(void) {

    ANSELA = 0x00; // all port A pins are digital I/O
    ANSELB = 0x00; // all port B pins are digital I/O
    ANSELC = 0x00; // all port B pins are digital I/O

    LATC = 0x00;

    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA3 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output

    TRISBbits.TRISB4 = 0; // RB4 = nc
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 = nc
    
    TRISCbits.TRISC0 = 0; // output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // output
    TRISCbits.TRISC3 = 0; // output
    TRISCbits.TRISC4 = 1; // address input
    TRISCbits.TRISC5 = 1; // address input
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input    
}

void uart_xmit(unsigned int mydata_byte) {

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void write_uart(const char *txt)
{
                                //this send a string to the TX buffer
                                //one character at a time
       while(*txt)
       uart_xmit(*txt++);

}

void serial_init(void)
{
    //9600 8N1
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

    SPBRGL=51;  // here is calculated value of SPBRGH and SPBRGL
    SPBRGH=0;

    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt

         __delay_ms(50);        // give time for voltage levels on board to settle

}

void run_timer (void) {
        T1CONbits.TMR1ON = 0;
        TMR1=0;
        TMR1L=0x00;
        TMR1H=0xAA;
        T1CONbits.TMR1CS = 0x01;
        T1CONbits.T1CKPS = 0x01;
        PIE1bits.TMR1IE = 1;
        T1CONbits.TMR1ON = 1;


//        PIR1bits.TMR1IF=0;
}



void check_my_address(void) {
    /*
     *  Determine what address the unit is based on DIP switches
     *  PORTCbits.RC4   Switch 1
     *  PORTCbits.RC5   Switch 2
    */

    if (PORTCbits.RC4 & PORTCbits.RC5) address = 0x04;
    if (!PORTCbits.RC4 & PORTCbits.RC5) address = 0x03;
    if (PORTCbits.RC4 & !PORTCbits.RC5) address = 0x02;
    if (!PORTCbits.RC4 & !PORTCbits.RC5) address = 0x01;

}

void send_mpoll (void) {
               //testing response
            uart_xmit (address);    //address
            uart_xmit (ctxt[0]);    //function
            uart_xmit (0x14);       //20 bytes back
            uart_xmit (0x01);
            uart_xmit (0x02);
            uart_xmit (0x03);
            uart_xmit (0x04);
            uart_xmit (0x05);
            uart_xmit (0x04);
            uart_xmit (0x03);
            uart_xmit (0x02);
            uart_xmit (0x01);
            uart_xmit (0x00);
            uart_xmit (0x01);
            uart_xmit (0x02);
            uart_xmit (0x03);
            uart_xmit (0x04);
            uart_xmit (0x05);
            uart_xmit (0x04);
            uart_xmit (0x03);
            uart_xmit (0x02);
            uart_xmit (0x01);
            uart_xmit (0x00);
            uart_xmit (0x77);       //2 byte CRC
            uart_xmit (0xA8);
}

//did the chicken come before the egg?
//this MODbus CRC bit was gently borrowed from the internet..
//I can't determine who wrote it.. it shows up in an Ardiuno Sketch for MODbus
// Modbus over serial line - RTU Slave Arduino Sketch
//with referenced of Juan Pablo Zometa, Samuel Marco, Andras Tucsni, Philip Costigan
//It also shows up on a number of websites and forums uncredited... many PIC or C based ..
//so who knows, but I didn't write it
//The MODbus CRC-16 function is outlined on the Modicon site for reference

unsigned int modbus_crc(unsigned char buf[], int start, int cnt)
{
	int 	i,j;
   unsigned temp, temp2, flag;

   temp=0xFFFF;

   for (i=start; i<cnt; i++){
   	temp=temp ^ buf[i];

      for (j=1; j<=8; j++){
      	flag=temp & 0x0001;
         temp=temp >> 1;
         if (flag) temp=temp ^ 0xA001;
         }
      }
   /*** Reverse byte order. ***/
   temp2=temp >> 8;
   temp= (temp << 8) | temp2;
   return(temp);
}

int check_master_crc (void) {

    int diff, diff1, diff2;       //what we send back
    char data[6];
    result = 0x0000;
    crc_high = 0x00;
    crc_low = 0x00;

    data[0] = address;  //this is ugly but all master queries are 6bytes+CRC so it'll do
    data[1] = ctxt[0];  //function
    data[2] = ctxt[1];  //start_addressH
    data[3] = ctxt[2];  //start_addressL
    data[4] = ctxt[3];  //# of regH
    data[5] = ctxt[4];  //# of regL

    result = modbus_crc(data,0,6);
    crc_high = result >> 8;
    crc_low = result & 0x00FF;

    diff1 = ctxt[5] ^ crc_high;
    diff2 = ctxt[6] ^ crc_low;
    diff = diff1 + diff2;
    //this spits back an XORed value between calculated and received CRC
    return(diff);
}

int main(void) {

    // set up oscillator control register, using internal OSC at 4MHz.
    OSCCONbits.IRCF = 0x0E; //set OSCCON IRCF bits to select OSC frequency 8MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
    OPTION_REGbits.nWPUEN = 0; // enable weak pullups (each pin must be enabled individually)
    new_rx=0;

    init_io();
    serial_init();
    run_timer();

    check_my_address();

    LATCbits.LATC0 = 1;
    LATCbits.LATC1 = 1;

    __delay_ms(500);
    LATCbits.LATC0 = 0;
    LATCbits.LATC1 = 0;

    if (address == 0x01){
        LATCbits.LATC0 = 1;
    }
    if (address == 0x02) {
        LATCbits.LATC1 = 1;
    }
    __delay_ms(500);
    __delay_ms(500);
    LATCbits.LATC0 = 0;
    LATCbits.LATC1 = 0;


    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;

    while (1) {

        //If read data, not me and then if no 232 traffic for >800us then any xmit over
        //104 us (9600) x 8 quiet periods minus some for wiggle.

        if (ping) {
            run_timer();
        }

        if (new_rx) {       //this device was polled
            //testing response
            i = check_master_crc();     //check master query crc
            if (i==0x00) {              //CRC Ok?
              send_mpoll();             //send test response to known test query
            }

            new_rx = 0;
        }

        //testcrc[0] = 0xFF;
        //testcrc[1] = 0x02;      //right now this is only thing processed


        //result = modbus_crc(testcrc,0,2);
        //crc_high = result >> 8;
        //crc_low = result & 0x00FF;


        __delay_ms(50);
        LATCbits.LATC0 = 0;
        LATCbits.LATC1 = 0;
    }
}
