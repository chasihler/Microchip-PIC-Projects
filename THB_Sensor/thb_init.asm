;******************************************************************
;*								  *
;*	Filename: thb_sensor.asm					  *
;*								  *
;*	J-Paul ROUBELAT - F6FBB - 5 March 2010			  *
;*								  *
;******************************************************************

#include <thb_sensor.inc>

	GLOBAL	_i_init

;******************************************************************
PGM	CODE

;******************************************************************
;
; Init processor:
;
; SCK (sbus clock)	GPIO0 = output
; SDT (sbus data)	GPIO1 = input
; TX  (TX data)		GPIO2 = output
; SCL (I2C clock)	GPIO4 = input
; SDA (I2C data)	GPIO5 = input
;
; clock = 4MHz
; no interrupt
; watchdog = 2 seconds
;
_i_init
	clrf	GPIO

	movlw	b'00000111'	; comp off			  |B0
	movwf	CMCON0		;				  |B0

	BANK1

	; timer 0 configuration, prescaler to timer0, no pull-ups
	movlw	b'10000101'	; timer0 on internal clock/64,	  |B1
	movwf	OPTION_REG	; and pull-ups -> option register

	clrf	VRCON		; disable Voltage Reference	  |B1

	clrf	ANSEL		; digital I/O			  |B1

	movlw	b'01100001'	;				  |B1
	movwf	OSCCON		; 4-mhz INTOSC system clock	  |B1

	movlw	b'00111000'	; GP3,4,5=IN GP0,1,2=OUT	  |B1
	movwf	TRISIO		; Set GPIO for intput/output      |B1	

	clrf	INTCON		; disable interrupts clear flags  |B1

	; wait for INTOSC to become stable before doing anything else
;_stable
;	btfss	OSCCON,HTS	;oscillator stable?		  |B1
;	goto	_stable		;no, branch			  |B1

	BANK0

	movlw	b'00010110'	; watchdog = 2 seconds
	movwf	WDTCON

	bcf	G_TX		; TX OFF

	return

;====================================================================================	
;
; End of module
;
	END
