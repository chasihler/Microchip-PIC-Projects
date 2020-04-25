;title "Roomba System 1"
;#define _version="0.04"
;==================================================================
;
; Roomba Sumo Bot System 1
; by Charles M Douvier
;
;    Software History
;    v 0.01    LED ON, board check			11/25/2012
;    v 0.02    RS-232 Added					11/29/2012
;    v 0.03    interrupts on RB<5:4>		12/01/2012
;    v 0.04    interrupts on RB<6:5>		12/01/2012
;				On change --> TX>RS232
;
;	PIN DIAGRAM
;
; 	RA0	O	LED/STATUS		RB0	I	
;	RA1	O					RB1	I	RX
;	RA2	O					RB2	I	TX
;	RA3	I					RB3	I	
;	RA4	I					RB4	I	PGM/PGM
;	RA5	I	/MCLR			RB5	I	L ENCODER
;	RA6	O					RB6	I	PGM/PGC/R ENCODER
;	RA7	O					RB7	I	PGM/PGD
;
;
;===================================================================
;
;list p=16F628A
    LIST R=DEC
#include "p16F628a.inc"
;Macro
    __CONFIG _CP_OFF & _WDT_OFF & _INTOSC_OSC_NOCLKOUT & _BODEN_OFF & _PWRTE_ON
;

	CBLOCK 0x20
		l_count
		l_cmp	
		r_count
		r_cmp	
		W_TEMP
		STATUS_TEMP
		d1
		d2
        Xmit_Byte
        Rcv_Byte
        Bit_Cntr
        Delay_Count
		THOUSANDS
		HUNDREDS
		TENS
		ONES
	ENDC

    org    0
startup:
	NOP
    GOTO init

    org 4
Int:
	MOVWF W_TEMP 		;copy W to temp register,
						;could be in any bank
	SWAPF STATUS,W 		;swap status to be saved
						;into W
	BCF STATUS,RP0 		;change to bank 0
						;regardless of current
						;bank
	MOVWF STATUS_TEMP 	;save status to bank 0
						;register
						;:
	NOP					;:(ISR)

	BTFSS	PORTB,5		;If port RB4 is high, increment left encoder count
	INCF	l_count,1	
	BTFSS	PORTB,6		;if port RB5 is high, increment right encoder count
	INCF	r_count,1
	
    bcf INTCON,RBIF 
    bsf INTCON,RBIE 

	NOP
						;:
	SWAPF STATUS_TEMP,W	;swap STATUS_TEMP register
						;into W, sets bank to original
						;state
	MOVWF STATUS 		;move W into STATUS
						;register
	SWAPF W_TEMP,F 		;swap W_TEMP
	SWAPF W_TEMP,W 		;swap W_TEMP into W

	RETFIE	

;===============================================
;INTERRUPT INIT SEQUENCE

;
;===============================================

INT_INIT
    BANKSEL INTCON	
	BCF	INTCON, RBIF
	BSF	INTCON, RBIE
;	BSF	INTCON, GIE
	BANKSEL OPTION_REG
    BCF OPTION_REG,7    ;RBPU: 0=portb pull ups are enabled 
	RETURN

;===============================================
;DELAY SEQUENCE
;250ms
;
;===============================================

DELAY25_0
	decfsz	d1, f
	goto	$+2
	decfsz	d2, f
	goto	DELAY25_0

			;2 cycles
	goto	$+1
	NOP
	RETURN
;================================================
;donniedj/21 Mat 2004
;http://www.electro-tech-online.com/microcontrollers/9121-converting-binary-number-ascii-code.html
;---------------------------------------
;PREREQUISITE:	LOADF	HIGH_BYTE
;				LOADF	LOW_BYTE	
;================================================
HEX_TO_DEC
	;0X2710=10,OOO,  0X3E8=1,000,  0X64=100,  0X0A=10,  0X01=1
	;0X270F = 9999
 
	MOVF_F	LOW_BYTE, TEMP_LOW
	CLRF	THOUSANDS
         CLRF     HUNDREDS
         CLRF     TENS
         CLRF     ONES
