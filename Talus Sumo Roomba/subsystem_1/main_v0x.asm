;------------------------------------------------------------
;SumoRoomba Subsystem PIC by Charles M Douvier
;PIC16F690
;VERSION=0.10
;filename=main_v0x.asm
;
;This susbsystem will:
;- handle power switch, start up timer and initialization
;- tacometer input and math
;- watch Vbatt and Ibatt 
;- potiential selectable bumper/cliff sensor input
;- bluetooth communication to monitoring PC
;POWER METERING
;...ditched LTC2990 I2C support (well start of what is there) to leave it for future integration
;TACHOMETER
;...on the back burner is encoder tachometer integration 
;update: tachometer to be put in alternate processor and read by voltage 0-3.3V on AN4, AN5
;Likely have to add 2x12LFxxx (something w/ CCP and Vout) 1 for each CCP for the encoders
;(60sec*(1/Fosc*4))/(Period*Interruptions per 360deg)
;
;
;v0.01	1/1/13		just enough to compile nothing
;v0.02				hello world (toggling port outputs, ditched ISM and misc code from i2c)
;v0.03  			Extensive Port Setup, Added core RS-232 code.
;v0.04  			added core A2D code, not complete.
;v0.05				TX->>232 works.
;v0.06				AN4 ->> Sucessful 0-3.3v = 0-100 output
;v0.07	1/5/2013	tweaked to work with VB app, fixed resetting bug (on dly)
;v0.08	1/6/2013	cliff sensors work/right not wired though (require calibration replacement for DIP)
;v0.10	1/7/2013	power_button enables power_en; cliff sensors communicate. 1st potiential release
;
;To Do:
;
;2. send status on poll (grab status from 18F2331)
;power polling
;LED status and power on control 
;AN5 0-5v --> send rx232
;bumper polling ... make room?
;bumper logic
;I2C power logic someday..
;
;------------------------------------------------------------
	errorlevel    -302 	;kill those annoying bank messages
;------------------------------------------------------------
;NOTES
;
;Pin Function Diagram 
;
;RA0 I	PGD										RC0 I	AN4/LTACOMTR_AI	
;RA1 I	PGC										RC1 I	AN5/RTACOMTR_AI
;RA2 O	POWER_EN								RC2 I	ATTACK_MODE (from 18F2331)
;RA3 I	|MCLR/VPP								RC3 I 	POWER_SW
;RA4 O	LED_1			RB4 O	SDA				RC4 I 	LR-CLFSENS
;RA5 O	HeartBeat LED	RB5 I	RX232			RC5 I 	LF-CLFSENS
;						RB6 O	SDL				RC6 I 	RF-CLFSENS
;						RB7 I	TX232			RC7 I 	RR-CLFSENS
;
;---------------------------------------------------------------------
; Include Files and Configuration
; No Clock, No WDT, No Code Protection, No Brownout Detection	
; _MCLRE_OFF?
;---------------------------------------------------------------------
#include <p16f690.inc> 
;
   __config (_INTRC_OSC_NOCLKOUT & _WDT_OFF & _PWRTE_ON & _MCLRE_OFF & _CP_OFF & _BOR_OFF & _IESO_OFF & _FCMEN_OFF)

;---------------------------------------------------------------------
; Variable declarations
;---------------------------------------------------------------------
	udata
d1 res 1		;delay
d2 res 1		;delay
d3 res 1		;delay

	cblock	0x20
		myflags
		dly0
		CVD00
		CVD01
		CVD02
		CVD03
		CVD04
		CVD05
		ADHIGH
		ADLOW
		CLFSEN
	endc

;---------------------------------------------------------------------
;Constant Definitions
;---------------------------------------------------------------------
#define mode myflags,0
#define attack_mode myflags,1
#define	LFCS	CLFSEN,0
#define LRCS	CLFSEN,1
#define	RFCS	CLFSEN,2
#define RRCS	CLFSEN,3
#define	LBUMP	CLFSEN,4
#define RBUMP	CLFSEN,5
	
