;---------------------------------------------------------------------------------------------
;Roomba Main Board Controller by Charles M Douvier
;
;Roomba Sumo Bot Firmware for main board to operator motors in responce to sensors
;for sumo competition.
;
;Device PIC 18F2331
;
;v0.01	01/14/13	Part on board.. first .asm save.
;v0.02	01/16/13	Blinking LED "Hello World" (Had A_VDD at 0v.. wouldn't prgm
;v0.03	01/18/13	Start of RS-232 code, commenting and routinue structure
;
;
;	PIN DIAGRAM
;
; 	RA0	O	x				RB0	I	x				RC0 I	x				
;	RA1	O	x				RB1	I	x				RC1 I	x
;	RA2	O	x				RB2	I	x				RC2 I	x
;	RA3	I	x				RB3	I	x				RC3 I 	x				RE3 I	/MCLR|Vpp
;	RA4	I	x				RB4	I	x				RC4 I	x
;	RA5	I	x				RB5	I	x				RC5 I	x
;	RA6 I	XTAL 16MHZ		RB6	I	PGM/PGC			RC6	O	RS232 TX
;	RA7 I	XTAL			RB7	I	PGM/PGD			RC7	I	RS232 RX	
;
;---------------------------------------------------------------------------------------------


	LIST R=DEC

#include "p18f2331.inc"


;TODO Reseach "new" way to write configure and upgrade code

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
		CNT1
		CNT2
		CNT3
		LEncCountH
		LEncCountL	
		REncCountH
		REncCountL
	endc
	
	org 0
		NOP
		bra start
	
	org 8
		NOP
	RETFIE		

;-------------------------------------------------------
;Reset Encoder Counters
;
;-------------------------------------------------------

init_ports
		movlw	0xCE		;PGM and ADC pins inputs, others are outputs
		movwf	TRISB
		clrf	LATB
		movlw	b'00000011' 
		movwf	PORTB
		clrf	TRISA
		clrf	LATA
		movlw	b'00011111' 
		movwf	PORTA
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
;
;-------------------------------------------------------

init_232
	movlw    B'10100100'        ;initialize USART
    movwf    TXSTA            ;8-bit, Async, High Speed
    movlw    .25
    movwf    SPBRG            ;9.6kbaud @ 4MHz
    movlw    B'10010000'
    movwf    RCSTA
	return

;	• SPEN (RCSTA<7>) bit must be set ( = 1),
;	• TRISC<6> bit must be set ( = 1), and
;	• TRISC<7> bit must be set ( = 1).

;-------------------------------------------------------
;Send RS-232 Data
;
;-------------------------------------------------------


rs232_write
    movff    temp_wr,TXREG
    btfss    TXSTA,TRMT
    goto    $-2
    return

;	spare tidbits...
;    movlw    "\r"            ;move data into TXREG
;    movwf    TXREG            ;carriage return
;    btfss    TXSTA,TRMT        ;wait for data TX
;    bra      $-2
;    movlw    "\n"            ;move data into TXREG
;    movwf    TXREG            ;next line
;    btfss    TXSTA,TRMT        ;wait for data TX
;    goto     $-2



	
;-------------------------------------------------------
;Main Code
;
;-------------------------------------------------------
	
	start
		;initialization
		call	init_ports
		call	init_EncCount
		call	init_232

		;main loop
		loop:
			call delay_1s
			clrf 	PORTA
			call delay_1s
			movlw	b'00011111' 
			movwf	PORTA
		goto loop

	end
