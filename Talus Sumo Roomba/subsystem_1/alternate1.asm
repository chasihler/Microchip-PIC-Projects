;------------------------------------------------------------
;SumoRoomba Subsystem PIC by Charles M Douvier
;PIC16F690
;...this alternate ditched LTC2990 I2C support (well start of what is there) to leave it for future integration
;This susbsystem will:
;- handle power switch, start up timer and initialization
;- tacometer input and math
;- potiential selectable bumper/cliff sensor input
;- bluetooth communication to monitoring PC
;
;v0.01	just enough to compile nothing
;v0.02	hello world (toggling port outputs, ditched ISM and misc code from i2c)
;
;
;(60sec*(1/Fosc*4))/(Period*Interruptions per 360deg)
;
;To Do:
;
;RS-232, 9600 baud, test to BT
;lm339, 74LS125 circuit
;CCP tachometers 2??
;bumper polling
;bumper logic
;power polling
;LED status and power on control 
;
;------------------------------------------------------------

;------------------------------------------------------------
;NOTES
;
;Pin Function Diagram 
;
;RA0	PGD									RC0	_	
;RA1	PGC									RC1	_
;RA2	?int?								RC2	_
;RA3	|MCLR/VPP							RC3 _
;RA4					RB4	?i2c?			RC4 _
;RA5	HeartBeat LED	RB5	RX232			RC5 _
;						RB6	?i2c?			RC6 _
;						RB7	TX232			RC7 ?i2c?
;
;---------------------------------------------------------------------
; Include Files
;---------------------------------------------------------------------
#include <p16f690.inc> ; Change to device that you are using.
;
   __config (_INTRC_OSC_NOCLKOUT & _WDT_OFF & _PWRTE_OFF & _MCLRE_OFF & _CP_OFF & _BOR_OFF & _IESO_OFF & _FCMEN_OFF)
;---------------------------------------------------------------------
;Constant Definitions
;---------------------------------------------------------------------
#define NODE_ADDR 0x02 ; I2C address of this node
; Change this value to address that
; you wish to use.
;---------------------------------------------------------------------
; Buffer Length Definition
;---------------------------------------------------------------------
#define RX_BUF_LEN 32 ; Length of receive buffer

;---------------------------------------------------------------------
; Variable declarations
;---------------------------------------------------------------------
	udata
WREGsave res 1
STATUSsave res 1
FSRsave res 1
PCLATHsave res 1
Index res 1 			; Index to receive buffer
Temp res 1 				;
RXBuffer res RX_BUF_LEN ; Holds rec’d bytes from master						; device.
d1 res 1
d2 res 1
d3 res 1

;---------------------------------------------------------------------
; Vectors
;---------------------------------------------------------------------

    org 0
	nop
	goto 	start

;---------------------------------------------------------------------
;Delay Code 0.5 seconds @ 4Mhz
;---------------------------------------------------------------------
dly
			;499994 cycles
	movlw	0x03
	movwf	d1
	movlw	0x18
	movwf	d2
	movlw	0x02
	movwf	d3
dly_0
	decfsz	d1, f
	goto	$+2
	decfsz	d2, f
	goto	$+2
	decfsz	d3, f
	goto	dly_0

			;2 cycles
	goto	$+1

			;4 cycles (including call)
	return

;---------------------------------------------------------------------
;init_ports
;Initializes ports, etc.
;---------------------------------------------------------------------
init_ports	
	     bsf      STATUS,RP0          ; select Register Page 1
	     bcf      TRISA,5             ; make IO Pin RA5 an output
	     bcf      STATUS,RP0          ; back to Register Page 0
	return

;---------------------------------------------------------------------
; Main Code
;---------------------------------------------------------------------
start
		call	init_ports
Main 
		;clrwdt  Clear the watchdog timer.
		bsf	PORTA,5
		call dly
		bcf	PORTA,5
		call dly
		goto Main ; Loop forever.


	end