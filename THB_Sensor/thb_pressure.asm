;******************************************************************
;*								  *
;*	Filename: thb_pressure.asm				  *
;*								  *
;*	J-Paul ROUBELAT - F6FBB - 5 March 2010			  *
;*								  *
;******************************************************************

#include <thb_sensor.inc>

	GLOBAL	_p_init,_read_ut,_read_up,_clear_ut,_clear_up
	GLOBAL	UP,UT,UH,measure

	EXTERN	_Read_BMP,_Write_BMP,_Seq_Read_BMP,_sleep16ms
	EXTERN	_e2p_read,_e2p_write
	EXTERN	e2p_addr,I2C_Addr,I2C_Len

;******************************************************************************
; global variables declaration
;
	UDATA
UP		res	3	;pressure
UT		res	3	;temperature
UH		res	3	;temperature
parCount	res	1	;parameters counter
delCount	res	1	;delay counter

	UDATA_SHR
measure		res	2	;measure

;******************************************************************************
PGM	CODE

_p_init
	goto	_read_cal	;Read calibration values

;******************************************************************************
; Read calibration values
; return W = 0 if error
;******************************************************************************
_read_cal
	movlw	BMP_PARM
	movwf	I2C_Addr		;base address of calibration values

	movlw	CALIB			;start address to write params in PIC e2prom
	movwf	e2p_addr

	movlw	BMP_NPAR		;22 parameters to read
	movwf	parCount

_cal_loop
	call	_Read_BMP
	call	_e2p_write		;store parameter in local E2PROM
	incf	I2C_Addr,F		;inc calibration address
	incf	e2p_addr,F		;inc local E2PROM address
	decfsz	parCount,F
	goto	_cal_loop

	return

;******************************************************************************
; Clear temperature
;******************************************************************************
_clear_ut
	clrf	UT
	clrf	UT+1
	clrf	UT+2
	return

;******************************************************************************
; Read temperature
; return W = 0 if error
;******************************************************************************
_read_ut
	movlw	BMP_REGT
	movwf	I2C_Addr		;write to BMP register
	movlw	BMP_TEMP		;request temperature measure
	call	_Write_BMP

	call	_sleep16ms		;wait 16ms

	movlw   BMP_MEAS		;Measure base address
	movwf	I2C_Addr
	movlw	.2			;nb of Parameters
	movwf	I2C_Len
	movlw	measure			;store to measure
	movwf	FSR

	call	_Seq_Read_BMP		; read temperature

	movfw	measure+1		; add to current measures
	addwf	UT+2,F
	movfw	measure
	skpnc
	incfsz	measure,W
	addwf   UT+1,F          ;
	skpnc
	incf	UT,F

	retlw	0

;******************************************************************************
; Clear pressure
;******************************************************************************
_clear_up
	clrf	UP
	clrf	UP+1
	clrf	UP+2
	return

;******************************************************************************
; Read pressure
; return W = 0 if error
;******************************************************************************
_read_up
	movlw	BMP_REGT
	movwf	I2C_Addr		;write to BMP register
	movlw	BMP_PRES		;request temperature measure
	call	_Write_BMP

	call	_sleep16ms		;wait 16ms

	movlw   BMP_MEAS		;Measure base address
	movwf	I2C_Addr
	movlw	.2			;nb of Parameters
	movwf	I2C_Len
	movlw	measure			;store to measure
	movwf	FSR

	call	_Seq_Read_BMP		; read temperature

	movfw	measure+1		; add to current measures
	addwf	UP+2,F
	movfw	measure
	skpnc
	incfsz	measure,W
	addwf   UP+1,F          ;
	skpnc
	incf	UP,F

	retlw	0


;******************************************************************************
;
; End of module
;
	END
