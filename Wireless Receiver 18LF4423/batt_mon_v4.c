//Include file for this application
#include "batt_mon.h"

//Put all read-only string data at the end of memory
#pragma romdata strings = 0x1000
//hexArray[] is used for nibble->ASCII character conversion
rom char hexArray[] = "0123456789ABCDEF";
//channels[] is used for cell number -> ADC channel lookup
rom unsigned char channels[] = {7, 6, 5, 4, 2, 1, 11, 0, 9, 8, 10, 12};

void main(void) {

	//Array to hold the cell voltages	
	//We define one extra for an obvious mapping between C's 0-based counting and
	// everything else's 1-based counting
	unsigned short int Vcell[12];
	//Keps track of the previous cell voltage measurement for pseudo-differential math
	unsigned short int prevVal;
	//The current (as in present) current (as in dQ/dt) measurement
	unsigned short int Current;
	//Pack current remaining
	unsigned long int Cpack;
	//Just a counter
	unsigned char i;

	initBoard();

	//Clear all the cell voltages
	for (i=0;i<12;i++) Vcell[i] = 0;

	//Let's just begin with the maximum possible charge remaining
	Cpack = 0xFFFFFFFF;

	//Infinite loop for main operation
	while (1) {

		//This must be zero before each pass through the loop
		prevVal = 0;
		
		//Step through all cells
		for (i=0; i<12; i++) {
	
			//Measure the voltage of each cell
			Vcell[i] = measureVoltage(i, &prevVal);
		}

		for (i=0; i<12; i++) {

			//Convert and display the resulting cell voltages
			sendVoltage(Vcell[i]);
		}

		//Finally, we should read and process the current measurement
		Current = measureCurrent();

		//Determine whether this was a charging current or a discharging current
		if (Current > 2500) {
			
			//If the current measured is more than 2500mV, we're in charging mode
			// So add this measurement to the current counter
			// Make sure to do the calculation with saturation to avoid overflow problems
			Cpack = add_w_sat(Cpack, (Current - 2500));
		} else {

			//Otherwise, we're in discharge mode
			// Subtract the current measurement from the current counter
			// Make sure to do the calcuation with saturation to avoid underflow problems
			Cpack = sub_w_sat(Cpack, (2500 - Current));
		}

		//Send the current counter value out the serial port
		sendHex32(Cpack);

		//Signal the end of a line by sending a CR/LF pair
		txbyte(10);
		txbyte(13);

		//Wait ~1/4 second before taking the next measurements
		// This is 0.25/500nsec = 500,000 cycles
		Delay10KTCYx(5);

	} //while (1)

	//Just in case...
	while (1);
}

void initBoard (void) {
	//This function initializes the board for proper operation.

	//First, configure the oscillator.  We will use the 8MHz internal oscillator option.
	OSCCON = 0x70;

	//Set up the I/O ports
	//ADC pins are inputs on PORTA, other PORTA pins are outputs
	TRISA = 0x2F;
	//On PORTB, PGM and ADC pins are inputs, the rest are outputs
	TRISB = 0xDF;
	//UART pins on PORTC are inputs (per datasheet)
	//SDI is an input, other pins are outputs
	TRISC = 0xD0;
	//PORTD is completely unused.  Make all pins outputs to ensure no inputs are floating
	TRISD = 0x00;
	//All PORTE pins are analog, so make them inputs
	TRISE = 0x07;
	//Make all output pins logic 0 by default
	LATA = 0;
	LATB = 0;
	LATC = 0;
	LATD = 0;
	LATE = 0;

	//ADC initializations are next
	//Configure the module for an internal Vref-, external Vref+, and all possible analog inputs
	ADCON1 = 0x10;
	//Make the module run as slow as possible for maximum accuracy
	//Also, use right-justified result storage to ease binary -> decimal conversion later
	ADCON2 = 0xBE;
	//Enable the ADC and select Channel 0 by default
	ADCON0 = 0x01;
	
	//Next, the EUSART
	//We will use this port as output only at 19,200 baud
	SPBRG = 25;
	TXSTA = 0x24;
	RCSTA = 0x90;

	//Set up the SPI channel
	//Sample at the middle of data output time, transmit on idle to active clock
	SSPSTAT = 0x00;
	//Set clock polarity and configure the MSSP (SPI) hardware
	SSPCON1 = 0x20;

	//Make sure the external ADC is initially de-selected
	ADC_CS = 1;
	
}

