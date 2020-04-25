/*
 * File:   main.c
 * Author: Charles M Douvier
 * Contact at: http://iradan.com
 *
 * Created on November 8, 2014, 2:37 PM
 *
 * Target Device:
 * 18F27J53 Development Board by AtomSoft
 *
 * Project: MODbus/RTU slave
 *
 *  Notes
 *  We will ignore both query high bytes for now
 *  TODO:   Pull all hard coded IO out eventually and set it up via USB?
 *          Maybe a terminal application for enabling points?
 *
 *
 * Version:
 * 0.02     Controlled sends caned reply on request to address on RS-232
 * 0.03     CRC works on RX
 * 0.04     No actual IO yet..
 *          Function 0x02 (Read Input Status) Started
 * 0.05     Ported to 18F17J53 for expanded IO/USB options.
 *          12MHz XTAL with CPUDIV set to 16MHz
 *
 */
//#ifndef _XTAL_FREQ
//#define _XTAL_FREQ 16000000 //
//#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
//#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
//#endif

// PIC18F27J53 Configuration Bit Settings

// 'C' source line config statements

#include <xc.h>

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

// CONFIG1L
#pragma config WDTEN = OFF      // Watchdog Timer (Disabled - Controlled by SWDTEN bit)
#pragma config PLLDIV = 3       // PLL Prescaler Selection (Divide by 3 (12 MHz oscillator input))
#pragma config CFGPLLEN = ON    // PLL Enable Configuration Bit (PLL Enabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset (Enabled)
#pragma config XINST = OFF       // Extended Instruction Set (Enabled)

// CONFIG1H
#pragma config CPUDIV = OSC3_PLL3// CPU System Clock Postscaler (CPU system clock divide by 3 from 48MHz)
#pragma config CP0 = OFF        // Code Protect (Program memory is not code-protected)

// CONFIG2L
#pragma config OSC = HSPLL      // Oscillator (HS+PLL, USB-HS+PLL)
#pragma config SOSCSEL = HIGH   // T1OSC/SOSC Power Selection Bits (High Power T1OSC/SOSC circuit selected)
#pragma config CLKOEC = OFF      // EC Clock Out Enable Bit  (CLKO output enabled on the RA6 pin)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor (Disabled)
#pragma config IESO = OFF       // Internal External Oscillator Switch Over Mode (Disabled)

#define _XTAL_FREQ 16000000 //defined for delay

    char    ctxt[10], buf1, testcrc[4];    //buff serial string
    volatile unsigned int     ping, isrcall, index, reading, new_rx;
    int address, crc_high, crc_low, i, querylength;
    unsigned int result;

    //these variables are for fuctions to be enabled/disabled and pushed into application code via later
    int frame, z, bufferbits, query_reg_address, y;
    char data_out[64];
    char reg_address[255];


/*
 *
 *  Interrupt Service
 *
 * UART RX and Timer 1
 *
 */
void interrupt ISR() {

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

    INTCONbits.GIE = 0;         //no interruptions please

    ADCON0 = 0b00000000;        //don't need any ADC
    ADCON1 = 0b00000000;        //speed Vref=AVdd, VssRef=AVss

    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;

    TRISAbits.TRISA0 = 1; // address 1
    TRISAbits.TRISA1 = 1; // address 2
    TRISAbits.TRISA2 = 1; // address 3
    TRISAbits.TRISA3 = 0; // output
    TRISAbits.TRISA5 = 0; // output

    TRISBbits.TRISB0 = 0; //
    TRISBbits.TRISB1 = 1; //
    TRISBbits.TRISB2 = 0; //
    TRISBbits.TRISB3 = 0; //
    TRISBbits.TRISB4 = 0; // 
    TRISBbits.TRISB5 = 1; // 
    TRISBbits.TRISB6 = 0; // 
    TRISBbits.TRISB7 = 0; // 

    TRISCbits.TRISC0 = 0; // timer LED output
    TRISCbits.TRISC1 = 0; // output
    TRISCbits.TRISC2 = 0; // LED test output
    TRISCbits.TRISC3 = 0; // output
    //TRISCbits.TRISC4 = 1; // USB
    //TRISCbits.TRISC5 = 1; // USB
    TRISCbits.TRISC6 = 0; // output TX1
    TRISCbits.TRISC7 = 1; // input  RX1
}

 void __delay_10ms(unsigned char n)     //__delay functions built-in can't be used for much at this speed... so!
 {
     while (n-- != 0) {
         __delay_ms(10);
     }
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

    TXSTA1bits.SYNC = 0;
    TXSTA1bits.BRGH=1;       // select low speed Baud Rate (see baud rate calcs below)
    TXSTA1bits.TX9=0;        // select 8 data bits
    TXSTA1bits.TXEN = 1;     // enable transmit
    RCSTA1bits.SPEN=1;       // serial port is enabled
    RCSTA1bits.RX9=0;        // select 8 data bits
    RCSTA1bits.CREN=1;       // receive enabled
    SPBRG1=102;
    SPBRGH1=0;


    PIR1bits.RCIF=0;        // make sure receive interrupt flag is clear
    PIE1bits.RCIE=1;        // enable UART Receive interrupt
    INTCONbits.PEIE = 1;    // Enable peripheral interrupt
    INTCONbits.GIE = 1;     // enable global interrupt

     __delay_10ms(5);        // give time for voltage levels on board to settle

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
     *  PORTAbits.RA0   Switch 1
     *  PORTAbits.RA1   Switch 2
    */

    if (PORTAbits.RA0 & PORTAbits.RA1) address = 0x04;
    if (!PORTAbits.RA0 & PORTAbits.RA1) address = 0x03;
    if (PORTAbits.RA0 & !PORTAbits.RA1) address = 0x02;
    if (!PORTAbits.RA0 & !PORTAbits.RA1) address = 0x01;

}

