//  Software License Agreement
//
// The software supplied herewith by Microchip Technology Incorporated (the "Company")
// for its PICmicro® Microcontroller is intended and supplied to you, the Company?s
// customer, for use solely and exclusively on Microchip PICmicro Microcontroller
// products.
//
// The software is owned by the Company and/or its supplier, and is protected under
// applicable copyright laws. All rights are reserved. Any use in violation of the
// foregoing restrictions may subject the user to criminal sanctions under applicable
// laws, as well as to civil liability for the breach of the terms and conditions of
// this license.
//
// THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES, WHETHER EXPRESS,
// IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE
// COMPANY SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
// CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.

//**********************************************************************************
// This very simple program shows how to communicate between PIC16F1784 and a 16K I2C
// Serial EEPROM (24LC16B) using the MSSP port on the PICmicro.
//
// In this example we show how to do a random write of a single byte of data
// and then how to read a single byte back.  Then we go through how to do a
// 'page' write of 16 bytes followed by a sequential read command reading 16 bytes
// back.
//
// This example was done using a 24LC16B serial eeprom but can be used with other
// size arrays with little or no modifications.
//
// Device: PIC16F1784 --> 24LC16B
// Compiler: Microchip XC8 v1.12
// IDE: MPLAB X v1.7
// Created: May 2013

// 
// **********************************************************************************
#include <xc.h> // include standard header file


// Set Config bits - we are using internal oscillator without the PLL
#pragma config FOSC=INTOSC, PLLEN=OFF, MCLRE=ON, WDTE=OFF
#pragma config LVP=OFF, CLKOUTEN=OFF,


// Definitions
#define _XTAL_FREQ  16000000        // this is used by the __delay_ms(xx) and __delay_us(xx) functions
#define device_control_code  0b1010 // All I2C devices have a control code assigned to them.
                                    // and the control code for a serial eeprom is b'1010'



//**************************************************************************************
// Send one byte to SEE
//**************************************************************************************
void Send_I2C_Data(unsigned int databyte)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSPBUF = databyte;              // send databyte
    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}


//**************************************************************************************
// Read one byte from SEE
//**************************************************************************************
unsigned int Read_I2C_Data(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSPCON2bits.RCEN=1;         // set the receive enable bit to initiate a read of 8 bits from the serial eeprom
    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete    
    return (SSPBUF);            // Data from eeprom is now in the SSPBUF so return that value
}

//**************************************************************************************
// Send control byte to SEE (this includes 4 bits of device code, block select bits and the R/W bit)
//**************************************************************************************
// Notes:
// 1) The device code for serial eeproms is defined as '1010' which we are using in this example
// 2) RW_bit can only be a one or zero
// 3) Block address is only used for SEE devices larger than 4K, however on
// some other devices these bits may become the hardware address bits that allow you
// to put multiple devices of the same type on the same bus.  Read the datasheet
// on your particular serial eeprom device to be sure.
//**************************************************************************************
void Send_I2C_ControlByte(unsigned int BlockAddress,unsigned int RW_bit)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit

    // Assemble the control byte from device code, block address bits and R/W bit
    // so it looks like this: CCCCBBBR
    // where 'CCCC' is the device control code
    // 'BBB' is the block address
    // and 'R' is the Read/Write bit

    SSPBUF = ((device_control_code << 4) | (BlockAddress <<1)) + RW_bit;  // send the control byte
    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}

//**************************************************************************************
// Send start bit to SEE
//**************************************************************************************
void Send_I2C_StartBit(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSPCON2bits.SEN=1;          // send start bit
    while(!PIR1bits.SSP1IF);    // Wait for the SSPIF bit to go back high before we load the data buffer
}

//**************************************************************************************
// Send stop bit to SEE
//**************************************************************************************
void Send_I2C_StopBit(void)
{
    PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
    SSPCON2bits.PEN=1;          // send stop bit
    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}


//**************************************************************************************
// Send ACK bit to SEE
//**************************************************************************************
void Send_I2C_ACK(void)
{
   PIR1bits.SSP1IF=0;          // clear SSP interrupt bit
   SSPCON2bits.ACKDT=0;        // clear the Acknowledge Data Bit - this means we are sending an Acknowledge or 'ACK'
   SSPCON2bits.ACKEN=1;        // set the ACK enable bit to initiate transmission of the ACK bit to the serial eeprom
   while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}

