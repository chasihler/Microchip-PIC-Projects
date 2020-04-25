/* 
 * File:   motor.h
 * Author: Adam
 *
 * Created on February 23, 2013, 10:19 PM
 */

#ifndef MOTOR_H
#define	MOTOR_H

#ifdef	__cplusplus
extern "C" {
#endif


void t0Delay(unsigned int usec);
void moveMotor(unsigned int targetStep);
void zeroMotor();
#ifdef	__cplusplus
}
#endif

#endif	/* MOTOR_H */