/*
 * TODO
 * Poll the inputs
 *
 * I will probably use a 74138 to address banks with 74HC245 for byte/banks of inputs
 * I will likely use the same set up for "Coils"
 * For input registers (analog inputs) I will consider perhaps some kind of A/D input with CD4066+74HC138
 *
 */

void poll_my_inputs(void) {

    //just stuff it with something for now
    for (z = 0; z < 64; ++z)
    {
    reg_address[z] = z;
    }

}


/*
 * did the chicken come before the egg?
* this MODbus CRC bit was gently borrowed from the internet..
* I can't determine who wrote it.. it shows up in an Ardiuno Sketch for MODbus
*  Modbus over serial line - RTU Slave Arduino Sketch
* with referenced of Juan Pablo Zometa, Samuel Marco, Andras Tucsni, Philip Costigan
* It also shows up on a number of websites and forums uncredited... many PIC or C based ..
* so who knows, but I didn't write it
* The MODbus CRC-16 function is outlined on the Modicon site for reference
*/
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

void respond_input_status(void) {
        //ctxt[1] start_addressH
        //ctxt[2] start_addressL
        //ctxt[3] # of regH
        //ctxt[4] # of regL
    querylength = ctxt[4];  //ignoring upper byte for now (TODO)
    query_reg_address = ctxt[2]; //ignoring upper address byte (TODO)

    frame = 0;     //data frame counter

    do {

        data_out[frame] = reg_address[query_reg_address];
        //TODO  make this do somethng
        querylength--;
        frame++;
    } while (querylength>0);

    querylength = ctxt[4];  //reload so we can count time transmitting data

    result = modbus_crc(data_out,0,2);
    crc_high = result >> 8;
    crc_low = result & 0x00FF;

    //uart_xmit response address, function, data_out[frames], CRC

    uart_xmit(address);
    uart_xmit(ctxt[0]);

    frame = 0;
    do {
        uart_xmit(data_out[frame]);
        //TODO  make this do somethng
        querylength--;
        frame++;
    } while (querylength>0);

    uart_xmit(crc_high);
    uart_xmit(crc_low);
}

int main(void) {
    OSCCONbits.SCS = 0x00;

    new_rx=0;

    init_io();
    serial_init();
    run_timer();

    check_my_address();
    
    y += address;

    write_uart("MODBus ");
    uart_xmit(y);

    LATCbits.LATC0 = 1;
    LATCbits.LATC1 = 1;

    __delay_10ms(50);
    LATCbits.LATC0 = 0;
    LATCbits.LATC1 = 0;

    if (address == 0x01){
        LATCbits.LATC0 = 1;
    }
    if (address == 0x02) {
        LATCbits.LATC1 = 1;
    }
    __delay_10ms(50);
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
                if (ctxt[0] == 2) {       //coil status query
                    respond_input_status();
                }
                //TODO other types of functions go here
            }
            //all done, get ready for new query
            new_rx = 0;
        }

        //testcrc[0] = 0xFF;
        //testcrc[1] = 0x02;      //right now this is only thing processed


        //result = modbus_crc(testcrc,0,2);
        //crc_high = result >> 8;
        //crc_low = result & 0x00FF;
        __delay_10ms(10);
        LATCbits.LATC2 = 1; //LED blinky test
        __delay_10ms(10);

        LATCbits.LATC0 = 0;
        LATCbits.LATC1 = 0;
        LATCbits.LATC2 = 0;
    }
}
