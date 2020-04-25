/*
  Copyright (C) 2008. LIS Laboratory, EPFL, Lausanne

  This file is part of Aeropic.

  Aeropic is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation, either version 2.1 of the License, or
  (at your option) any later version.

  Aeropic is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with Aeropic.  If not, see <http://www.gnu.org/licenses/>.
*/ 
/*!
*	\file i2c_compass.c
*	\brief Source file of the I2C communication module with compass
*
*	This module allows to communicate with compass through I2C
*	I2C port communicates in MASTER mode.
*
*	A command of 1 byte is written to the external device, followed by 2 bytes of data. After this, 4 bytes of data are expected as response (2 bytes heading and 2 bytes heading rate).
*	If there is no response, the protocol software resets.
*/

//----------
// Includes
//----------
#include "i2c.h"
#include "i2c_compass.h"

#ifdef AEROPIC_ISHTAR
	#include <ishtar.h>
#endif


//-------------------
// Private variables
//-------------------

//! Data coming from slave device
union 
{
	struct
	{
		unsigned char c0;
		unsigned char c1;
	}
	rchar;
	unsigned int rint;
}
i2c_compass_slaveData;

//! Data going to slave device
union
{ 
	struct
	{
		unsigned char c0;
		unsigned char c1;
	} 
	rchar;
	unsigned int rint;
}
i2c_compass_masterData;

//! The heading value read from the I2C compass
short i2c_compass_heading;
//! The heading rate value read from the I2C compass
short i2c_compass_headingRate;

//! The mag_xyz value read from the I2C compass
short i2c_compass_magX, i2c_compass_magY, i2c_compass_magZ;

//! State of the I2C master state machine
unsigned char i2c_compass_masterState = 0;	// 0 is IDLE state

//! If an error occured during the execution of the state machine
unsigned char i2c_compass_statemachineError = I2C_NO_ERROR;
//! If an error occured anywhere during whole master read process
unsigned char i2c_compass_masterError = I2C_NO_ERROR;


//------------------------------
// Private functions prototypes
//------------------------------

//! The state-machine function called on every I2C master interrupt
void i2c_compass_StateMachine();


//---------------------------------
// Private functions implementation
//---------------------------------

