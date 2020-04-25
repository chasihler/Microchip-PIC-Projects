Change PWM
0x84, 0x01, 0x02, 0x55, 0x2A //7.8KHz PWM

unsigned int delayonnewcycle :1;	//add delay to keep bot in current drive mode
unsigned int nextcycle_stop :1;		//coast on next cycle (test to slow down bot)	
unsigned int nextcycle_reverse:1;	//second step reverse in backup cycle.	
unsigned int nextcycle_turn:1;	//second step turn in backup cycle.	



void determine_drive_by_cliff(void)		//
{

   if (SENSORbits.leftcliff)
   {
       	DRIVEbits.left_rev=1;
	DRIVEbits.delayonnewcycle=1;
	DRIVEbits.nextcycle_reverse=1;
   }
   if (SENSORbits.rightcliff)
   {
       DRIVEbits.right_rev=1;
	DRIVEbits.delayonnewcycle=1;
	DRIVEbits.nextcycle_reverse=1;
   }
   if (SENSORbits.leftcliff && SENSORbits.rightcliff && !DRIVEbits.estop)
   {
       	DRIVEbits.rev=1;
	DRIVEbits.delayonnewcycle=1;
	DRIVEbits.nextcycle_turn=1;
   }
}


void set_drive(void)		//replace
{
   if  (DRIVEbits.estop)
       motor_stop();
   else if (DRIVEbits.left_rev && !DRIVEbits.rev && !DRIVEbits.nextcycle_reverse)
       motor_left_rev_quarter();
   else if (DRIVEbits.right_rev && !DRIVEbits.rev && !DRIVEbits.nextcycle_reverse)
       motor_right_rev_quarter();
   else if (DRIVEbits.rev)
   {
       motor_right_rev_quarter();
       motor_left_rev_quarter();
   }
   else if (DRIVEbits.stop)
       motor_stop();
   else if (DRIVEbits.left_fwd && !DRIVEbits.nextcycle_reverse)
       {
           uart_xmit(0x86); //right stop
           motor_left_fwd_quarter();
       }
       else if (DRIVEbits.right_fwd)
       {
           motor_right_fwd_quarter();
           uart_xmit(0x87); //left stop
       }
   else if (DRIVEbits.fwd && !DRIVEbits.nextcycle_reverse)
       {
           motor_right_fwd_quarter();	//drive forward
           motor_left_fwd_quarter();
       }

   else if (DRIVEbits.nextcycle_turn)
	{
	       motor_left_rev_quarter();
	}
	
   else if (DRIVEbits.nextcycle_reverse)
	{
		motor_right_rev_quarter();
        motor_left_rev_quarter();
		DRIVEbits.nextcycle_reverse=0;
	}
}




if (DRIVEbits.delayonnewcycle)	//Insert into upper main loop
{
i=0x10;
while (i>1)
  {
  __delay_ms(100);
  i=i-1;
  }	
  DRIVEbits.delayonnewcycle=0;
}