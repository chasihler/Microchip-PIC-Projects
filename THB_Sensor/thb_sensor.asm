;******************************************************************
;*								  *
;*	Filename: thb_sensor.asm					  *
;*								  *
;*	J-Paul ROUBELAT - F6FBB - 5 March 2010			  *
;*								  *
;******************************************************************

#include <thb_sensor.inc>

	EXTERN	_i_init
	EXTERN	_h_init,_read_uh
	EXTERN	_p_init,_read_ut,_read_up,_clear_ut,_clear_up,_clear_uh
	EXTERN	_subtract,_add,_multiply,_divide,_sh2rega,_sh2regb,_ush2rega,_ush2regb
	EXTERN	_ee2rega,_ee2regb
	EXTERN	_rega2sh,_shlrega,_shrrega,_bt2rega,_bt2regb,_rega2regb
	EXTERN	_send_data

	EXTERN	UT,UP,UH
	EXTERN	REGA3,REGA2,REGA1,REGA0,REGB3,REGB2,REGB1,REGB0

	GLOBAL	temperature,pressure,humidity,e2p_addr
	GLOBAL	_sleep16ms,_e2p_read,_e2p_write

	__config _FCMEN_OFF & _IESO_OFF & _BOD_OFF & _CPD_OFF & _CP_OFF & _MCLRE_OFF & _PWRTE_ON & _WDT_ON & _INTOSCIO

;******************************************************************************
; variables declaration
;
	UDATA
temperature	res	2	;temperature
pressure	res	2	;pressure
humidity	res	2	;humidity

	UDATA_SHR
e2p_addr	res	1	;address of EEPROM
flags		res	1	;8bits flags
read_count	res	1	;measures counter

	UDATA_OVR	0x50
temp		res	1	;temporary value

X1		res	2
X2		res	2
X3		res	2
B3		res	2
B4		res	2
B5		res	2
B6		res	2
B7		res	2


;******************************************************************************
; DEFINES
;


;******************************************************************************
; macros
;

;******************************************************************************
OSC	CODE    0x03FF

;******************************************************************************
RST	CODE	0x0000

;******************************************************************************
; RESET VECTOR *
;******************************************************************************
	clrf	STATUS
	goto	_init

;******************************************************************************
; INTERRUPT ROUTINE (unused)
;******************************************************************************
INT	CODE	0x0004
	retfie


;******************************************************************************
; MAINLINE CODE
;******************************************************************************
PGM	CODE
_init
	clrf	flags

	call	_i_init		; init processor

	call	_delay1s

	call	_p_init		; init pressure module
	call	_h_init		; init humidity module

;******************************************************************************
; MAIN LOOP
;******************************************************************************
	call	_delay1s
	call	_delay1s

	movlw	.16
	movwf	read_count

_short_loop			; 16 short loops (4s per measure)
	call	_clear_ut
	call	_clear_up
	call	_clear_uh

	call	_read_ut	; read temperature
	call	_read_up	; read pressure
	call	_read_uh	; read humidity
	
	movlw	UT
	call	_shl8
	movlw	UP
	call	_shl8
	movlw	UH
	call	_shl8

	call	_comp_ut	; compute temperature
	call	_comp_up	; compute pressure
	call	_comp_uh	; compute humidity
	call	_send_data	; send data

	call	_sleep4s	; 4 secondes

	decfsz	read_count,F
	goto	_short_loop

