int	tc;		//time constant
int	pb;		//prop band
int	ig;		//integration gain
int id;		//integration divsor
int db;		//deadband
int ki;		//integration output
int x, y;	//temp
float z;
int pv, sp;	//process variable, setpoint

//on interrupt timer
void intrp {

z = ig / id;
x = pv - sp;
x = x * z;

Ki = x;

}

void setup timer(void) {
//adjust timer by tc
//tc = max(tc, 0);
//tc = min(tx, maxnumber_for_tc);

//... adjust time base.
}

// if pv - sp > db ... 