;#define NODE_ADDR 0x02 ; I2C address of this node

;---------------------------------------------------------------------
; Vectors -- Whats your vector victor?
;---------------------------------------------------------------------

    org 0
	nop
	goto 	start
	org 4
	bsf	attack_mode
	goto start



;---------------------------------------------------------------------
;init_ports
;Initializes ports and set up the basics
;
;RP1 RP0
;0 0 ? Bank 0 is selected
;0 1 ? Bank 1 is selected
;1 0 ? Bank 2 is selected
;1 1 ? Bank 3 is selected
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
		CLRF 	PORTA
		movlw 	0x0B		;0000 1011
		MOVWF	TRISA
							;PORTB
		movlw	0xF0		;1111 0000
		MOVWF	TRISB
							;PORTC
		movlw	0xFF		;1111 1111
		MOVWF	TRISC
							;A2D Setup
		MOVLW b'01110000' 	;A/D clock from internal clk
		MOVWF ADCON1

		bcf      STATUS,RP0     ;Bank 0


		;RS-232 initialization
		;Port Map
		;		bit7	bit6	bit5	bit4	bit3	bit2	bit1	bit0
		;TXSTA 	CSRC 	TX9 	TXEN 	SYNC 	SENDB 	BRGH 	TRMT 	TX9D 	0000 0010
		;RCSTA 	SPEN 	RX9 	SREN 	CREN 	ADDEN 	FERR 	OERR 	RX9D 	0000 000X
		;
		;TXSTA
		;
		;bit 7 CSRC: Clock Source Select bit
		;	Asynchronous :
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
	;The PIC16F690 has one or two quirks that are only hinted about in the EUSART section. One is that RX input is shared with AN11. If you don't set ANSELH bit 3 to zero the receiver never works. 
	;• SPEN (RCSTA<7>) bit must be set (= 1),
	;• TRISB<6> bit must be set (= 1), and
	;• TRISB<7> bit must be set (= 1).

		bsf	STATUS, RP0				;Bank 1
		bcf	STATUS,	RP1
		;bsf	BAUDCTL,3

		movlw    b'10100100'        ;initialize USART
	    movwf    TXSTA            	;8-bit, Async, High Speed, tx enable								
									;9615bps 
        movlw 0x19              	; 0x19=9600 bps (0x0C=19200 bps)
        movwf SPBRG		


		bcf	STATUS, RP0				;Bank 0
		bcf	STATUS,	RP1

	    movlw    b'10010000'
	    movwf    RCSTA				;Enable Port SPEN&CREN


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

		BSF STATUS,RP0 ;Bank 1
		BCF STATUS,RP1 ;
		
		MOVLW B'01110000' ;A/D clock from internal clk
		MOVWF ADCON1 ;
		
		BCF STATUS,RP0 			;Bank 2
		BSF STATUS,RP1 			;
		
		BSF ANSEL,4 			;Set RA4 to analog
		BSF ANSEL,5 			;Set RA5 to analog
		
		BCF STATUS,RP0 			;Bank 0
		BCF STATUS,RP1

		

		return



;---------------------------------------------------------------------
;TX 232  W -->
;		send whatever is in W to the BlueTooth Module			
;RP1 RP0
;0 0 ? Bank 0 is selected
;0 1 ? Bank 1 is selected		
;---------------------------------------------------------------------
send_232
		bcf	STATUS, RP0				;Bank 0
		bcf	STATUS,	RP1
									;Move W to TXREG
		movwf    TXREG  
									;
		bsf	STATUS, RP0				;Bank 1
		bcf	STATUS,	RP1

	    btfss    TXSTA,TRMT     	;wait for data TX
	    goto    $-1

		bcf	STATUS, RP0				;Bank 0
		bcf	STATUS,	RP1

		return