_main_loop			; infinite loop (1'15" per measure)
	call	_clear_ut
	call	_clear_up
	call	_clear_uh

	movlw	.16
	movwf	read_count

_read_loop
	call	_read_ut	; read temperature
	call	_read_up	; read pressure
	call	_read_uh	; read humidity

	call	_sleep4s	; 4 secondes

	decfsz	read_count,F
	goto	_read_loop

	movlw	UT
	call	_shl4
	movlw	UP
	call	_shl4
	movlw	UH
	call	_shl4

	call	_comp_ut	; compute temperature
	call	_comp_up	; compute pressure
	call	_comp_uh	; compute humidity
	call	_send_data	; send data

	goto	_main_loop


;******************************************************************************
; Compute temperature
;******************************************************************************
_comp_ut
	movlw	UT		;A = temperature
	call	_ush2rega
	movlw	AC6		;B = AC6
	call	_ee2regb
	call	_subtract	;A = (UT - AC6)

	movlw	AC5
	call	_ee2regb	;B = AC5
	call	_multiply	;A = (UT - AC6) * AC5

	movlw	.15
	call	_shrrega	;A = (UT - AC6) * AC5 / 2^15
	movlw	X1
	call	_rega2sh	;X1 = (UT - AC6) * AC5 / 2^15 -> X1

	movlw	MD
	call	_ee2regb	;B = MD
	call	_add		;A = X1 + MD
	movlw	X3
	call	_rega2sh	;X1 + MD -> X3

	movlw	MC		;A = MC
	call	_ee2rega
	movlw	.11
	call	_shlrega	;A = MC * 2^11

	movlw	X3		;X3
	call	_sh2regb
	call	_divide		;MC * 2^11 / X3

	movlw	X1
	call	_sh2regb
	call	_add		;X1 + X2

	movlw	B5
	call	_rega2sh

	movlw	.8
	call	_bt2regb
	call	_add		;X1 + X2 + 8

	movlw	.4
	call	_shrrega

	movlw	temperature
	call	_rega2sh	;result to temperature

	return

;******************************************************************************
; Compute pressure
;******************************************************************************
_comp_up
	movlw	B5
	call	_sh2rega	;A = B5

	movlw	0x0f
	movwf	X1
	movlw	0xa0
	movwf	X1+1
	movlw	X1
	call	_sh2regb	;B = 4000

	call	_subtract	;A = B5 - 4000

	movlw	B6
	call	_rega2sh	;B6 = B5 - 4000

	call	_rega2regb
	call	_multiply	;A = B6 * B6

	movlw	.12
	call	_shrrega	;A = (B6 * B6) / 2^12

	movlw	B5
	call	_rega2sh	;B5 = (B6 * B6) / 2^12

	movlw	B2
	call	_ee2regb	;B = B2

	call	_multiply	;A = B2 * ((B6 * B6) / 2^12)

	movlw	.11
	call	_shrrega	;A = (B2 * ((B6 * B6) / 2^12)) / 2^11

	movlw	X1
	call	_rega2sh	;X1 = (B2 * ((B6 * B6) / 2^12)) / 2^11

	movlw	AC2
	call	_ee2rega	;A = AC2

	movlw	B6
	call	_sh2regb	;B = B6

	call	_multiply	;A = AC2 * B6

	movlw	.11
	call	_shrrega	;A = AC2 * B6 / 2^11

	movlw	X1
	call	_sh2regb

	call	_add		;A = X1 + (AC2 * B6 / 2^11)

	movlw	X3
	call	_rega2sh	;X3 = X1 + (AC2 * B6 / 2^11)

	movlw	AC1
	call	_ee2rega	;A = AC1

	movlw	.2
	call	_shlrega	;A = AC1 * 4

	movlw	X3
	call	_sh2regb	;B = X3

	call	_add		;A = AC1 * 4 + X3

	movlw	.2
	call	_bt2regb	;B = 2

	call	_add		;A = (AC1 * 4 + X3) + 2

	movlw	.2
	call	_shrrega	;A = ((AC1 * 4 + X3) + 2) / 4

	movlw	B3
	call	_rega2sh	;B3 = ((AC1 * 4 + X3) + 2) / 4

	movlw	AC3
	call	_ee2rega	;A = AC3

	movlw	B6
	call	_sh2regb	;B = B6

	call	_multiply	;A = AC3 * B6

	movlw	.13
	call	_shrrega	;A = AC3 * B6 / 2^13

	movlw	X1
	call	_rega2sh	;X1 = AC3 * B6 / 2^13

	movlw	B1
	call	_ee2rega	;A = B1

	movlw	B5
	call	_sh2regb	;B = (B6 * B6) / 2^12

	call	_multiply	;A = B1 * ((B6 * B6) / 2^12)

	movlw	.16
	call	_shrrega	;(X2) A = (B1 * ((B6 * B6) / 2^12)) / 2^16

	movlw	X1
	call	_sh2regb	;B = X1

	call	_add		;A = X1 + X2

	movlw	.2
	call	_bt2regb	;B = 2

	call	_add		;A = X1 + X2 + 2

	movlw	.2
	call	_shrrega	;A = (X1 + X2 + 2) / 4

	movlw	0x80
	movwf	B6
	movlw	0x00
	movwf	B6+1
	movlw	B6
	call	_ush2regb	;B = 32768

	call	_add		;A = X3 + 32768

	movlw	AC4
	call	_ee2regb	;B = AC4

	call	_multiply	;A = AC4 * (X3 + 32768)

	movlw	.15
	call	_shrrega	;A = (AC4 * (X3 + 32768)) / 2^15

	movlw	B4
	call	_rega2sh	;B4 = (AC4 * (X3 + 32768)) / 2^15

	movlw	UP
	call	_ush2rega	;A = UP

	movlw	B3
	call	_sh2regb	;B = B3

	call	_subtract	;A = UP - B3

	movlw	0xc3
	movwf	B6
	movlw	0x50
	movwf	B6+1
	movlw	B6
	call	_ush2regb	;B = 50000

	call	_multiply	;A = (UP - B3) * 50000

	movlw	B4
	call	_ush2regb	;B = B4

	call	_divide		;(P) A = ((UP - B3) * 50000) / B4

	movlw	B3
	call	_rega2sh	; B3 = P/2

	movlw	.7
	call	_shrrega	;A = (P/2) / 2^7

	call	_rega2regb	;B = (P/2) / 2^7

	call	_multiply	;A = ((P/2) / 2^7) * ((P/2) / 2^7)

	movlw	0x0b
	movwf	B6
	movlw	0xde
	movwf	B6+1
	movlw	B6
	call	_ush2regb	;B = 3038

	call	_multiply	;A = (P / 2^7) * (P / 2^7) * 3038

	movlw	.16
	call	_shrrega	;A = ((P / 2^7) * (P / 2^7) * 3038) / 2^16

	movlw	X1
	call	_rega2sh

	movlw	B3
	call	_ush2rega	;A = P/2

	movlw	.1
	call	_shlrega	;A = P

	call	_rega2regb	;B = P

	movlw	0xe3
	movwf	B6
	movlw	0x43	
	movwf	B6+1
	movlw	B6
	call	_sh2rega	;A = -7357

	call	_multiply	;A = -3757 * P

	movlw	.16
	call	_shrrega	;(X2) A = (-3757 * P) / 2^16

	movlw	X1
	call	_sh2regb

	call	_add		;A = X1 + X2

	movlw	0x0e
	movwf	B6
	movlw	0xcf	
	movwf	B6+1
	movlw	B6
	call	_sh2regb	;B = 3791
	
	call	_add		;A = X1 + X2 + 3791

	movlw	.4
	call	_shrrega	;A = (X1 + X2 + 3791) / 2^4
	call	_rega2regb	;B = (X1 + X2 + 3791) / 2^4

	movlw	B3
	call	_ush2rega	;A = P/2

	movlw	.1
	call	_shlrega	;A = P

	call	_add		;A = P + (X1 + X2 + 3791) / 2^4

	movlw	.10
	call	_bt2regb	;A = A / 10
	call	_divide		;A = pressure * 0.1hPa

	movlw	0x07
	movwf	B6
	movlw	0xd0
	movwf	B6+1
	movlw	B6
	call	_ush2regb	;B = 2000

	call	_subtract	;A = (pressure - 200) * 0.1hPa

	movlw	pressure
	call	_rega2sh	;result to pressure

	return

;******************************************************************************
; Compute humidity
;******************************************************************************
_comp_uh
	movlw	UH
	call	_ush2rega	;A = UH
	movlw	.20
	call	_bt2regb	;B = 20
	call	_multiply	;A = 20 * UH
	movlw	.75
	call	_bt2regb	;B = 75
	call	_multiply	;A = 75 * 20 * UH
	call	_rega2regb	;B = A
	movlw	.1
	call	_bt2rega
	movlw	.10
	call	_shlrega	;B = 2^10
	call	_add		;for rounding
	movlw	.11
	call	_shrrega	;A = (75 * 20 * UH) / 2^11
	movlw	X1
	call	_rega2sh	;X1 = (75 * 20 * UH) / 2^11
	
	movlw	UH
	call	_ush2rega	;A = UH
	movlw	.20
	call	_bt2regb	;B = 20
	call	_multiply	;A = 20 * UH
	movlw	.5
	call	_shrrega	;A /= 2^5
	call	_rega2regb	;B = A
	call	_multiply	;A = (A^2)
	movlw	.43
	call	_bt2regb
	call	_multiply	;A = 43 * (X1 * X1) / 2^8
	call	_rega2regb	;B = A
	movlw	.1
	call	_bt2rega
	movlw	.18
	call	_shlrega	;B = 2^10
	call	_add		;for rounding
	movlw	.19
	call	_shrrega	;A = (43 * (X1 * X1) / 2^8) / 2^19
	movlw	X2
	call	_rega2sh	;X2 = A

	; X1 - 41 - X2
	movlw	X1		;A = X1
	call	_sh2rega
	movlw	.41
	call	_bt2regb
	call	_subtract	;A -= 41
	movlw	X2		;B = X2
	call	_sh2regb
	call	_subtract	;A -= X2
	movlw	.1
	call	_shrrega	;A /= 2
	movlw	humidity
	call	_rega2sh	;A -> humidity (*10)

	; temperature compensation
	movlw	temperature
	call	_sh2rega	;A = temperature (*10)
	movlw	0x00
	movwf	B6
	movlw	.250
	movwf	B6+1
	movlw	B6
	call	_sh2regb	;B = 250
	call	_subtract
	movlw	X1
	call	_rega2sh	;X1 = deltaT

	movlw	.50
	call	_bt2regb
	btfsc	REGA0,7
	call	_subtract	;for rounding
	btfss	REGA0,7
	call	_add		;for rounding
	movlw	.100
	call	_bt2regb
	call	_divide		;A = deltaT / 100
	movlw	X2
	call	_rega2sh	;X2 = deltaT / 100

	movlw	X1
	call	_sh2rega	;A = deltaT
	movlw	UH
	call	_sh2regb	;B = UH
	call	_multiply	;A = deltaT * UH
	movlw	0x30
	movwf	B6
	movlw	0xd4
	movwf	B6+1
	movlw	B6
	call	_sh2regb	;B = 12500
	call	_divide		;A = deltaT * UH / 12500

	movlw	X2
	call	_sh2regb	;B = X2
	call	_add		;A = (deltaT * UH) + (deltaT * UH / 12500) + X2

	movlw	humidity
	call	_sh2regb
	call	_add		;A = humidity + correction

	movlw	humidity
	call	_rega2sh	;X1 = humidity

	btfsc	humidity,7	;negative ?
	goto	humi_0

	call	_rega2regb
	movlw	0x03
	movwf	B6
	movlw	0xe7
	movwf	B6+1
	movlw	B6
	call	_sh2rega
	call	_subtract

	btfss	REGA3,7		;negative ?
	return

	movlw	0x3
	movwf	humidity
	movlw	0xe7
	movwf	humidity+1
	return

humi_0
	clrf	humidity
	clrf	humidity+1
	return

;******************************************************************************
; shift left 4 bits
;******************************************************************************
_shl4
	movwf	FSR
	movlw	.4
	movwf	temp

_shl4_lp
	incf	FSR,F
	incf	FSR,F
	bcf	STATUS,C
	rlf	INDF,F
	decf	FSR,F
	rlf	INDF,F
	decf	FSR,F
	rlf	INDF,F
	decfsz	temp,F
	goto	_shl4_lp

	return

;******************************************************************************
; shift left 8 bits
;******************************************************************************
_shl8
	movwf	FSR

	incf	FSR,F		; U+1 -> U
	movfw	INDF
	decf	FSR,F
	movwf	INDF

	incf	FSR,F		; U+2 -> U+1
	incf	FSR,F
	movfw	INDF
	decf	FSR,F
	movwf	INDF

	return

;******************************************************************************
; delay 10ms -> Uses TIMER0 (predivisor = 64)
;******************************************************************************
_delay10ms
	movlw	.99
	movwf	TMR0
	bcf	INTCON,T0IF
	clrwdt
	btfss	INTCON,T0IF
	goto $-1
	return

;******************************************************************************
; delay # 1 second
;******************************************************************************
_delay1s
	movlw	.100
	movwf	temp
_delay1s_loop
	call	_delay10ms
	decfsz	temp,F
	goto	_delay1s_loop
	return

;******************************************************************************
; sleep #4 seconde using the watchdog timer
;******************************************************************************
_sleep4s
	movlw	b'00010110'	; 2.1s watchdog
	movwf	WDTCON
	clrwdt
	sleep			; sleep 2.1s
	sleep			; sleep 2.1s
	return

;******************************************************************************
; sleep 16ms using the watchdog timer
;******************************************************************************
_sleep16ms
	movlw	b'00010000'	; 16ms watchdog
	movwf	WDTCON
	clrwdt
	sleep			; sleep 16ms
	movlw	b'00010110'	; back to 2.1s watchdog
	movwf	WDTCON
	clrwdt
	return

;******************************************************************************
; e2p_write 
; write the content of W to flash address e2p_addr
;
_e2p_write
	movwf	temp	; Save character

	BANK1
	btfsc	EECON1,WR
	goto	$-1		; Wait for end of previous eeprom write

	BANK0
	movfw	e2p_addr
	BANK1
	movwf	EEADR
	BANK0
	movfw	temp
	BANK1
	movwf	EEDATA

	bsf	EECON1,WREN	; Enable EEPROM Write

	movlw	0x55
	movwf	EECON2
	movlw	0xaa
	movwf	EECON2
	bsf	EECON1,WR	; Start write

	bcf	EECON1,WREN	; Disable EEPROM write
	BANK0

	return

;******************************************************************************
; ReadFlash 
; w returns the value pointed by flash address e2p_addr
;
_e2p_read
	movfw	e2p_addr

	BANK1
	movwf	EEADR
	bsf	EECON1,RD
	movfw	EEDATA
	BANK0

	return

;====================================================================================	
;
; End of module
;
	END