/*!
*	State machine to read compass, called during interruptions of I2C
*/
void i2c_compass_StateMachine()
{
	i2c_compass_masterState++; // advance to next transmission state

	// check for any anomaly
	if (I2C1STATbits.IWCOL)
	{
		I2C1STATbits.IWCOL = 0; // clear error flag
		i2c_compass_statemachineError = I2C_ERROR_STATUS_IWCOL;
	}
	else if (I2C1STATbits.BCL)
	{
		I2C1STATbits.BCL = 0; // clear error flag
		i2c_compass_statemachineError = I2C_ERROR_STATUS_BCL;
	}
	else if (I2C1STATbits.I2COV)
	{
		I2C1STATbits.I2COV = 0; // clear error flag
		i2c_compass_statemachineError = I2C_ERROR_STATUS_I2COV;		
	}
	else  
	switch (i2c_compass_masterState)  
	{
    	case  2 :
			I2C1TRN = I2C_COMPASS_WRITE_ADDRESS;		// slave address will automatically be transmitted
			break;

    	case  3 : 
			if (I2C1STATbits.ACKSTAT) 					// NACK received, send stop sequence
			{ 
				i2c_compass_statemachineError = I2C_ERROR_NACK;
			} 
			else 
			{ 
				// command to slave will automatically be transmitted
				I2C1TRN = I2C_SLAVE_COMMAND; 
			}  
			break;
  
    	case  4 :
			if (I2C1STATbits.ACKSTAT)					// NACK received, send stop sequence
			{
				i2c_compass_statemachineError = I2C_ERROR_NACK;
			} 
			else
			{
				// 1st data byte to slave will automatically be transmitted 
				I2C1TRN = i2c_compass_masterData.rchar.c0;
			}
			break;

    	case  5 :
			if (I2C1STATbits.ACKSTAT) 					// NACK received, send stop sequence
			{
				i2c_compass_statemachineError = I2C_ERROR_NACK;
			} 
			else
			{
				// 2nd data byte to slave will automatically be transmitted
				I2C1TRN = i2c_compass_masterData.rchar.c1;
			}  
			break;

    	case  6 :			    
			if (I2C1STATbits.ACKSTAT)					// NACK received, send stop sequence
			{
				i2c_compass_statemachineError = I2C_ERROR_NACK;
			}
			else
			{
				// issue repeated start sequence
				I2C1CONbits.RSEN = 1;
			}
			break;

		case  7 :
			I2C1TRN = I2C_COMPASS_READ_ADDRESS;			// send slave address and read command
			break;

		case  8 :
			if (I2C1STATbits.ACKSTAT)					// NACK received, send stop sequence
			{ 
				i2c_compass_statemachineError = I2C_ERROR_NACK;
			}
			else
			{
				I2C1CONbits.RCEN = 1;					// issue master read from slave (read 1 byte)
			}  
			break;

		case  9 :
			i2c_compass_slaveData.rchar.c0 = I2C1RCV;	// read data byte, which clears RBF bit
			I2C1CONbits.ACKDT = 0;						// there is no hardware clear (automatic clear) of the bit! 0 means "acknoledge data"
			I2C1CONbits.ACKEN = 1;						// send ACK			    
			break;

		case 10 :
			I2C1CONbits.RCEN = 1;						// issue master read from slave (read 1 byte)
			break;

		case 11 :
			i2c_compass_slaveData.rchar.c1 = I2C1RCV;	// read data byte, which clears RBF bit
			i2c_compass_heading = (short)i2c_compass_slaveData.rint;	// assign the newly read heading value 			
			I2C1CONbits.ACKDT = 0;						// achknoledge data = 0 -> produces ACK to slave 
			I2C1CONbits.ACKEN = 1;
			break;

		case 12 :
			I2C1CONbits.RCEN = 1;						// issue master read from slave (read 1 byte)
			break;
			
		case 13 :
			i2c_compass_slaveData.rchar.c0 = I2C1RCV;	// read data byte, which clears RBF bit
			I2C1CONbits.ACKDT = 0;						// there is no hardware clear (automatic clear) of the bit! 0 means "acknoledge data"
			I2C1CONbits.ACKEN = 1;						// send ACK			    
			break;
		
		case 14 :
			I2C1CONbits.RCEN = 1;						// issue master read from slave (read 1 byte)
			break;
			
		case 15 :
			i2c_compass_slaveData.rchar.c1 = I2C1RCV;	// read data byte, which clears RBF bit
			i2c_compass_headingRate = (short)i2c_compass_slaveData.rint;	// assign the newly read heading rate value			
			I2C1CONbits.ACKDT = 0;						// achknoledge data = FALSE -> produces NACK to slave 
			I2C1CONbits.ACKEN = 1;						// send NACK to end message (this is important for the slave, because the slave does not suspend the clock then)
			break;
		
		case 16 :
			I2C1CONbits.RCEN = 1;						// issue master read from slave (read 1 byte)
			break;
			
		case 17 :
			i2c_compass_slaveData.rchar.c0 = I2C1RCV;	// read data byte, which clears RBF bit
			I2C1CONbits.ACKDT = 0;						// there is no hardware clear (automatic clear) of the bit! 0 means "acknoledge data"
			I2C1CONbits.ACKEN = 1;						// send ACK			    
			break;
		
		case 18 :
			I2C1CONbits.RCEN = 1;						// issue master read from slave (read 1 byte)
			break;
			
		case 19 :
			i2c_compass_slaveData.rchar.c1 = I2C1RCV;	// read data byte, which clears RBF bit
			i2c_compass_magX = (short)i2c_compass_slaveData.rint;	// assign the newly read heading rate value			
			I2C1CONbits.ACKDT = 0;						// achknoledge data = FALSE -> produces NACK to slave 
			I2C1CONbits.ACKEN = 1;						// send NACK to end message (this is important for the slave, because the slave does not suspend the clock then)
			break;
			
			
		
		case 20 :
			I2C1CONbits.RCEN = 1;						// issue master read from slave (read 1 byte)
			break;
			
		case 21 :
			i2c_compass_slaveData.rchar.c0 = I2C1RCV;	// read data byte, which clears RBF bit
			I2C1CONbits.ACKDT = 0;						// there is no hardware clear (automatic clear) of the bit! 0 means "acknoledge data"
			I2C1CONbits.ACKEN = 1;						// send ACK			    
			break;
		
		case 22 :
			I2C1CONbits.RCEN = 1;						// issue master read from slave (read 1 byte)
			break;
			
		case 23 :
			i2c_compass_slaveData.rchar.c1 = I2C1RCV;	// read data byte, which clears RBF bit
			i2c_compass_magY = (short)i2c_compass_slaveData.rint;	// assign the newly read heading rate value			
			I2C1CONbits.ACKDT = 0;						// achknoledge data = FALSE -> produces NACK to slave 
			I2C1CONbits.ACKEN = 1;						// send NACK to end message (this is important for the slave, because the slave does not suspend the clock then)
			break;
			
		
		case 24 :
			I2C1CONbits.RCEN = 1;						// issue master read from slave (read 1 byte)
			break;
			
		case 25 :
			i2c_compass_slaveData.rchar.c0 = I2C1RCV;	// read data byte, which clears RBF bit
			I2C1CONbits.ACKDT = 0;						// there is no hardware clear (automatic clear) of the bit! 0 means "acknoledge data"
			I2C1CONbits.ACKEN = 1;						// send ACK			    
			break;
		
		case 26 :
			I2C1CONbits.RCEN = 1;						// issue master read from slave (read 1 byte)
			break;
			
		case 27 :
			i2c_compass_slaveData.rchar.c1 = I2C1RCV;	// read data byte, which clears RBF bit
			i2c_compass_magZ = (short)i2c_compass_slaveData.rint;	// assign the newly read heading rate value			
			I2C1CONbits.ACKDT = 1;						// achknoledge data = FALSE -> produces NACK to slave 
			I2C1CONbits.ACKEN = 1;						// send NACK to end message (this is important for the slave, because the slave does not suspend the clock then)
			break;
			
		case 28 : 
			I2C1CONbits.PEN = 1;
			break;
	
		case 29 :
			i2c_compass_masterState = 0;				// reset master state
			i2c_FinishedExecution(0);					// remove from interruption, terminate regularly
			break;

		default :
			i2c_compass_statemachineError = I2C_COMPASS_ERROR_READ_FAILED;
	}

	// state machine error occured, stop all
	if (i2c_compass_statemachineError)					// in case I2C_ERROR_NACK
	{
		i2c_compass_masterState = 0;					// reset master state
		i2c_FinishedExecution(1);						// remove from interruption, terminate without unhandled stop sequence
	}
}


