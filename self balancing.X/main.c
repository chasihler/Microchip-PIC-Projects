/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on February 14, 2014, 11:39 AM
 *
 * Target Device:
 * 18F14K22 on Tautic 20 pin dev board
 *
 * Project: Self Balancing Test
 *
 *
 * Version:
 * 0.1  Configuration, TX + AD setup
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

    long int    decm;           //long temp
    int     tempi;              //temp
    int     error, pid_out, pid_outi, pid_int, gain, pband, tc;
    int     i, ilevel, an4_value;                  //temp
    int     itxdata, txdata;            //int RS232 tx data
    char    buf[10];            //buff for iota
    volatile unsigned int uart_data;    // use 'volatile' qualifer as this is changed in ISR

/*
 *  Functions
 */

    void interrupt ISR() {

    if (PIR1bits.RCIF)          // see if interrupt caused by incoming data
    {
        uart_data = RCREG;     // read the incoming data
        PIR1bits.RCIF = 0;      // clear interrupt flag
                                //
    }
    //time out timer, if tripped new_rx=0;
    if (PIR1bits.TMR1IF)
    {
        //T1CONbits.TMR1ON = 0;
        PIR1bits.TMR1IF = 0;
        tc = 1;
        //T1CONbits.TMR1ON = 1;

    }
}


     void __delay_10ms(unsigned char n)     //__delay functions built-in can't be used for much at this speed... so!
 {
     while (n-- != 0) {
         __delay_ms(10);
     }
 }


void uart_xmit(unsigned int mydata_byte) {      //bytes

    while(!TXSTAbits.TRMT);    // make sure buffer full bit is high before transmitting
    TXREG = mydata_byte;       // transmit data
}

void write_uart(const char *txt)                //strings
{
                                //this send a string to the TX buffer
                                //one character at a time
       while(*txt)
       uart_xmit(*txt++);
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

    //  uart_xmit('R');         // transmit a character example

}


void init_io(void) {
    ANSEL = 0x00;         // no A/D
    ANSELH = 0x00;

    TRISAbits.TRISA0 = 0; // output
    TRISAbits.TRISA1 = 0; // output
    TRISAbits.TRISA2 = 0; // output
    TRISAbits.TRISA4 = 0; // output
    TRISAbits.TRISA5 = 0; // output



    TRISBbits.TRISB4 = 0; // RB4 = nc
    TRISBbits.TRISB5 = 1; // RB5 = nc
    TRISBbits.TRISB6 = 0; // RB6 = nc
    TRISBbits.TRISB7 = 0; // RB7 = nc

    TRISCbits.TRISC0 = 1; // AN4
    TRISCbits.TRISC1 = 0; // /high low speed
    TRISCbits.TRISC2 = 0; // Enable Drive
    TRISCbits.TRISC3 = 0; // Direction
    TRISCbits.TRISC4 = 0; // Driver Output Test
    TRISCbits.TRISC5 = 0; // PWM output
    TRISCbits.TRISC6 = 1; // input
    TRISCbits.TRISC7 = 1; // input

}

void dir_fwd (void){
    LATCbits.LATC3 = 0;
}

void dir_rev (void){
    LATCbits.LATC3 = 1;
}

void en_drive (void){
    LATCbits.LATC2 = 0;
}

void dis_drive (void){
    LATCbits.LATC2 = 1;
}

void high_speed (void){
    LATCbits.LATC1 = 0;
}
void low_speed (void){
    LATCbits.LATC1 = 1;
}


void drive (int inc){
         while (inc-- != 0) {
             LATCbits.LATC4 = 1;
             __delay_ms(4);
             LATCbits.LATC4 = 0;
             __delay_ms(4);
     }
}

void run_timer (void) {

    T1CONbits.TMR1ON = 0;
    PIR1bits.TMR1IF=0;
    //0x0FF0 = 32ms

    //Time Constant
    TMR1L=0x0A;
    TMR1H=0x09;
    //T1CONbits.T1OSCEN = 0;
    T1CONbits.TMR1CS = 0;
    T1CONbits.T1CKPS = 0x01;
    PIE1bits.TMR1IE = 1;
    T1CONbits.TMR1ON = 1;

}

int pid (int pv) {
    int err;
    err = pv - 0x7E;
    //I need to add K (gain)
    //some kind of timer for my process interval
    //integration...
    return err;
}

void init_adc (void)
{
    ANSELbits.ANSEL4=1;         //PORTC.0
    ADCON2bits.ADCS = 0x02;     //Fosc/32
    ADCON2bits.ADFM=0;          //left
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

        decm = an4_value;
        txdata = decm * 1;
}

int main(void) {

    init_io();

    // set up oscillator control register, using internal OSC at 16MHz.
    OSCCONbits.IRCF = 0x07; //set OSCCON IRCF bits to select OSC frequency 16MHz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block

    //RCONbits.IPEN = 0;          //dsiable priority levels

    INTCONbits.PEIE = 1;        // Enable peripheral interrupt
    INTCONbits.GIE = 1;         // enable global interrupt


    init_adc();
    serial_init();
    run_timer();

    // Tuning Parameters
    //
    pband = 40;
    gain = 0x01;

    LATAbits.LATA0=1;

    dir_fwd();
    en_drive();
    drive(10);
    dis_drive();


    while (1) {

        LATAbits.LA1 = 0;
        read_adc();
        if (tc) {
            itoa(buf,txdata,10);
            write_uart (buf);
            uart_xmit (' ');
            error = pid(txdata);
            if (error < 6 && error > -6) {
               dis_drive();
            }
            LATAbits.LA1 = 1;
            tc = 0;
            low_speed();
            pid_outi = 0;
            pid_out = 0;
            //output
            if (error < -1) {       //deadband
                    en_drive();
                    pid_outi = abs(error);      //determine absolute value of error

                    if (pid_outi > 0x1F) {
                        LATCbits.LATC1 = 0;
                    }

                    pid_outi = (pid_outi / 4);

                    dir_rev();
                    
                    pid_out=pid_outi;//+pid_int;
                    drive(pid_out);
                    
                    //pid_out = abs(error);      //determine absolute value of error
                uart_xmit ('-');
                itoa(buf,pid_out,10);
                write_uart (buf);

            }

            if (error > 1) {
                    en_drive();
                    pid_outi = abs(error);      //determine absolute value of error

                    if (pid_outi > 0x1F) {
                        LATCbits.LATC1 = 0;
                    }

                    pid_outi = (pid_outi / 4);

                    dir_fwd();

                    pid_out=pid_outi;//+pid_int;
                    drive(pid_out);

                    //pid_out = abs(error);      //determine absolute value of error
                uart_xmit ('+');
                itoa(buf,pid_out,10);
                write_uart (buf);
            }

            pid_int = ((pid_int + pid_outi+2)/2);
            uart_xmit (' ');
            uart_xmit ('i');
            itoa(buf,pid_int,10);  //int conv to buffer
            write_uart (buf);
            uart_xmit (0x0D); //CR
        }
        

    }
    return (EXIT_SUCCESS);
}