//**************************************************************************************
// Send NAK bit to SEE
//**************************************************************************************
void Send_I2C_NAK(void)
{
    PIR1bits.SSP1IF=0;           // clear SSP interrupt bit
    SSPCON2bits.ACKDT=1;        // set the Acknowledge Data Bit- this means we are sending a No-Ack or 'NAK'
    SSPCON2bits.ACKEN=1;        // set the ACK enable bit to initiate transmission of the ACK bit to the serial eeprom
    while(!PIR1bits.SSP1IF);    // Wait for interrupt flag to go high indicating transmission is complete
}

//*************************************************************************************
//***********************      main rouitine     **************************************
//*************************************************************************************
void main() {

    unsigned int eeprom_array[20];
    unsigned int i;
    volatile unsigned int eeprom_data,incoming_data;
    volatile unsigned int block_address, word_address;
    unsigned int ACK_bit;
    

    // set up oscillator control register
    OSCCONbits.IRCF = 0x0F; //set OSCCON IRCF bits to select OSC frequency=16Mhz
    OSCCONbits.SCS = 0x02; //set the SCS bits to select internal oscillator block
 
    //	PORT C Assignments
    TRISCbits.TRISC0 = 0; // RC0 = nc
    TRISCbits.TRISC1 = 0; // RC1 = nc
    TRISCbits.TRISC2 = 0; // RC2 = nc
    TRISCbits.TRISC3 = 1; // RC3 = SCL signal to SEE (must be set as input)
    TRISCbits.TRISC4 = 1; // RC4 = SDA signal to SEE (must be set as input)
    TRISCbits.TRISC5 = 0; // RC5 = nc
    TRISCbits.TRISC6 = 0; // RC6 = nc
    TRISCbits.TRISC7 = 0; // RC7 = nc

    

    //**********************************************************************************
    // Setup MSSP as I2C Master mode, clock rate of 100Khz
    //**********************************************************************************
    // Note: current version of the XC8 compiler (v1.12)  uses the designator "SSPCON" for the
    // first MSSP control register, however, future versions of the compiler may use
    // "SSPCON1" or another variant. If you get errors for this register below
    // this is probably the reason.
    //**********************************************************************************

    SSPCONbits.SSPM=0x08;       // I2C Master mode, clock = Fosc/(4 * (SSPADD+1))
    SSPCONbits.SSPEN=1;         // enable MSSP port

    // **************************************************************************************
    // The SSPADD register value  is used to determine the clock rate for I2C communication.
    //
    // Equation for I2C clock rate:   Fclock = Fosc/[(SSPADD +1)*4]
    //
    // For this example we want the the standard 100Khz I2C clock rate and our
    // internal Fosc is 16Mhz so we get:  100000 = 16000000/[(SSPADD+1)*4]
    // or solving for SSPADD = [(16000000/100000)-4]/4
    // and we get SSPADD = 39

    SSPADD = 39;                // set Baud rate clock divider
    // **************************************************************************************
  

    __delay_ms(10); // let everything settle.


    // ******************************************************************************
    // ***********  write a single byte of data to address 0x00   *******************
    //*******************************************************************************

    block_address = 0x00;   // Set the eeprom block address that we will write the data to
    word_address = 0x00;    // Set the eeprom word address that we will write the data to 
    eeprom_data = 0x55;     // This is the data  we are going to write

    Send_I2C_StartBit();                    // send start bit
    Send_I2C_ControlByte(block_address,0);  // send control byte with R/W bit set low
    Send_I2C_Data(word_address);            // send word address
    Send_I2C_Data(eeprom_data);             // send data byte
    Send_I2C_StopBit();                     // send stop bit

    // ***************************************************************************************
    // At this point the serial eeprom is writing the data we just sent.  We can either
    // wait the maximum write cycle time for the write to complete (5 ms) or we can 'poll'
    // the eeprom to see when it is done.  Polling is done by sending a start bit and
    // a control byte with R/W set to a zero and checking the ACK bit coming back from
    // the eeprom.  While it is still in the write cycle it will not send an ACK so
    // we can just send the startbit/control byte sequence over and over again until
    // the eeprom finally sends and ACK and then we know it is finished. Here is how
    // you can do the polling:
    //
    // ************************ polling loop ***************************************************
    block_address = 0x00;   // Set the eeprom block address

    ACK_bit = 1;            // init the Ack bit high
    while(ACK_bit)          // loop as long as the ack bit is high
    {
        Send_I2C_StartBit();
        Send_I2C_ControlByte(block_address,0);
        ACK_bit = SSPCON2bits.ACKSTAT;  // Ack bit will come back low when the write is complete
    }
    // ******************************************************************************************

    
    // ******************************************************************************
    // ********* Now read back a single byte of data from address 0x00   ************
    //*******************************************************************************

    block_address = 0x00;   // Set the eeprom block address that we will read from
    word_address = 0x00;    // Set the eeprom word address that we will read from

    Send_I2C_StartBit();                    // send start bit
    Send_I2C_ControlByte(block_address,0);  // send control byte with R/W bit set low
    Send_I2C_Data(word_address);            // send word address

    Send_I2C_StartBit();                    // send start bit
    Send_I2C_ControlByte(block_address,1);  // send control byte with R/W bit set high
    incoming_data = Read_I2C_Data();        // now we read the data coming back from the eeprom
    Send_I2C_NAK();                         // send a the NAK to tell the eeprom we don't want any more data
    Send_I2C_StopBit();                     // and then send the stop bit
    


    // *********************************************************************************
    // The 24LC16B has a page buffer of 16 bytes which allows us to execute a 'page' write
    // where we send up to 16 bytes of data before sending the stop bit and executing the
    // write cycle.  Page writes must start on a page boundary.
    //
    // In this section we will write 16 bytes of data starting at address 0x00.
    //*******************************************************************************
   
    block_address = 0x00;   // Set the eeprom block address that we will write the data to
    word_address = 0x00;    // Set the starting eeprom word address that we will write the data to

    Send_I2C_StartBit();                    // send start bit
    Send_I2C_ControlByte(block_address,0);  // send control byte with R/W bit set low
    Send_I2C_Data(word_address);            // send starting address
    Send_I2C_Data('M');             // send data byte 1
    Send_I2C_Data('i');             // send data byte 2
    Send_I2C_Data('c');             // send data byte 3
    Send_I2C_Data('r');             // send data byte 4
    Send_I2C_Data('o');             // send data byte 5
    Send_I2C_Data('c');             // send data byte 6
    Send_I2C_Data('h');             // send data byte 7
    Send_I2C_Data('i');             // send data byte 8
    Send_I2C_Data('p');             // send data byte 9
    Send_I2C_Data(' ');             // send data byte 10
    Send_I2C_Data('R');             // send data byte 11
    Send_I2C_Data('o');             // send data byte 12
    Send_I2C_Data('c');             // send data byte 13
    Send_I2C_Data('k');             // send data byte 14
    Send_I2C_Data('s');             // send data byte 15
    Send_I2C_Data('!');             // send data byte 16
    Send_I2C_StopBit();             // send stop bit

    // now poll until write cycle is finished
    block_address = 0x00;   // Set the eeprom block address
    ACK_bit = 1;            // init the Ack bit high
    while(ACK_bit)          // loop as long as the ack bit is high
    {
        Send_I2C_StartBit();                    // send start bit
        Send_I2C_ControlByte(block_address,0);  // send control byte with R/W bit set low
        ACK_bit = SSPCON2bits.ACKSTAT;          // Ack bit will come back low when the write is complete
    }


    // *********************************************************************************
    // Now lets read those 16 bytes back using a 'sequential' read where we set the starting
    // address and keep clocking in data from the eeprom until we send a NAK telling the
    // eeprom we don't want any more data.
    //
    // We will start reading from address 0x00 and read 16 bytes.
    //*******************************************************************************

    block_address = 0x00;   // Set the eeprom block address that we will read from
    word_address = 0x00;    // Set the eeprom word address that we will read from

    Send_I2C_StartBit();                    // send start bit
    Send_I2C_ControlByte(block_address,0);  // send control byte with R/W bit set low
    Send_I2C_Data(word_address);            // send starting address

    Send_I2C_StartBit();                    // send start bit
    Send_I2C_ControlByte(block_address,1);  // send control byte with R/W bit set high

    for (i=0;i<=14;i++)
    {
        eeprom_array[i] = Read_I2C_Data();  // now we read the first 15 bytes from the eeprom
        Send_I2C_ACK();                     // we have to ACK each byte to tell the eeprom to send another byte
    }

    eeprom_array[15] = Read_I2C_Data();     // now read the last byte from the eeprom
    Send_I2C_NAK();                         // and send a NAK instead of ACK this time to tell the eeprom we don't want any more data
    Send_I2C_StopBit();                     // and then send the stop bit
    
    while (1); //just sit here
    


}








