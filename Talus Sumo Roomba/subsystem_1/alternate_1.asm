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
;v0.03  Extensive Port Setup, Added core RS-232 code.
;v0.04  added core A2D code, not complete.
;
;(60sec*(1/Fosc*4))/(Period*Interruptions per 360deg)
;
;To Do:
;
;RS-232, 9600 baud, test to BT
;1. send "alive" heartbeat on wake-up (should pair BT?)
;2. send status on poll
;
;power polling
;LED status and power on control 
;
;AN4,5 0-5v --> send rx232
;bumper polling ... make room?
;bumper logic
;
;------------------------------------------------------------

;------------------------------------------------------------
;NOTES
;
;Pin Function Diagram 
;
;RA0 I	PGD						RC0 I	AN4/LTACOMTR_AI	
;RA1 I	PGC						RC1 I	AN5/RTACOMTR_AI
;RA2 O	POWER_EN					RC2 I	BOT_MODE (from 18F2331)
;RA3 I	|MCLR/VPP					RC3 I 	POWER_SW
;RA4 O	LED_1		RB4 O	LED_2			RC4 I 	LR-CLFSENS
;RA5 O	HeartBeat LED	RB5 I	RX232			RC5 I 	LF-CLFSENS
;			RB6 O	LED_3			RC6 I 	RF-CLFSENS
;			RB7 O	TX232			RC7 I 	RR-CLFSENS
;
;---------------------------------------------------------------------
; Include Files and Configuration
;	No Clock, No WDT, No Code Protection, No Brownout Detection	
;_MCLRE_OFF?
;---------------------------------------------------------------------
#include <p16f690.inc> 
;
   __config (_INTRC_OSC_NOCLKOUT & _WDT_OFF & _PWRTE_OFF & _MCLRE_OFF & _CP_OFF & _BOR_OFF & _IESO_OFF & _FCMEN_OFF)
;---------------------------------------------------------------------
;Constant Definitions, for when I use I2C
;---------------------------------------------------------------------
;#define NODE_ADDR 0x02 ; I2C address of this node
;---------------------------------------------------------------------
; Buffer Length Definition
;---------------------------------------------------------------------
;#define RX_BUF_LEN 32 ; Length of receive buffer
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

cblock	0x20
	mode
endc

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
;Initializes ports and set up the basics
;---------------------------------------------------------------------
init_ports	

		BCF STATUS,RP0 		;Bank 0
		BCF STATUS,RP1 		;

		CLRF	PORTA
		CLRF	PORTB
		CLRF 	PORTC

		BSF STATUS,RP1 		;Bank 2

		movlw	0x30		;00110000
		movwf 	ANSEL		;Enable AN4,5--Rest Digital I/O
		CLRF	ANSELH		;Mostly to kill aN11 for RS-232 but I'm not using the other AIs either

		BSF STATUS,RP0 		;Bank 1
		
					;PORTA
		movlw 	0x0B
		MOVWF	TRISA
					;PORTB
		movlw	0x20
		MOVWF	TRISB
					;PORTC
		movlw	0xFF
		MOVWF	TRISC
					;A2D Setup
		MOVLW B’01110000’ ;A/D clock from internal clk
		MOVWF ADCON1

		bcf      STATUS,RP0     ;Bank 0

		;add in RS232 port config
	

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
;The PIC16F690 has one or two quirks that are only hinted about in the EUSART section. One is that RX input is shared with AN11. If you don't set ANSELH bit 3 to zero the receiver never works. 
;---------------------------------------------------------------------

	init_232
		movlw    B'10100100'        ;initialize USART
	    movwf    TXSTA            	;8-bit, Async, High Speed, tx enable
	    movwf    SPBRG            	;into 8bit baud rate generator
	    movlw    B'10010000'
	    movwf    RCSTA				;Enable Port SPEN&CREN
		return


;• SPEN (RCSTA<7>) bit must be set (= 1),
;• TRISB<6> bit must be set (= 1), and
;• TRISB<7> bit must be set (= 1).


;---------------------------------------------------------------------
;TX 232  W -->
;		send whatever is in W to the BlueTooth Module					
;---------------------------------------------------------------------
	send_rs232
	;	movlw    .170            ;Move "AA" to TXREG
	movwf    TXREG  
    	btfss    TXSTA,TRMT		
    	goto    $-2


