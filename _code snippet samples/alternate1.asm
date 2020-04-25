;------------------------------------------------------------
;SumoRoomba Subsystem PIC by Charles M Douvier
;PIC16F690
;...this alternate ditched LTC2990 I2C support (well start of what is there) to leave it for future integration
;...also on the back burner is encoder tachometer integration (for now).. 
;Likely have to add 2x16F628 1 for each CCP for the encoders
;
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
;RA0	PGD						RC0	_	
;RA1	PGC						RC1	_
;RA2	?int?						RC2	_
;RA3	|MCLR/VPP					RC3 _
;RA4			RB4	?i2c?			RC4 _
;RA5	HeartBeat LED	RB5	RX232			RC5 _
;			RB6	?i2c?			RC6 _
;			RB7	TX232			RC7 ?i2c?
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
;#define NODE_ADDR 0x02 ; I2C address of this node
; Change this value to address that
; you wish to use.
;---------------------------------------------------------------------
; Buffer Length Definition
;---------------------------------------------------------------------
;#define RX_BUF_LEN 32 ; Length of receive buffer
;
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
d1 res 1		;delay
d2 res 1		;delay
d3 res 1		;delay

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

;-------------------------------------------------------
;RS-232 initialization
;Port Map
;		bit7	bit6	bit5	bit4	bit3	bit2	bit1	bit0
;TXSTA 	CSRC 	TX9 	TXEN 	SYNC 	SENDB 	BRGH 	TRMT 	TX9D 	0000 0010
;RCSTA 	SPEN 	RX9 	SREN 	CREN 	ADDEN 	FERR 	OERR 	RX9D 	0000 000X
;
;TXSTA
;
;bit 7 CSRC: Clock Source Select bit
;	Asynchronous mode:
;	Don’t care.
;	Synchronous mode:
;	1 = Master mode (clock generated internally from BRG)
;	0 = Slave mode (clock from external source)
;bit 6 TX9: 9-Bit Transmit Enable bit
;	1 = Selects 9-bit transmission
;	0 = Selects 8-bit transmission
;bit 5 TXEN: Transmit Enable bit(1)
;	1 = Transmit enabled
;	0 = Transmit disabled
;bit 4 SYNC: EUSART Mode Select bit
;	1 = Synchronous mode
;	0 = Asynchronous mode
;bit 3 SENDB: Send Break Character bit
;	Asynchronous mode:
;	1 = Send Sync Break on next transmission (cleared by hardware upon completion)
;	0 = Sync Break transmission completed
;	Synchronous mode:
;	Don’t care.
;bit 2 BRGH: High Baud Rate Select bit
;	Asynchronous mode:
;	1 = High speed
;	0 = Low speed
;	Synchronous mode:
;	Unused in this mode.
;bit 1 TRMT: Transmit Shift Register Status bit
;	1 = TSR is empty
;	0 = TSR is full
;bit 0 TX9D: 9th Bit of Transmit Data
;	Can be address/data bit or a parity bit
;
;
;
;RCSTA
;
;bit 7 SPEN: Serial Port Enable bit
;	1 = Serial port enabled
;	0 = Serial port disabled
;bit 6 RX9: 9-Bit Receive Enable bit
;	1 = Selects 9-bit reception
;	0 = Selects 8-bit reception
;bit 5 SREN: Single Receive Enable bit
;	Asynchronous mode:
;	Don’t care.
;	Synchronous mode – Master:
;	1 = Enables single receive
;	0 = Disables single receive
;	This bit is cleared after reception is complete.
;	Synchronous mode – Slave:
;	Don’t care.
;bit 4 CREN: Continuous Receive Enable bit
;	Asynchronous mode:
;	1 = Enables receiver
;	0 = Disables receiver
;	Synchronous mode:
;	1 = Enables continuous receive until enable bit, CREN, is cleared (CREN overrides SREN)
;	0 = Disables continuous receive
;bit 3 ADDEN: Address Detect Enable bit
;	Asynchronous mode 9-Bit (RX9 = 1):
;	1 = Enables address detection, enables interrupt and loads the receive buffer when RSR<8> is set
;	0 = Disables address detection, all bytes are received and ninth bit can be used as parity bit
;	Asynchronous mode 8-Bit (RX9 = 0):
;	Don’t care.
;bit 2 FERR: Framing Error bit
;	1 = Framing error (can be cleared by reading RCREGx register and receiving next valid byte)
;	0 = No framing error
;bit 1 OERR: Overrun Error bit
;	1 = Overrun error (can be cleared by clearing bit, CREN)
;	0 = No overrun error
;bit 0 RX9D: 9th Bit of Received Data
;	This can be address/data bit or a parity bit and must be calculated by user firmware.
;
;
;Clock Frequence Determination
;	For a device with FOSC of 16 MHz, desired baud rate of 9600, Asynchronous mode, 8-bit BRG:
;	Desired Baud Rate = FOSC/(64 ([SPBRGH:SPBRG] + 1))
;	Solving for SPBRGH:SPBRG:
;	X = ((FOSC/Desired Baud Rate)/64) – 1
;	= ((16000000/9600)/64) – 1
;	= [25.042] = 25
;	Calculated Baud Rate = 16000000/(64 (25 + 1))
;	= 9615
;	Error = (Calculated Baud Rate – Desired Baud Rate)/Desired Baud Rate
;	= (9615 – 9600)/9600 = 0.16%
;
;

;---------------------------------------------------------------------

init_232
	movlw    B'10100100'        ;initialize USART
    movwf    TXSTA            	;8-bit, Async, High Speed, tx enable
;   movlw    .25
;	movlw	.26
	movlw    .103				;9600 Baud
    movwf    SPBRG            	;into 8bit baud rate generator
    movlw    B'10010000'
    movwf    RCSTA				;Enable Port SPEN&CREN
	return

;	• SPEN (RCSTA<7>) bit must be set ( = 1),
;	• TRISC<6> bit must be set ( = 1), and
;	• TRISC<7> bit must be set ( = 1).



;---------------------------------------------------------------------
;
;
;---------------------------------------------------------------------
send_rs232
	movlw    .170            ;Move "AA" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

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