//******************************************************************************
//
// Software License Agreement                                         
//                                                                    
// The software supplied herewith by Microchip Technology             
// Incorporated (the "Company") is intended and supplied to you, the  
// Company’s customer, for use solely and exclusively on Microchip    
// products. The software is owned by the Company and/or its supplier,
// and is protected under applicable copyright laws. All rights are   
// reserved. Any use in violation of the foregoing restrictions may   
// subject the user to criminal sanctions under applicable laws, as   
// well as to civil liability for the breach of the terms and         
// conditions of this license.                                        
//                                                                     
// THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES,  
// WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED  
// TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A       
// PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,  
// IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR         
// CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.       
//
//*********************************************************************************
//
// File:		pendulum.c
// Date:		14 October 2004		Original code
// Version: 	1.00
//
//*********************************************************************************
#include <pic.h>
#include <pic16f684.h>
#include <math.h>
#include <stdlib.h>

void Init();
void PID();
void Set_Constants();

bit flag1,do_PID,int_flag;
signed char en0, en1, en2, en3, term1_char, term2_char, off_set;
unsigned char temp;
short int temp_int;
unsigned short int ki, kd, kp;
signed int SumE_Min, SumE_Max, SumE, integral_term, derivative_term, un;
signed long Cn;

//	__CONFIG   _CP_OFF & _CPD_OFF & _BOD_OFF & _MCLRE_ON & _WDT_OFF & _INTRC_OSC_NOCLKOUT & _FCMEN_ON

//***************************************************************************
// Positional PID 256 Hz  
//***************************************************************************

//***************************************************************************
//Main() - Main Routine
//***************************************************************************
void main()
{
	Init();                        			//Initialize 12F629 Microcontroller
	Set_Constants();						//Get PID coefficients ki, kp and kd
	while(1)                             	//Loop Forever
	{	
		if(do_PID){
			PID();
		}
	}
}

//***************************************************************************
//Init - Initialization Routine
//***************************************************************************
void Init()
{
	PORTA = 0;				
	TRISA = 0b00101101; 	// Set RA4 and RA2 as outputs			
	PORTC = 0;
	TRISC = 0b00000011;		// Set RC0 and RC1 as inputs, rest outputs
	CMCON0 = 0x07;			// Disable the comparator

	IRCF0 = 1;				// Used to set intrc speed to 8 MHz
	IRCF1 = 1;				// Used to set intrc speed to 8 MHz
	IRCF2 = 1;				// Used to set intrc speed to 8 MHz

	CCP1CON = 0b01001100;	// Full bridge PWM forward
	ECCPAS = 0;				// Auto_shutdown is disabled for now
	PR2 = 0x3F;				// Sets PWM Period at 31.2 kHz
	T2CON = 0;				// TMR2 Off with no prescale
	CCPR1L = 0;				// Sets Duty Cycle to zero
	TMR2ON = 1;				// Start Timer2

	ANSEL = 0b00110101;		// Configure AN0,AN2,AN4 and AN5 as analog
	VCFG = 0;				// Use Vdd as Ref	
	ADFM = 1;				// Right justified A/D result
	ADCS0 = 1;				// 16 TOSC prescale
	ADCS1 = 0;
	ADCS2 = 1;
	CHS0 = 0;				// Channel select AN0
	CHS1 = 0;
	CHS2 = 0;
	ADON = 1;				//Turn A/D on

	en0 = en1 = en2 = en3 = term1_char = term2_char =0;
	ki = kd = 0;
	kp = off_set = 0;
	temp_int = integral_term = derivative_term = un =0;
	SumE_Max = 30000;
	SumE_Min = 1 - SumE_Max;
	do_PID = 1;				// Allowed to do PID function
	T0CS = 0;				// Timer0 as timer not a counter
	TMR0 = 10;				// Preload value
	PSA = 0;				// Prescaler to Timer0
	PS0 = 0;				// Prescale to 32 => 256 Hz
	PS1 = 0;
	PS2 = 1;
	INTCON = 0;
	PIE1 = 0; 
	T0IE = 1;				// Enable Timer0 int
	GIE = 1;
	return;
}


