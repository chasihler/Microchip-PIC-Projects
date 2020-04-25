;=======pwm.ASM=================================15/11/06==
;     standard INTRC_OSC_NOCLKOUT 4MHz
;	 http://www.module.ro
;------------------------------------------------------------
;     configure programmer
	LIST P=16F628;f=inhx8m
	#include "P16F628.INC"  ; Include header file
	__CONFIG	_PWRTE_ON  & _WDT_OFF & _INTRC_OSC_NOCLKOUT & _BODEN_ON & _LVP_OFF & _CP_OFF & _MCLRE_OFF
;------------------------------------------------------------
	cblock 0x20	; Beginn General Purpose-Register
;-------------------------- counters	
	count1
	count2
	count3
;--------------------------
	endc
;--------------------------
#DEFINE pwmu	PORTB,3
;--- Reset --------------------------------------------------
	org	h'00'
	goto	init		; reset -> init
;--- Interrupt ----------------------------------------------
	org	h'04'
;--------------------------
init	clrf	PORTA
	clrf	PORTB
	movlw	0x07		; Turn comparators off and enable pins for I/O 
	movwf	CMCON	
	bcf	STATUS,RP1
	call	usi	;  port settings
	call	pause
	movlw	0xFF
	movwf	PORTB
	call	pause	; wait 126ms
	call	set_timer

;-------------------------- 
uu	
	call	pause
	movlw	0x02	; set pwm at 2/256
	movwf	CCPR1L
	call	pause	; wait 126ms
	movlw	0x10
	movwf	CCPR1L	; set pwm at 10/256
	call	pause
	movlw	0x40
	movwf	CCPR1L
	call	pause
	movlw	0x80
	movwf	CCPR1L
	call	pause
	movlw	0xFF	; max. power
	movwf	CCPR1L
	call	pause
	movlw	0x80
	movwf	CCPR1L
	call	pause
	movlw	0x40
	movwf	CCPR1L
	call	pause
	movlw	0x10
	movwf	CCPR1L
	goto	uu	; repeat

;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
set_timer
	clrf	T2CON
	clrf	TMR2
	clrf	INTCON
	bsf	STATUS,RP0
	clrf	PIE1
	bcf	STATUS,RP0
	clrf	PIR1
	bsf	STATUS,RP0
	movlw	0xFF
	movwf	PR2 	; compare with 255
	bcf	STATUS,RP0
	movlw	0x02
	movwf	CCPR1L
	movlw	0x03
	movwf	T2CON	 ; prescaler 1:16 and postscaler 1:1
	movlw	0x3C
	movwf	CCP1CON
	bsf	T2CON,TMR2ON	;turn on timer, see pic16f628 datasheet
	return

;************************************************************************
;     Delay					                                             *
;************************************************************************
pause	movlw	0x02	 ;126 ms	
	movwf	count3
d3	movlw	0x3F	;63 ms
	movwf	count1
d1	movlw	0xFA	; 1ms  
	movwf	count2
d2	nop
	decfsz	count2,F	
	goto	d2		
	decfsz	count1,F	
	goto	d1		
	decfsz	count3,F    
	goto	d3          
	retlw	0x00
;============================================================
usi	bsf	STATUS,RP0	; Bank 1
	movlw	0xFF ; all input
	movwf	TRISA
	movlw	0x00
	movwf	TRISB ; all output
	bcf	STATUS,RP0	; Bank 0
	return
;============================================================
	end
;============================================================