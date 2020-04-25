//****************************************************************************/
// Includes
//****************************************************************************/
#include "PID.h"
#include "QEI.h"
 
//****************************************************************************/
// Defines
//****************************************************************************/
#define RATE  0.01
#define Kc   -2.6
#define Ti    0.0
#define Td    0.0
 
//****************************************************************************/
// Globals
//****************************************************************************/
//--------
// Motors 
//--------
//Left motor.
PwmOut leftMotor(p23);
DigitalOut leftBrake(p19);
DigitalOut leftDirection(p28);
QEI leftQei(p29, p30, NC, 624);
PID leftController(Kc, Ti, Td, RATE);
//-------
// Files
//-------
LocalFileSystem local("local");
FILE* fp;
//--------
// Timers
//--------
Timer endTimer;
//--------------------
// Working variables.
//--------------------
volatile int leftPulses     = 0;
volatile int leftPrevPulses = 0;
volatile float leftPwmDuty  = 1.0;
volatile float leftVelocity = 0.0;
//Velocity to reach.
int goal = 3000;
 
//****************************************************************************/
// Prototypes
//****************************************************************************/
//Set motors to go "forward", brake off, not moving.
void initializeMotors(void);
//Set up PID controllers with appropriate limits and biases.
void initializePidControllers(void);
 
void initializeMotors(void){
 
    leftMotor.period_us(50);
    leftMotor = 1.0;
    leftBrake = 0.0;
    leftDirection = 0;
 
}
 
void initializePidControllers(void){
 
    leftController.setInputLimits(0.0, 10500.0);
    leftController.setOutputLimits(0.0, 1.0);
    leftController.setBias(1.0);
    leftController.setMode(AUTO_MODE);
 
}
 
int main() {
 
    //Initialization.
    initializeMotors();
    initializePidControllers();
 
    //Open results file.
    fp = fopen("/local/pidtest.csv", "w");
    
    endTimer.start();
 
    //Set velocity set point.
    leftController.setSetPoint(goal);
 
    //Run for 3 seconds.
    while (endTimer.read() < 3.0){
        leftPulses = leftQei.getPulses();
        leftVelocity = (leftPulses - leftPrevPulses) / RATE;
        leftPrevPulses = leftPulses;
        leftController.setProcessValue(leftVelocity);
        leftPwmDuty = leftController.compute();
        leftMotor = leftPwmDuty;
        fprintf(fp, "%f,%f\n", leftVelocity, goal);
        wait(RATE);
    }
 
    //Stop motors.
    leftMotor  = 1.0;
    
    //Close results file.
    fclose(fp);
 
}