void PID()					// The from of the PID is C(n) = K(E(n) + (Ts/Ti)SumE + (Td/Ts)[E(n) - E(n-1)])					
{
	integral_term = derivative_term = 0;
	
// Calculate the integral term
	SumE = SumE + en0;							// SumE is the summation of the error terms
	if(SumE > SumE_Max){						// Test if the summation is too big
		SumE = SumE_Max;
	}
	if(SumE < SumE_Min){						// Test if the summation is too small
		SumE = SumE_Min;
	}											// Integral term is (Ts/Ti)*SumE where Ti is Kp/Ki
												// and Ts is the sampling period
												// Actual equation used to calculate the integral term is 
												// Ki*SumE/(Kp*Fs*X) where X is an unknown scaling factor 
												// and Fs is the sampling frequency
	integral_term = SumE / 256;					// Divide by the sampling frequency
	integral_term = integral_term * ki;			// Multiply Ki
	integral_term = integral_term / 16;			// combination of scaling factor and Kp

// Calculate the derivative term
	derivative_term = en0 - en3;
	if(derivative_term > 120){					// Test if too large
		derivative_term = 120;
	}
	if(derivative_term < -120){					// test if too small
		derivative_term = -120;
	} 											// Calculate derivative term using (Td/Ts)[E(n) - E(n-1)]
												// Where Td is Kd/Kp
												// Actual equation used is Kd(en0-en3)/(Kp*X*3*Ts)
	derivative_term = derivative_term * kd;		// Where X is an unknown scaling factor
	derivative_term = derivative_term >> 5;  	// divide by 32 precalculated Kp*X*3*Ts

	if(derivative_term > 120){					
		derivative_term = 120;
	}
	if(derivative_term < -120){
		derivative_term = -120;
	}
												// C(n) = K(E(n) + (Ts/Ti)SumE + (Td/Ts)[E(n) - E(n-1)])
	Cn = en0 + integral_term + derivative_term;	// Sum the terms
	Cn = Cn * kp / 1024;						// multiply by Kp then scale
 
	if(Cn >= 1000)								// Used to limit duty cycle not to have punch through
	{
		Cn = 1000;
	}
    if(Cn <= -1000)
	{
		Cn = -1000;
	}  
	if(Cn == 0){				// Set the speed of the PWM
		DC1B1 = DC1B1 = 0;
		CCPR1L = 0;
	}
	if(Cn > 0){					// Motor should go forward and set the duty cycle to Cn
		P1M1 = 0;				// Motor is going forward
		temp = Cn;
		if(temp^0b00000001){
			DC1B0 = 1;
		}
		else{
			DC1B0 = 0;
		}
		if(temp^0b00000010){
			DC1B1 = 1;
		}
		else{
			DC1B1 = 0;
		}	
		CCPR1L = Cn >> 2;		// Used to stop the pendulum from continually going around in a circle	
		off_set = off_set +1;	// the offset is use to adjust the angle of the pendulum to slightly 
		if(off_set > 55){		// larger than it actually is
			off_set = 55;
		}	
	}

	else {						// Motor should go backwards and set the duty cycle to Cn
		P1M1 = 1;				// Motor is going backwards
		temp_int = abs(Cn);		// Returns the absolute int value of Cn
		temp = temp_int;		// int to char of LS-Byte
		if(temp^0b00000001){
			DC1B0 = 1;
		}
		else{
			DC1B0 = 0;
		}
		if(temp^0b00000010){
			DC1B1 = 1;
		}
		else{
			DC1B1 = 0;
		}	
		CCPR1L = temp_int >> 2;	// Used to stop the pendulum from continually going around in a circle	
		off_set = off_set -1;
		if(off_set < -55){
			off_set = -55;
		}
	}
	en3 = en2;		// Shift error signals
	en2 = en1;
	en1 = en0;
	en0 = 0;
	do_PID = 0;				// Done
	RA4 = 0;				// Test flag to measure the speed of the loop
	return;
}

void Set_Constants()
{
	ANS2 = 1;				// Configure AN2 as analog
	ANS4 = 1;				// Configure AN4 as analog
	ANS5 = 1;				// Configure AN5 as analog

	ADFM = 1;				// Right justified A/D result
	CHS0 = 0;				// Channel select AN4
	CHS1 = 0;
	CHS2 = 1;
	temp = 200;				// Gives delay
	while(temp){
		temp--;
	}
	GODONE = 1;
	while(GODONE);{
		temp = 0;			// Does nothing.....
	}
	ki = ADRESH << 8;		// Store the A/D result to Integral Constant
	ki = ki + ADRESL;

	CHS0 = 1;				// Channel select AN5
	CHS1 = 0;
	CHS2 = 1;
	temp = 200;				// Gives delay
	while(temp){
		temp--;
	}
	GODONE = 1;
	while(GODONE);{
		temp = 0;			// Does nothing.....
	}
	kd = ADRESH << 8;		// Store the A/D result to Differential Constant
	kd = kd + ADRESL;

	CHS0 = 0;				// Channel select AN2
	CHS1 = 1;
	CHS2 = 0;
	temp = 200;				// Gives delay
	while(temp){
		temp--;
	}
	GODONE = 1;
	while(GODONE);{
		temp = 0;			// Does nothing.....
	}
	kp = ADRESH << 8;		// Store the A/D result to Proportional Constant
	kp = kp + ADRESL;
	CHS0 = 0;				// Channel select AN0
	CHS1 = 0;
	CHS2 = 0;	
}

void interrupt Isr()
{

	if(T0IF&&T0IE){
		TMR0 = 10;				// Preload value
		T0IF = 0;				// Clear Int Flag

//		flag1 = (!flag1);
		RA4 = 1;

	  	temp_int = 0;
		temp_int = ADRESH << 8;	// Store the A/D result with offset
		temp_int = temp_int + ADRESL - 512;
		en0 = temp_int + off_set/8;				// Store to error function asuming no over-flow
	
		do_PID = 1;				// Allowed to do PID function
		GODONE = 1;				// Start next A/D cycle
	}
	else
	{
		PIR1 = 0;
		RAIF = 0;
		INTF = 0;
	}
	if(temp_int > 180){ 		//Check if error is too large (positive)
		
		DC1B0 = DC1B1 = 0;		// Stop PWM
		CCPR1L = 0;	
		en0 = en1 = en2 = en3 = term1_char = term2_char = off_set = 0; 		// Clear all PID constants
		Cn = integral_term = derivative_term = SumE = RA4 = 0;
		do_PID = 0;				// Stop doing PID
	}
	if(temp_int < -180){ 		//Check if error is too large (negative)
		
		DC1B0 = DC1B1 = 0;		// Stop PWM
		CCPR1L = 0;	
		en0 = en1 = en2 = en3 = term1_char = term2_char = off_set = 0; 		// Clear all PID constants
		Cn = integral_term = derivative_term = SumE = RA4 = 0;
		do_PID = 0;				// Stop doing PID
	}

}