CHECK_H       
         MOVF_F   TEMP_LOW, TEMP
         MOVLW    .100
         SUBWF    TEMP, F
         BTFSC    STATUS, C
         GOTO     $ + 2
         GOTO     CHECK_T
 
         INCF     HUNDREDS, F
         MOVF_F   TEMP, TEMP_LOW
         GOTO     CHECK_H
CHECK_T  MOVF_F   TEMP_LOW, TEMP
         MOVLW    .10
         SUBWF    TEMP, F
         BTFSC    STATUS, C
         GOTO     $ + 2
         GOTO     CHECK_O
 
         INCF     TENS, F
         MOVF_F   TEMP, TEMP_LOW
         GOTO     CHECK_T
CHECK_O  MOVF_F   TEMP_LOW, TEMP
         MOVLW    .1
         SUBWF    TEMP, F
         BTFSC    STATUS, C
         GOTO     $ + 2
         GOTO     HEXDEC2
 
         INCF     ONES, F
         MOVF_F   TEMP, TEMP_LOW
         GOTO     CHECK_O
HEXDEC2
 
 
HI_0	BTFSC	HIGH_BYTE, 0
	GOTO	$ + 2
	GOTO	HI_1
 
	MOVLW	.2
	ADDWF	HUNDREDS, F
	MOVLW	.5
	ADDWF	TENS, F
	MOVLW	.6
	ADDWF	ONES, F
HI_1	BTFSC	HIGH_BYTE, 1
	GOTO	$ + 2
	GOTO	HI_2
 
	MOVLW	.5
	ADDWF	HUNDREDS, F
	MOVLW	.1
	ADDWF	TENS, F
	MOVLW	.2
	ADDWF	ONES, F
HI_2	BTFSC	HIGH_BYTE, 2
	GOTO	$ + 2
	GOTO	HI_3
 
	MOVLW	.1
	ADDWF	THOUSANDS, F
	MOVLW	.2
	ADDWF	TENS, F
	MOVLW	.4
	ADDWF	ONES, F
HI_3	BTFSC	HIGH_BYTE, 3
	GOTO	$ + 2
	GOTO	HI_4
 
	MOVLW	.2
	ADDWF	THOUSANDS, F
	MOVLW	.4
	ADDWF	TENS, F
	MOVLW	.8
	ADDWF	ONES, F
HI_4	BTFSC	HIGH_BYTE, 4
	GOTO	$ + 2
	GOTO	HI_5
 
	MOVLW	.4
	ADDWF	THOUSANDS, F
	MOVLW	.9
	ADDWF	TENS, F
	MOVLW	.6
 	ADDWF	ONES, F
HI_5	BTFSC	HIGH_BYTE, 5
	GOTO	$ + 2
	GOTO	ENDBITSUM
 
	MOVLW	.8
	ADDWF	THOUSANDS, F
	MOVLW	.1
	ADDWF	HUNDREDS, F
	MOVLW	.9
	ADDWF	TENS, F
	MOVLW	.2
	ADDWF	ONES, F
ENDBITSUM
 
 
	;PROCESS OVERFLOW	
ONES_OVER
	MOVLW	.9
	SUBWF	ONES, W
	BTFSC	STATUS, Z
	GOTO	TENS_OVER
	BTFSS	STATUS, C
	GOTO	TENS_OVER
 
	INCF	TENS, F
	MOVLW	.10
	SUBWF	ONES, F
 
TENS_OVER
	MOVLW	.9
	SUBWF	TENS, W
	BTFSC	STATUS, Z
	GOTO	HUNDREDS_OVER
	BTFSS	STATUS, C
	GOTO	HUNDREDS_OVER
 
	INCF	HUNDREDS, F
	MOVLW	.10
	SUBWF	TENS, F
 