;---------------------------------------------------------------------
;RX 232 --> W
;			edit to skip and not wait
;		READ FROM BT (If I even use this.. ? testing?)
;---------------------------------------------------------------------

receive_232
		btfss PIR1,RCIF         ; (5) check for received data
        goto receive_232

        movf RCREG,W            ; save received data in W
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
        return

;---------------------------------------------------------------------
;AN4_READ
;
;---------------------------------------------------------------------
AN4_READ
;
;;This code block configures the A/D
;for polling, Vdd reference, R/C clock
;and RA0 input.
;

		BCF STATUS,RP0 			;Bank 0
		BCF STATUS,RP1

		MOVLW B'00010001' 		;left, Vdd Vref, AN4, turn on
		MOVWF ADCON0 ;

		BSF ADCON0,GO ;Start conversion
		BCF STATUS,RP1 ;
		BTFSC ADCON0,GO ;Is conversion done?
		GOTO $-1 ;No, test again
		MOVF ADRESH,W 				;Read upper bits
		MOVWF ADLOW ;
;		BSF STATUS,RP0 				;Bank 1
;		MOVF ADRESL,W ;Read lower bits
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
;		MOVWF ADLOW
									;ready to convert
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
		return

;---------------------------------------------------------------------
;AN5_READ
;		Read A2D on AN5
;---------------------------------------------------------------------
AN5_READ
;
;;This code block configures the A/D
;for polling, Vdd reference, R/C clock
;and RA0 input.
;
		BCF STATUS,RP0 ;Bank 0
		BCF STATUS,RP1

		MOVLW B'00010101' 		;left, Vdd Vref, AN5, turn on
		MOVWF ADCON0 ;

		BSF ADCON0,GO ;Start conversion
		BCF STATUS,RP1 ;
		BTFSC ADCON0,GO ;Is conversion done?
		GOTO $-1 ;No, test again
		MOVF ADRESH,W 				;Read upper bits
		MOVWF ADLOW ;
;		BSF STATUS,RP0 				;Bank 1
;		MOVF ADRESL,W ;Read lower bits
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
;		MOVWF ADLOW
									;ready to convert
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
		return

;---------------------------------------------------------------------
;A2D Stuff CVDEC Converts A2D Values to Decimal
;0-Vdd = 0-200
;---------------------------------------------------------------------


CVDEC

;    clear receiving bytes

	CLRF	CVD00
	CLRF	CVD01
	CLRF	CVD02
	CLRF	CVD03
	CLRF	CVD04
	CLRF	CVD05
;  for bit 0     50
CVDEC00
	BTFSS	ADLOW,07
	GOTO	CVDEC01
	MOVLW	D'05'
	ADDWF	CVD01,F
;  for bit 1     25

CVDEC01
	BTFSS	ADLOW,06
	GOTO	CVDEC02

	MOVLW	D'02'
	ADDWF	CVD01,F

	MOVLW	D'05'
	ADDWF	CVD02,F



;  for bit 2     12.5

CVDEC02
	BTFSS	ADLOW,05
	GOTO	CVDEC03

	MOVLW	D'01'
	ADDWF	CVD01,F

	MOVLW	D'02'
	ADDWF	CVD02,F

	MOVLW	D'05'
	ADDWF	CVD03,F



;  for bit 3     6.25

CVDEC03
	BTFSS	ADLOW,04
	GOTO	CVDEC04

	MOVLW	D'06'
	ADDWF	CVD02,F

	MOVLW	D'02'
	ADDWF	CVD03,F

	MOVLW	D'05'
	ADDWF	CVD04,F



;  for bit 4     3.125

CVDEC04
	BTFSS	ADLOW,03
	GOTO	CVDEC05

	MOVLW	D'03'
	ADDWF	CVD02,F

	MOVLW	D'01'
	ADDWF	CVD03,F

	MOVLW	D'02'
	ADDWF	CVD04,F

	MOVLW	D'05'
	ADDWF	CVD05,F



;  for bit 5     1.5625

