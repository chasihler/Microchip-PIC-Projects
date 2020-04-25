;---------------------------------------------------------------------------------------------
;Roomba Main Board Controller by Charles M Douvier
;Robot Name: Talos
;version=0.08
;
;Roomba Sumo Bot Firmware for main board to operator motors in responce to sensors
;for sumo competition.
;
;Device PIC 18F2331
;
;v0.01	01/14/13	Part on board.. first .asm save.
;v0.02	01/16/13	Blinking LED "Hello World" (Had A_VDD at 0v.. wouldn't prgm
;v0.03	01/18/13	Start of RS-232 code, proper port commenting and initialization, 
;					commenting and routinue structure
;v0.04	01/18/2013	Initialization of serial rs232 motor controller.
;v0.05	01/20/2013	motor commands
;v0.06	2/7/2013	Power_En wait w/ delay (2 seconds right now)
;v0.07	2/7/2013	switch and LED output properly sequence.. just turning in circles for now.
;v0.08	2/12/2013	alpha, seek only
;
;
;	PIN DIAGRAM
;
; 	RA0	O	|Seek/Attack	RB0	O	LTACH			RC0 I	_______				
;	RA1	O	LED1			RB1	O	RTACH			RC1 I	CCP2
;	RA2	O	LED2			RB2	I	LFCS			RC2 I	CCP1
;	RA3	O	LED3			RB3	I	LRCS			RC3 I 	INT0			RE3 I	/MCLR|Vpp
;	RA4	I	POWER_EN		RB4	I	RFCS			RC4 I	LBUMP
;	RA5	I	______			RB5	I	RRCS			RC5 I	RBUMP
;	RA6 I	XTAL 16MHZ		RB6	I	PGM/PGC			RC6	I(O)RS232 TX
;	RA7 I	XTAL			RB7	I	PGM/PGD			RC7	I	RS232 RX	
;
;
;
;
;TODO Reseach "new" way to write configure and upgrade code
;
;	Bumper Inputs
;	Cliff Sensors 
;	Poll Them
;	React to them 
;	Tachometers --> PWM out x2
;
;
;---------------------------------------------------------------------------------------------
	errorlevel -230

	LIST R=DEC

#include "p18f2331.inc"



	__CONFIG _CONFIG1H, _IESO_ON_1H & _FCMEN_OFF_1H & _OSC_HS_1H 
	__CONFIG _CONFIG2L, _PWRTEN_OFF_2L & _BOREN_OFF_2L
	__CONFIG _CONFIG2H, _WDTEN_OFF_2H
	__CONFIG _CONFIG3H, _MCLRE_ON_3H
	__CONFIG _CONFIG4L, _DEBUG_OFF_4L & _LVP_OFF_4L & _STVREN_OFF_4L
	__CONFIG _CONFIG5L, _CP0_OFF_5L & _CP1_OFF_5L
	__CONFIG _CONFIG5H, _CPB_OFF_5H & _CPD_OFF_5H
	__CONFIG _CONFIG6L, _WRT0_OFF_6L & _WRT1_OFF_6L
	__CONFIG _CONFIG6H, _WRTC_OFF_6H & _WRTB_OFF_6H & _WRTD_OFF_6H
	__CONFIG _CONFIG7L, _EBTR0_OFF_7L & _EBTR1_OFF_7L
	__CONFIG _CONFIG7H, _EBTRB_OFF_7H 

;_OSC_IRCIO_1H		


	cblock	0x20
		CNT1								;Delay Counter
		CNT2								;Delay Counter
		CNT3								;Delay Counter
		LEncCountH							;Left Encoder Val High
		LEncCountL							;Left Encoder Val Low	
		REncCountH							;Right Encoder Val High
		REncCountL							;Right Encoder Val Low
		temp_wr								;rs232 byte to send
		myflags								;cliff and bumper status
	endc
	
#define	cliff_left myflags,0				
#define	cliff_right myflags,1

	org 0
		NOP
		bra start
	
	org 8
		NOP
		;TODO INT0  Handler  tied to OR gate to bumpers/edge sensors
		;
	RETFIE		

;-------------------------------------------------------
;Reset Encoder Counters
;
;-------------------------------------------------------

init_ports

; 	PORTA
		CLRF	PORTA
		
		movwf	TRISA
		bsf		PORTA,3
;		MOVLW 0x3F 			; Configure A/D
		CLRF	ANSEL0
;		MOVWF ANSEL0 ; 		for digital inputs
		movlw	b'11110000'
							;MOVLW 0xCF ; Value used to
							; initialize data
							; direction
		MOVWF TRISA ; Set RA<3:0> as outputs

;	PORTB
		CLRF	PORTB
		movlw	0xFC		;<7:2> pins inputs, <1:0>  outputs
		movwf	TRISB
		clrf	LATB
		movlw	b'00000011' 
		movwf	PORTB
;	PORTC				
		CLRF	PORTC	
		MOVLW	0xFF		;RC0:7 inputs
		MOVWF	TRISC

		return


;-------------------------------------------------------
;Reset Encoder Counters
;
;-------------------------------------------------------

init_EncCount
		clrf	LEncCountH
		clrf	LEncCountL
		clrf	REncCountH
		clrf	REncCountL
		return	

;-------------------------------------------------------
;Delays
;
;-------------------------------------------------------

delay_1s:
		MOVLW d'5'
		MOVWF CNT1
LOOP1:
			MOVLW d'255'
			MOVWF CNT2
LOOP2:
			MOVLW d'255'
			MOVWF CNT3
LOOP3:
			DECFSZ CNT3,F
		GOTO LOOP3
			DECFSZ CNT2,F
		GOTO LOOP2
			DECFSZ CNT1,F
		GOTO LOOP1
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
;-------------------------------------------------------

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


;-------------------------------------------------------
;Initialize Motor Controller
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------


init_motor_controller
	movlw    .170            ;Move "AA" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

    return
;-----------------------------------------------------
;Motor Commands
;
;M0 = Left Motor
;M1 = Right Motor
;
;Command 0x88: Motor M0 Forward
;Compact protocol: 0x88, motor speed
;
;Command 0x8A: Motor M0 Reverse
;Compact protocol: 0x8A, motor speed
;
;Motor M1 Commands;
;
;Command 0x8C: Motor M1 Forward/140
;Compact protocol: 0x8C, motor speed
;
;
;
;Command 0x8E: Motor M1 Reverse
;Compact protocol: 0x8E, motor speed
;

;-------------------------------------------------------
;Right Motor Stop
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------
motor_right_stop			
							;0x87/135
	movlw    .135            ;Move "0x87" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	return
;-------------------------------------------------------
;Left Motor Stop
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------
motor_left_stop
							;0x86/134
	movlw    .134            ;Move "0x88" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2
	return

;-------------------------------------------------------
;Left Motor Forward Full Speed
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------


motor_left_fwd_full			;M0 0x88-0x7F
	movlw    .136            ;Move "0x88" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .127            ;Move 0x7F
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2

    return	

;-------------------------------------------------------
;Left Motor Forward "Normal" Speed
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;Full speed doesn't detect the cliff in time to reverse 
;and stop to turn around
;-------------------------------------------------------


motor_left_fwd_normal			;M0 0x88-0x7F
	movlw    .136            ;Move "0x88" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .85            ;Move 0x7F
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2

    return	

;-------------------------------------------------------
;Left Motor Forward Half Speed
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------


motor_left_fwd_half			;M0 0x88-0x3F
	movlw    .136            ;Move "0x88" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .63            ;Move 0x3F
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2

    return	


;-------------------------------------------------------
;Left Motor Reverse Half Speed
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------


motor_left_rev_half			;M0 0x88-0x3F
	movlw    .138            ;Move "0x88" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .63            ;Move 0x3F
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2

    return	

;-------------------------------------------------------
;Right Motor Forward "Normal" Speed
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;see M0 (left) notes
;-------------------------------------------------------


motor_right_fwd_normal			;M0 0x88-0x3F
	movlw    .140            ;Move "0x8C" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .85            ;Move 0x3F
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2

    return	

;-------------------------------------------------------
;Right Motor Forward Half Speed
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------


motor_right_fwd_half			;M0 0x88-0x3F
	movlw    .140            ;Move "0x8C" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .63            ;Move 0x3F
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2

    return	


;-------------------------------------------------------
;Right Forward Full Speed
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------


motor_right_fwd_full		;M0 0x8C-0x7F
	movlw    .140            ;Move "0x8C" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .127            ;Move 0x7F
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2

    return	

;-------------------------------------------------------
;Right Forward Quarter--ISH
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------

motor_right_fwd_quarter		;M0 0x8C-0x7F
	movlw    .140            ;Move "0x8C" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .40            ;
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2



;-------------------------------------------------------
;Right Motor Reverse Half Speed
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------


motor_right_rev_half			;M0 0x88-0x3F
	movlw    .142            ;Move "0x8E" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .63            ;Move 0x3F
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2

    return	


;-------------------------------------------------------
;Right Motor Reverse Full Speed
;Polou Serial Controller requires 'AA' before any other
;signal to detect baud/initialize
;
;-------------------------------------------------------


motor_right_rev_full			;M0 0x88-0x3F
	movlw    .142            ;Move "0x8E" to TXREG
	movwf    TXREG  
    btfss    TXSTA,TRMT		
    goto    $-2

	movlw    .127            ;Move 0x7F
	movwf    TXREG  
    btfss    TXSTA,TRMT
    goto    $-2

    return	

;-------------------------------------------------------
;Send RS-232 Data
;
;-------------------------------------------------------


rs232_write
    movff    temp_wr,TXREG
    btfss    TXSTA,TRMT
    goto    $-2
    return

;-------------------------------------------------------
;Response Routines
;All driving commands for seeking and attack
;
;|SEEKING
;drive_leftcliff - top right wheel, left reverse half speed 2.0(?) seconds; resume seek
;
;ATTACK_MODE
;
;-------------------------------------------------------
drive_leftcliff
	bsf		PORTA,1
	call	motor_right_stop
	call	motor_left_rev_half
	call	delay_1s
	call	delay_1s
	call	delay_1s
	call	delay_1s	
	call	delay_1s
	call	delay_1s
	call	delay_1s
	call	delay_1s
	call	delay_1s
	call	delay_1s
	call	delay_1s
	call	delay_1s	
	call	delay_1s
	call	delay_1s
	call	delay_1s
	call	delay_1s

	bcf		PORTA,1
	return

;-------------------------------------------------------
;All Polling
;
;poll_leftclfsen - cliff sensors triggered on the left?
;-------------------------------------------------------
poll_leftclfsen				;RB<2:3>
	BCF		cliff_left
	btfsc	PORTB,2			;clear means no IR reflection!
	bsf		cliff_left
	NOP
	btfsc	PORTB,3		
	bsf		cliff_left
	NOP	
		
	return


;-------------------------------------------------------
;Main Code
;
;-------------------------------------------------------
	
start
		call delay_1s			;let the power supply and such settle out..
		call delay_1s
		;initialization
		call	init_ports
		call	init_EncCount
		call	init_232
		call delay_1s
		call delay_1s
		call	init_motor_controller
		call delay_1s
		call delay_1s
		call delay_1s

		bcf		PORTA,1			;LED_1 Off
		bcf		PORTA,2			;LED_2 Off
		bcf		PORTA,3			;LED_3 Off
		
wait	BTFSS	PORTA,4			;wait for power_en
		goto $-2
		NOP
		bsf		PORTA,3			;001
		call delay_1s
		call delay_1s
		call delay_1s
		call delay_1s
		bcf		PORTA,3			;010
		bsf		PORTA,2			
		call delay_1s
		call delay_1s
		call delay_1s
		call delay_1s
		bsf		PORTA,3			;011
		call delay_1s
		call delay_1s
		call delay_1s
		call delay_1s
		bcf		PORTA,2
		bcf		PORTA,3			;100
		bsf		PORTA,1
		call delay_1s
		call delay_1s
		call delay_1s
		call delay_1s
		bsf		PORTA,3			;101
		call delay_1s
		call delay_1s
		call delay_1s
		call delay_1s
		bcf		PORTA,1
		bcf		PORTA,3
		bsf		PORTA,2


;EDIT A LOT BELOW	
		;call	motor_left_fwd_half
		;call	motor_right_fwd_full
		;call	motor_left_rev_half
		;main loop
loop
			
		NOP
		call	poll_leftclfsen
		btfss	cliff_left				;if cliff lets avoid it!
		call	drive_leftcliff
		nop

		call	motor_left_fwd_normal
		call	motor_right_fwd_normal
		goto loop



	end
