//PID 
//Kerry Wong

void loop() {
    float a = EstimateAngle() - INIT_ANGLE;
 
    angleBuffer[angleBufferIndex] = a;
    angleBufferIndex = (angleBufferIndex + 1) % ANGLE_BUFFER_LENGTH;
    float ang = GetAvgAngle();
 
    curErr = ang - INIT_ANGLE; //error    
    SumErr += curErr;
     
    if (SumErr > SumErrMax) SumErr = SumErrMax;
    else if (SumErr < SumErrMin) SumErr = SumErrMin;
     
    //Ki*SumE/(Kp*Fs*X) 
    integralTerm = SumErr * elapsedTimeSec * Ki / Kp * 10.0; 
    derivativeTerm = curErr - prevErr;
     
    if(derivativeTerm > 0.1) derivativeTerm = 0.1;
    else if (derivativeTerm < -0.1) derivativeTerm = -0.1;
     
    // Kd(curErr-prevErr)*Ts/(Kp*X)
    derivativeTerm = derivativeTerm * Kd * elapsedTimeSec / Kp; 
     
    if(derivativeTerm > 120) derivativeTerm = 120;
    else if (derivativeTerm < -120) derivativeTerm = -120;
     
    Cn = (curErr + integralTerm + derivativeTerm) * Kp / 10.0;
    Serial.println(Cn);
    WheelDirection dir;
     
    if (Cn > 0) dir = DIR_FORWARD;
    else if (Cn < -0) dir = DIR_BACKWARD;
    else dir = DIR_STOP;
        
    throttle = abs(Cn);
         
    if (abs(ang) > 0.7)  MoveWheels(DIR_STOP, 0); //if angle too large to correct, stop motor        
    else MoveWheels(dir, throttle);
     
    prevErr = curErr;
}


The current inclination angle is estimated and stored in the circular buffer. The averaged inclination angle measurement is fed into the PID controller. As mentioned earlier, if no averaging is needed, the current inclination estimate can be used directly by setting the buffer length to 1.
Parameters of the PID controller is calculated using the current error measurement (curErr) and the cumulative error (SumErr).
We use the output of the PID controller (Cn) as the “throttle” to control the motors. The sign of the Cn corresponds to the motor rotation direction.
If the inclination angle is too large (e.g. greater than roughly 40 degree), there is no way for the robot to correct its position anyway so we will simply stop the motor. This condition is needed to prevent the wheels from moving when the robot is positioned horizontally.
The current error term is assigned to prevErr and now we are ready for the next loop.

/*
TILT-START ENGAGED
3,9\n
balancegyroDegrees:-6.84 accelDegrees:7.88 overallAngleofTilt:-1.263,10\n
balancegyroDegrees:-8.92 accelDegrees:5.17 overallAngleofTilt:-3.173,10\n
balancegyroDegrees:-58.72 accelDegrees:13.55 overallAngleofTilt:-14.824,9\n
balancegyroDegrees:-9.22 accelDegrees:12.48 overallAngleofTilt:-15.913,9\n
balancegyroDegrees:-9.38 accelDegrees:12.07 overallAngleofTilt:-16.614,9\n
*/