//---------------------------------
// Public functions implementation
//---------------------------------

/*
*	This function reads the compass and returns the result
*/
unsigned char i2c_compass_GetData(CompassData * data, short rollAngle)
{
	unsigned char error;

	i2c_compass_masterError     = I2C_NO_ERROR;	
	
	error = i2c_IsBusy(0);							// check if I2C bus is busy or not
	if (error)
	{
		i2c_compass_masterError = error;
	}
	else
	{
		i2c_compass_masterData.rint   = rollAngle;	// the currently measured roll angle will be send to the compass for roll-angle compensation of the magnetic axes

		i2c_compass_statemachineError = I2C_NO_ERROR;			
		i2c_compass_masterState       = 1;			// attempt to launch the state machine
		i2c_Execute(&i2c_compass_StateMachine);
		
		error = i2c_Wait(I2C_COMPASS_TIMEOUT_CYCLES);	// wait for the compass read command to finish execution
		if (error)
		{
			i2c_compass_masterError = error;		// if a timeout error occured (communication to compass lost?)
		}	
		else 
		if (i2c_compass_statemachineError)
		{
			i2c_compass_masterError = i2c_compass_statemachineError;	// error during state machine execution ...
		}	
		else										// ...else: correct values expected
		{
			// normalize the newly read heading value
			if (i2c_compass_heading < 0) 
				i2c_compass_heading += 3600;
				
			// convert to float [0,360]
			data->heading     = (float)i2c_compass_heading * 0.1;	// convert to true degrees
			data->headingRate = (float)i2c_compass_headingRate;		// leave at 0.1 degrees/second scale
			data->magX       = (float)i2c_compass_magX;
			data->magY       = (float)i2c_compass_magY;
			data->magZ       = (float)i2c_compass_magZ;
		}
	}
	if (i2c_compass_masterError)
	{
		data->headingStatus     = TIMEOUT;
		data->headingRateStatus = TIMEOUT;
		data->magXStatus        = TIMEOUT;
		data->magYStatus        = TIMEOUT;
		data->magZStatus        = TIMEOUT;
	}	
	else
	{
		data->headingStatus     = CORRECT;
		data->headingRateStatus = CORRECT;
		data->magXStatus        = CORRECT;
		data->magYStatus        = CORRECT;
		data->magZStatus        = CORRECT;
	}	

	return i2c_compass_masterError;		
}


#ifdef AEROPIC_ISHTAR
/*!
*	This function registers variables to Ishtar.
*/
void i2c_compass_RegisterToIshtar()
{	
	ishtar_Variable("i2c.compass.error", &i2c_compass_masterError, UCHAR, 1, NO_FLAGS);
}	
#endif