;---------------------------------------------------------------------
;RX 232 --> W
;			edit to skip and not wait
;		READ FROM BT (If I even use this.. ? testing?)
;---------------------------------------------------------------------

	receive btfss PIR1,RCIF         ; (5) check for received data
        goto receive

        movf RCREG,W            ; save received data in W
        return

;---------------------------------------------------------------------
;AN4_READ
;			make this happen and dup for 5
;
;1. Configure the A/D module:
;• Configure analog/digital I/O (ANSx)
;• Select A/D conversion clock (ADCON1<6:4>)
;• Configure voltage reference (ADCON0<6>)
;• Select A/D input channel (ADCON0<5:2>)
;• Select result format (ADCON0<7>)
;• Turn on A/D module (ADCON0<0>)
;2. Configure A/D interrupt (if desired):
;• Clear ADIF bit (PIR1<6>)
;• Set ADIE bit (PIE1<6>)
;• Set PEIE and GIE bits (INTCON<7:6>)
;3. Wait the required acquisition time.
;4. Start conversion:
;• Set GO/DONE bit (ADCON0<1>)
;5. Wait for A/D conversion to complete, by either:
;• Polling for the GO/DONE bit to be cleared
;(with interrupts disabled); OR
;• Waiting for the A/D interrupt
;6. Read A/D Result register pair
;(ADRESH:ADRESL), clear bit ADIF if required.
;7. For next conversion, go to step 1 or step 2 as
;required. The A/D conversion time per bit is
;defined as TAD. A minimum wait of 2 TAD is
;required before the next acquisition starts.
;---------------------------------------------------------------------
AN4_READ
;
;;This code block configures the A/D
;for polling, Vdd reference, R/C clock
;and RA0 input.
;
;Conversion start & wait for complete
;polling code included.
;
;setup stuff
;BSF STATUS,RP0 ;Bank 1
;BCF STATUS,RP1 ;
;MOVLW B’01110000’ ;A/D clock from internal clk
;MOVWF ADCON1 ;
;BCF STATUS,RP0 ;Bank 2
;BSF STATUS,RP1 ;
;BSF ANSEL,4 ;Set RA4 to analog
;
;select AN4
;BCF STATUS,RP0 ;Bank 0
;MOVLW B’00010001’ ;left, Vdd Vref, AN4, turn on
;MOVWF ADCON0 ;
;and go...
;CALL SampleTime ;Wait min sample time
;BSF ADCON0,GO ;Start conversion
;BCF STATUS,RP1 ;
;BTFSC ADCON0,GO ;Is conversion done?
;GOTO $-1 ;No, test again
;MOVF ADRESH,W ;Read upper 8 bits
;MOVWF RESULTHI ;
;BSF STATUS,RP0 ;Bank 1
;MOVF ADRESL,W ;Read lower 2 bits
;BCF STATUS,RP0 ;Bank 0
;MOVWF RESULTLO
;
		return
;---------------------------------------------------------------------
; toggle_state
;	turns on/off mode of 18F2331
;---------------------------------------------------------------------
	toggle_state
		;if on turn off else turn on
		btfsc	mode
		bcf	mode	
		bsf	mode
		return	
;---------------------------------------------------------------------
; turn_off_leds
;	shut'em down.
;	LED_1, RA4, LED_2, RB4, LED_3 = RB6
;---------------------------------------------------------------------
	turn_off_leds
		BCF	PORTA,4
		BCF	PORTB,4
		BCF	PORTB,6	
		return

;---------------------------------------------------------------------
; power_leds
;---------------------------------------------------------------------
	power_leds
		;if on display 5 second count in binary on LED1-3
		btfsc	mode
		call	turn_off_leds
		return

;---------------------------------------------------------------------
; poll_power (RC3)
;---------------------------------------------------------------------
	poll_power	
		BTFSS PORTC,3			;button pressed?
		call toggle_state
		call power_leds
		return

;---------------------------------------------------------------------
; send232_status
;---------------------------------------------------------------------
	send232_status
		;send M+mode
		return


;---------------------------------------------------------------------
; Main Code
;---------------------------------------------------------------------
start
		call	init_ports
Main 
		call	poll_power
		bsf	PORTA,5		;visual heatbeat

		;call	poll_bumpers			
		;call	poll_cliffsensors
		call	AN4_READ	;read tachometer and send via BT	[
;		call	AN5_READ 

		call dly

		call	send232_status	;update monitoring software
;		call	poll_battery	;future battery monitoring
		bcf	PORTA,5		;visual heartbeat
		

		call dly
		goto Main ; Loop forever.

	end