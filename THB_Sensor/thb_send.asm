;******************************************************************
;*								  *
;*	Filename: thb_send.asm					  *
;*								  *
;*	J-Paul ROUBELAT - F6FBB - 5 March 2010			  *
;*								  *
;******************************************************************

#include <thb_sensor.inc>

	GLOBAL	_send_data
	EXTERN	temperature,pressure,humidity,UT,UP,UH
	EXTERN	_e2p_read,e2p_addr

;******************************************************************************
; global variables declaration
;

	UDATA_OVR	0x50
temp1		res	1
temp2		res	1

counter		res	1
nib_tmp		res	1
nib_del		res	1

sn_temp		res	3
sn_humi		res	3
sn_pres		res	4
sn_type		res	1
sn_addr		res	1
sn_xor		res	1
sn_chck		res	1

;******************************************************************************
; local variables declaration
;
	UDATA_SHR
bcd_th_hu	res	1
bcd_te_on	res	1

;******************************************************************
PGM	CODE

;******************************************************************
;
; send a character in W to RSOUT 9600Bds, 8bits, no parity, 1 stop
;
_send_data
	movlw	.4
	movwf	sn_type		; type of sensor = 4
	movlw	.3
	movwf	sn_addr		; address = 3

	; temperature
	btfss	temperature,7	; check temperature sign
	goto	_temp_pos

	comf	temperature+1,F	; temperature is negative
	comf	temperature,F	; change it to positive
	movlw	1
	addwf	temperature+1,F
	skpnc
	addwf	temperature,F

	bsf	sn_addr,3	; set minus sign

_temp_pos
	movlw	temperature	; temperature is positive
	call	_hexToDec

	movfw	bcd_th_hu
	andlw	0xf
	movwf	sn_temp

	swapf	bcd_te_on,W
	andlw	0xf
	movwf	sn_temp+1

	movfw	bcd_te_on
	andlw	0xf
	movwf	sn_temp+2

	; humidity
	movlw	humidity
	call	_hexToDec

	movfw	bcd_th_hu
	andlw	0xf
	movwf	sn_humi

	swapf	bcd_te_on,W
	andlw	0xf
	movwf	sn_humi+1

	movfw	bcd_te_on
	andlw	0xf
	movwf	sn_humi+2

	; pressure
	movlw	pressure
	call	_hexToDec

	swapf	bcd_th_hu,W
	andlw	0xf
	movwf	sn_pres

	movfw	bcd_th_hu
	andlw	0xf
	movwf	sn_pres+1

	swapf	bcd_te_on,W
	andlw	0xf
	movwf	sn_pres+2

	movfw	bcd_te_on
	andlw	0xf
	movwf	sn_pres+3

	goto	_send_frame


;******************************************************************************
; Send a HF data frame
;******************************************************************************
_send_frame

	; prepare the frame

	clrw

	xorwf	sn_type,w
	xorwf	sn_addr,w

	xorwf	sn_temp+2,w
	xorwf	sn_temp+1,w
	xorwf	sn_temp+0,w

	xorwf	sn_humi+2,w
	xorwf	sn_humi+1,w
	xorwf	sn_humi+0,w

	xorwf	sn_pres+2,w
	xorwf	sn_pres+1,w
	xorwf	sn_pres+0,w
	xorwf	sn_pres+3,w

	movwf	sn_xor		; check xor

	movlw	.5
	addwf	sn_type,w
	addwf	sn_addr,w

	addwf	sn_temp+2,w
	addwf	sn_temp+1,w
	addwf	sn_temp+0,w

	addwf	sn_humi+2,w
	addwf	sn_humi+1,w
	addwf	sn_humi+0,w

	addwf	sn_pres+2,w
	addwf	sn_pres+1,w
	addwf	sn_pres+0,w
	addwf	sn_pres+3,w

	addwf	sn_xor,w
	
	andlw	0xf		; get LSB
	movwf	sn_chck		; check sum
	
	; send the frame
	clrw			; 10 bits 0 header
	call	_send_nib
	clrw
	call	_send_nib
	call	_send_0
	call	_send_0
	call	_send_1

	movfw	sn_type
	call	_send_nib
	call	_send_1

	movfw	sn_addr
	call	_send_nib
	call	_send_1

	movfw	sn_temp+2
	call	_send_nib
	call	_send_1

	movfw	sn_temp+1
	call	_send_nib
	call	_send_1

	movfw	sn_temp+0
	call	_send_nib
	call	_send_1

	movfw	sn_humi+2
	call	_send_nib
	call	_send_1

	movfw	sn_humi+1
	call	_send_nib
	call	_send_1

	movfw	sn_humi+0
	call	_send_nib
	call	_send_1

	movfw	sn_pres+2
	call	_send_nib
	call	_send_1

	movfw	sn_pres+1
	call	_send_nib
	call	_send_1

	movfw	sn_pres+0
	call	_send_nib
	call	_send_1

	movfw	sn_pres+3
	call	_send_nib
	call	_send_1

	movfw	sn_xor
	call	_send_nib
	call	_send_1

	movfw	sn_chck
	call	_send_nib
	call	_send_1

	return


;******************************************************************************
; send_nibble : send the LSB nibble of W
;******************************************************************************

_send_nib
	movwf	nib_tmp
	call	_send_bit
	call	_send_bit
	call	_send_bit
	goto	_send_bit

_send_bit
	rrf	nib_tmp,f
	btfsc	STATUS,C
	goto	_send_1
	goto	_send_0
	
_send_0
	bsf	T_TX
	call	_d_400
	call	_d_400
	bcf	T_TX
	goto	_d_400

_send_1
	bsf	T_TX
	call	_d_400
	bcf	T_TX
	call	_d_400
	goto	_d_400

_d_400
	movlw	.98
	movwf	nib_del
	goto	_tx_delay


;******************************************************************************
; tx_delay
;******************************************************************************
_tx_delay
	nop
	decfsz  nib_del,F
	goto    _tx_delay
	return

;******************************************************************************
; Binary to decimal conversion / 16 bits -> 5 digits
;
_hexToDec
	movwf	temp1
	incf	temp1,F
	bcf     STATUS,0	; clear the carry bit
	movlw   .16
	movwf   counter
	clrf    bcd_th_hu
	clrf    bcd_te_on
_loop16
	movfw	temp1
	movwf	FSR
	rlf     INDF, F
	decf	FSR,f
	rlf     INDF, F
	rlf     bcd_te_on, F
	rlf     bcd_th_hu, F

	decfsz  counter, F
	goto    _adjDEC

	return

_adjDEC
	movlw   bcd_te_on
	movwf   FSR
	call    _adjBCD

	movlw   bcd_th_hu
	movwf   FSR
	call    _adjBCD

	goto    _loop16

_adjBCD
	movlw   3
	addwf   0,W
	movwf   temp2
	btfsc   temp2,3	; test if result > 7
	movwf   0
	movlw   30
	addwf   0,W
	movwf   temp2
	btfsc   temp2,7	; test if result > 7
	movwf   0		; save as MSD

	return

;******************************************************************
	END