CVDEC05
	BTFSS	ADLOW,02
	GOTO	CVDEC06

	MOVLW	D'01'
	ADDWF	CVD02,F

	MOVLW	D'05'
	ADDWF	CVD03,F

	MOVLW	D'06'
	ADDWF	CVD04,F

	MOVLW	D'03'
	ADDWF	CVD05,F



;  for bit 6     0.78125

CVDEC06
	BTFSS	ADLOW,01
	GOTO	CVDEC07

	MOVLW	D'07'
	ADDWF	CVD03,F

	MOVLW	D'08'
	ADDWF	CVD04,F

	MOVLW	D'01'
	ADDWF	CVD05,F



;  for bit 7     0.390625

CVDEC07
	BTFSS	ADLOW,00
	GOTO	CVDEC08

	MOVLW	D'03'
	ADDWF	CVD03,F

	MOVLW	D'09'
	ADDWF	CVD04,F

	MOVLW	D'01'
	ADDWF	CVD05,F


CVDEC08
	MOVLW	D'10'

;   adjust digit  5

	INCF	CVD04,F
	SUBWF	CVD05,F
	BTFSC	STATUS,C
	GOTO	$-3

	;  went past zero

	DECF	CVD04,F
	ADDWF	CVD05,F

;   adjust digit  4

	INCF	CVD03,F
	SUBWF	CVD04,F
	BTFSC	STATUS,C
	GOTO	$-3

	;  went past zero

	DECF	CVD03,F
	ADDWF	CVD04,F

;   adjust digit  3

	INCF	CVD02,F
	SUBWF	CVD03,F
	BTFSC	STATUS,C
	GOTO	$-3

	;  went past zero

	DECF	CVD02,F
	ADDWF	CVD03,F

;   adjust digit  2

	INCF	CVD01,F
	SUBWF	CVD02,F
	BTFSC	STATUS,C
	GOTO	$-3

	;  went past zero

	DECF	CVD01,F
	ADDWF	CVD02,F

;   adjust digit  1

	INCF	CVD00,F
	SUBWF	CVD01,F
	BTFSC	STATUS,C
	GOTO	$-3

	;  went past zero

	DECF	CVD00,F
	ADDWF	CVD01,F
;  no need to adjust high order byte
;   convert to ascii

	MOVLW	D'48'

	ADDWF	CVD00,F
	ADDWF	CVD01,F
	ADDWF	CVD02,F
	ADDWF	CVD03,F
	ADDWF	CVD04,F
	ADDWF	CVD05,F
;	max value a/d is 99.61
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
	return

;---------------------------------------------------------------------
; leds
;---------------------------------------------------------------------	
led_on
		;if on display 5 second count in binary on LED1-3
;		BCF	PORTA,4
		nop
		return

led_off
;		BSF	PORTA,4
		nop
		return

;---------------------------------------------------------------------
; poll_18f (RC2)
;in attack mode? 0=search 1=attack
;---------------------------------------------------------------------
poll_18f
		BTFSC	PORTC,2
		bsf		attack_mode
		BTFSS	PORTC,2
		bcf		attack_mode
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
		return

;---------------------------------------------------------------------
; poll_power (RC3)
;---------------------------------------------------------------------
poll_power	
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1

		BTFSS PORTC,3
		bsf	mode
		NOP
;		BCF	PORTA,2					;Tell that free loadn' 18F2331 it's time to dance!
		btfsc	mode
		BSF	PORTA,2
		NOP		

		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
		return

;---------------------------------------------------------------------
; send232_status
;---------------------------------------------------------------------
send232_status
						;send + mode attack +
		movlw	'+'
		call	send_232
		btfsc	mode
		movlw	'1'
		btfss	mode
		movlw	'0'
		call	send_232
		btfsc	attack_mode
		movlw	'1'
		btfss	attack_mode
		movlw	'0'
		call	send_232
		movlw	'+'
		call	send_232
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
		return

