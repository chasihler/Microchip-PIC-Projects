
//	INTERFACE
//	7 Tune Buttons
//	1 MONITOR (1 SEC HOLD)/STANDBY (RESET SAME INT)
//
//	RA0,1,2,3 	scan buttons
//	RC0,1		source


(void) Tune400(void)
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

(void) Tune425(void)
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

(void) Tune450(void)
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

(void) Tune475(void)
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

(void) Tune500(void)
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

(void) Tune525(void)
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

(void) Tune550(void)
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

(void) VolumeUp(void)
{

}

(void) VolumeDown(void)
{

}

(void) CheckStatus (void)
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


(void) CheckSAME(void)
{
//0x54   SAME_STATUS
//STATUS, ARG1, ARG2, STATUS, RESP1-RESP13.
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
}


(void) ResetSAME(void)
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

poll_buttons()
{
//check buttons for switch. no need for debounce, polling is long in the case of keydown.

LATCBits.LATAC=1;
LATCBits.LATC1=0;
if (LATABits.LATA0) then
	Tune400();
if (LATABits.LATA1) then
	Tune425();
if (LATABits.LATA2) then
	Tune450();	
if (LATABits.LATA3) then
	Tune475();
	
LATCbits.LATC0=0;	
LATCBits.LATC1=1;	
if (LATABits.LATA0) then
	Tune500();
if (LATABits.LATA1) then
	Tune525();	
if (LATABits.LATA2) then
	Tune550();	
if (LATABits.LATA3) then
	MultiFunction();	
}

MultiFunction()
{
LATCbits.LATC0=0;	
LATCBits.LATC1=1;	
	
__delay_ms(1000);

	if (LATABits.LATA3) {
	Monitor();
	else
	Standby();
	}
}