HUNDREDS_OVER
	MOVLW	.9
	SUBWF	HUNDREDS, W
	BTFSC	STATUS, Z
	GOTO	ENDHEX_TO_DEC
	BTFSS	STATUS, C
	GOTO	ENDHEX_TO_DEC
 
	INCF	THOUSANDS, F
	MOVLW	.10
	SUBWF	HUNDREDS, F
 
 
ENDHEX_TO_DEC
         RETURN

;==========================================
;Transmit encoder info via RS-232
;
;
;==========================================

xmit_l_count
    banksel    TXREG            ;move data into TXREG
    movlw    "L"           
    movwf    TXREG
    banksel    TXSTA
    btfss    TXSTA,TRMT        ;wait for data TX
    goto    $-1
    banksel    PORTA

	movfw	l_count
    banksel    TXREG            ;move data into TXREG
    movwf    TXREG
    banksel    TXSTA
    btfss    TXSTA,TRMT        ;wait for data TX
    goto    $-1
    banksel    PORTA

	MOVFW	l_count
	MOVWF	l_cmp

	RETURN

xmit_r_count
    banksel    TXREG            ;move data into TXREG
    movlw    "R"           
    movwf    TXREG
    banksel    TXSTA
    btfss    TXSTA,TRMT        ;wait for data TX
    goto    $-1

    banksel    TXREG            ;move data into TXREG
   	MOVFW	r_count  
    movwf    TXREG
    banksel    TXSTA
    btfss    TXSTA,TRMT        ;wait for data TX
    goto    $-1
    banksel    PORTA

    banksel    PORTA
	MOVFW	r_count
	MOVWF	r_cmp
	RETURN

init:
;==========================================
;Interrupt Initialization
;Delay Initialization
;Port Initialization
;RS-232 Initialization
;==========================================
	CALL	INT_INIT

			;counter setup, change this later to 0x00
	MOVLW	0x00
	MOVWF	l_count
	MOVWF	r_count
	MOVWF	l_cmp
	MOVWF	r_cmp
			;DELAY SETUP
			;249998 cycles
	movlw	0x4F
	movwf	d1
	movlw	0xC4
	movwf	d2

			;PORT SETUP
	BCF	STATUS,RP0
	BCF	STATUS,RP1
	CLRF	PORTA
	CLRF	PORTA

	MOVLW 0x07 				;Turn comparators off and
	MOVWF CMCON 			;enable pins for I/O
							;functions
	BCF STATUS, RP1
	BSF STATUS, RP0 		;Select Bank1
	MOVLW 0x38 				;Value used to initialize
							;data direction
	MOVWF TRISA 			;Set RA<5:3> as inputs
							;TRISA<5> always read as ‘1’.
	MOVLW	0xFF
	MOVWF	TRISB			;PORTB RB<7:0> all inputs
	BCF	STATUS,RP0
	BCF	STATUS,RP1

				;RS-232 

    banksel    TXSTA		;initialize USART       
    movlw    B'10100100'  	;Master mode, 8-bit, Async, High speed
    movwf    TXSTA
    movlw    .25            ;9.6Kbaud @ 4MHz
    movwf    SPBRG
    banksel    RCSTA
    movlw    B'10010000'
    movwf    RCSTA

    banksel    TXREG            ;move data into TXREG
    movlw    "+"           
    movwf    TXREG
    banksel    TXSTA
    btfss    TXSTA,TRMT        ;wait for data TX
    goto    $-1
    banksel    PORTA
MainCode:
;==========================================
;
;goodies go here.
;
;==========================================
							

LOOP:
	BCF	STATUS,RP0
	BCF	STATUS,RP1
	BSF PORTA,0
	BSF	INTCON, GIE	
	
	MOVFW	l_count
	SUBWF	l_cmp,W
	BTFSS	STATUS,Z
	CALL	xmit_l_count

	NOP

	MOVFW	r_count
	SUBWF	r_cmp,W
	BTFSS	STATUS,Z
	CALL	xmit_r_count

	NOP
	NOP

	CALL	DELAY25_0
	BCF PORTA,0
	CALL	DELAY25_0

    GOTO 	LOOP
    end