void sendVoltage(unsigned short int voltage) {
	unsigned char i,j;		//Just two counters
	unsigned short int temp;	//Temporary storage location
	//Each converted result will be stored here
	unsigned short int thous, huns, tens, ones;

	//Convert the voltage to packed BCD for easy serial transmission
	//The value in the 'voltage' argument for this function is assumed to already be processed.
	thous = voltage / 1000;
	txbyte(hexToASCII(thous));
	txbyte('.');
	temp = voltage - 1000 * thous;
	huns = temp / 100;
	txbyte(hexToASCII(huns));
	temp -= 100 * huns;
	tens = temp / 10;
	txbyte(hexToASCII(tens));
	//ones = temp - 10 * tens;

	// *** These function calls were interleaved with the division instructions for speed.
	//     Moving them this way shaved ~8ms off of the time the processor spent measuring and
	//		converting all the channels.
	//Now, we just need to send the number out the serial port.
	//txbyte(hexToASCII(thous));
	//txbyte('.');
	//txbyte(hexToASCII(huns));
	//txbyte(hexToASCII(tens));
	//txbyte(hexToASCII(ones));
	txbyte(' ');
}

unsigned short int measureVoltage(unsigned char channel, unsigned short int *raw) {
	unsigned short int result;

	//First, we need to look up which channel we should use
	channel = channels[channel];
	
	//Select that channel in the converter, then wait a millisecond for the
	// sample-and-hold capacitor to charge up
	//Make sure to include the bit pattern to enable the ADC!
	ADCON0 = channel<<2 | 1;
	conversion_delay;

	//Set the GO bit to initiate a conversion, then wait for the conversion to be complete
	ADCON0bits.GO = 1;
	while (ADCON0bits.GO);

	//Read the result from the ADC module
	result = ADRES;

	//Scale the result.
	// The measurement circuit has a divide-by-17 function, so we multiply
	// by 17 to restore the actual voltage reading.
	result *= 8;//17;

	//Subtract off the previous voltage reading
	result -= *raw;

	//Save the raw reading for this channel
	*raw += result;

	//Return the value we just calculated.
	return result;	
}

unsigned short int measureCurrent(void) {
	//Used for storing intermediate bytes from the external ADC
	unsigned char temp1, temp2;

	//Select the ADC to begin communication
	ADC_CS = 0;
	
	//Per section 6.2 of the MCP3204 datasheet, we delay the ADC start bit
	// to maintain byte-aligned transfers from the ADC.
	spio(0x06);
	temp1 = spio(0x40);
	temp2 = spio(0x00);
	
	//The transmission should be done now.
	ADC_CS = 1;

	return (256*(temp1&15) + temp2);
}

	

void txbyte(char txdata) {
	//Wait for the transmit shift register to be empty
	while (!TXSTAbits.TRMT);
	//Copy the data to transmit to the transmit register
	// The data will be shifted out in the background while further processing occurs
	TXREG = txdata;
}

char spio (char txdata) {
	//Write the transmit data to the transmit shift register
	SSPBUF = txdata;
	//Wait for transmission to be completed
	while (!SSPSTATbits.BF);
	//Read the SPI data register and return with its contents
	return SSPBUF;
}

unsigned long int add_w_sat (unsigned long int input, unsigned short int addend) {
	//We'll use this to keep track of the MSB of the number before and after adding
	unsigned char flags;
	//the output variable
	unsigned long int output;

	//Copy the MSbit of the input into the temporary variable
	flags = (input>>31) & 1;

	//Perform the addition
	output = input + addend;

	//Is the MSBit zero?
	if (!(output & 1<<31)) {
		
		//If yes, see if it was one previously
		if (flags) {

			//If here, adding that last number caused an overflow, so just peg the result to
			// 0xFFFFFFFF
			output = 0xFFFFFFFF;
		}
	}
	
	//Done!
	// Output contains (input + addend) or 0xFFFFFFFF, whichever is less
	return output;
}

unsigned long int sub_w_sat (unsigned long int input, unsigned short int subtrahend) {
	//We'll use this to keep track of the MSB of the number before and after adding
	unsigned char flags;
	//the output variable
	unsigned long int output;

	//Copy the MSbit of the input into the temporary variable
	flags = (input>>31) & 1;

	//Perform the subtraction
	output = input - subtrahend;

	//Is the MSBit one?
	if (output & (1<<31)) {
		
		//If yes, see if it was zero previously
		if (!flags) {

			//If here, adding that last number caused an overflow, so just peg the result to
			// 0x00000000
			output = 0;
		}
	}
	
	//Done!
	// Output contains (input - subtrahend) or 0x00000000, whichever is more
	return output;
}

void sendHex32(unsigned long int input) {
	//Loop counter
	char i;

	for (i=7; i>=0; i--) {
		txbyte(hexToASCII(input>>(4*i)));
	}
	txbyte(' ');
}
	