;---------------------------------------------------------------------
; send232_adc
;drop the contents of the converted ADC info to RS232
;---------------------------------------------------------------------
send_adc

		call	CVDEC
;		movfw	CVD00
;		call	send_232
		movfw	CVD01
		call	send_232
		movfw	CVD02
		call	send_232
		movfw	CVD03
		call	send_232
		movfw	CVD04
		call	send_232
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
		return



;---------------------------------------------------------------------
; poll_bumpers
;.. polling the bumpers 
;---------------------------------------------------------------------

poll_bumpers
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1

		CLRF	CLFSEN
		btfsc	PORTC,5			;RC5 Left Front Cliff Sensor
		bsf		LFCS
		nop				

		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
		return


;---------------------------------------------------------------------
; poll_cliffsensors	FUTURE USE? NOT USED NOW.
;.. a start/polls the cliff sensors
;---------------------------------------------------------------------
poll_cliffsensors
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1

		CLRF	CLFSEN
;		btfsc	PORTC,5			;RC5 Left Front Cliff Sensor
;		bsf		LFCS
;		nop
;		btfsc	PORTC,4			;RC4 Left Rear Cliff Sensor
;		bsf		LRCS
;		nop
;		btfsc	PORTC,6			;RC6 Right Front Cliff Sensor
;		bsf		RFCS
;		nop
;		btfsc	PORTC,7			;RC7 Right Rear Cliff Sensor
;		bsf		RRCS
		nop

		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
		return
;---------------------------------------------------------------------
; Main Code
;---------------------------------------------------------------------
start
		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		call	init_ports
		clrf	myflags
		CLRF	ADHIGH		;not used right now
main
		NOP

		BCF STATUS,RP0				;Bank 0
		BCF STATUS,RP1

		CLRF	CLFSEN				;reset cliff sensor status

		call	poll_power		;power switch pressed?
			
;		btfsc	mode
;		call	led_on

;		btfss	mode
;		call	led_off

		bsf		PORTA,5			;visual heatbeat/turn on LED

		call	send232_status	;update monitoring software   +00+ xxxx xxxx xxxx xxxx

		call	AN4_READ		;read tachometer and send via BT  xxxx 999xxx xxxx xxxx xxxx
		call	send_adc

		call	AN5_READ	 	;read tachometer and send via BT  xxxx xxx999 xxxx xxxx xxxx
		call	send_adc

		;call	poll_battery	;future battery monitoring
		movlw	'0'				;send 00.0v for now
		call	send_232
		movlw	'0'
		call	send_232
		movlw	'0'
		call	send_232
		movlw	'0'				;also send 0.0 amps
		call	send_232
		movlw	'0'
		call	send_232

		call	poll_bumpers		;this is technicially disabled.

		movlw	'0'					;send no Left Bumper
		btfsc	LBUMP				;
		movlw	'1'
		call	send_232

		movlw	'0'					;send no Right Bumper
		btfsc	RBUMP				;
		movlw	'1'
		call	send_232			

		CLRF	CLFSEN
		call	poll_cliffsensors	;any cliffs?
		
		movlw	'0'			
		btfsc	LRCS				;LRCS
		movlw	'1'
		nop				
		call	send_232
	
		movlw	'0'	
		btfsc	LFCS				;LFCS
		movlw	'1'
		nop				
		call	send_232

		movlw	'0'					;FRCS
		btfsc	RFCS				;FRCS
		movlw	'1'
		call	send_232

		movlw	'0'					;RRCS
		btfsc	RRCS				;RRCS
		movlw	'1'
		call	send_232

		movlw	'-'					;"done" bit
		call	send_232

		movlw	0x0D	;CR

		call	send_232

		call	poll_18f	;poll for bot's mode (seek/attack)

		bcf	PORTA,5			;visual heartbeat/turn off LED
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		clrf dly0
		decfsz dly0 ,f
		goto $-1
		nop
		goto main
	end