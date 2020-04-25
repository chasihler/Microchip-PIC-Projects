;******************************************************************
;*								  *
;*	Filename: thb_humidity.asm					  *
;*								  *
;*	J-Paul ROUBELAT - F6FBB - 5 March 2010			  *
;*								  *
;*	Management of the SHT11 (humidity sensor)								  *
;*								  *
;******************************************************************

#include <thb_sensor.inc>

	GLOBAL	_h_init,_read_uh,_clear_uh
	extern	_sleep16ms
	extern	UH,measure

;******************************************************************************
; global variables declaration
;
	UDATA

nbits		res	1	; Bit counter
byte		res	1	; byte shifter
;mes_msb		res	1	; measure MSB
;mes_lsb		res	1	; measure LSB


;******************************************************************************
PGM	CODE

;====================================================================================	
;
; init module
;
_h_init
	call	_SCK_LOW
	call	_SDT_HIGH
	return

_SCK_LOW
	bcf	G_SCK
	return

_SCK_HIGH
	bsf	G_SCK
	return

_SDT_HIGH			; high impedance by making SDT an input
;	bsf 	G_SDT	
	bsf 	STATUS,RP0	; bank 1
	bsf 	T_SDT		; make SDT pin an input
	bcf 	STATUS,RP0	; back to bank 0
	return

_SDT_LOW
	bcf 	G_SDT	
	bsf 	STATUS,RP0	; bank 1
	bcf 	T_SDT		; make SDT pin an output
	bcf 	STATUS,RP0	; back to bank 0
	return

;====================================================================================	
;
; start sequence
;
_h_start
	call	_SDT_HIGH
	call	_SCK_HIGH
	call	_SDT_LOW
	call	_SCK_LOW
	call	_SCK_HIGH
	call	_SDT_HIGH
	call	_SCK_LOW
	return

;====================================================================================	
;
; send a clock pulse
;
_h_clock
	call	_SCK_HIGH
	call	_SCK_LOW
	return

;====================================================================================	
;
; check a received ACK
;
_h_rcv_ack
	call	_SDT_HIGH	; Data TS
	bcf	STATUS,C	; C is zero if no error
	btfsc	G_SDT
	bsf	STATUS,C	; error: C=1 !
	goto	_h_clock	; clock pulse

;====================================================================================	
;
; send ACK
;
_h_snd_ack
	call	_SDT_LOW
	call	_h_clock	; clock pulse
	goto	_SDT_HIGH

;====================================================================================	
;
; send NACK
;
_h_snd_nack
	call	_SDT_HIGH
	goto	_h_clock	; clock pulse

;====================================================================================	
;
; wait for measure done
;
_h_wait_mes
	btfss	G_SDT		; wait for G_SDT = 1 -> measure begin
	goto	_h_wait_mes
_h_wait_mes1
	call	_sleep16ms
	btfsc	G_SDT		; wait for G_SDT = 0 -> measure end
	goto	_h_wait_mes1
	return

;====================================================================================	
;
; send bit depending of C flag
;
_h_snd_bit
	btfss	STATUS,C
	call	_SDT_LOW
	btfsc	STATUS,C
	call	_SDT_HIGH
	goto	_h_clock

;====================================================================================	
;
; receive bit and set C flag
;
_h_rcv_bit
	btfss	G_SDT
	bcf	STATUS,C
	btfsc	G_SDT
	bsf	STATUS,C
	goto	_h_clock

;====================================================================================	
;
; send byte in W to sensor
;
_h_snd_byte
	movwf	byte
	movlw	0x8
	movwf	nbits
_h_snd_b1
	rlf	byte,F
	call	_h_snd_bit
	decfsz	nbits,F
	goto	_h_snd_b1
	return

;====================================================================================	
;
; receive byte from sensor in W
;
_h_rcv_byte
	clrf	byte
	movlw	.8
	movwf	nbits
_h_rcv_b1
	call	_h_rcv_bit
	rlf	byte,F
	decfsz	nbits,F
	goto	_h_rcv_b1
	movfw	byte
	return

;====================================================================================	
;
; Send command in W and wait answer
;
_h_read_values
	call	_h_start	; start sequence
	call	_h_snd_byte	; Send 8 bits in W

	call	_h_rcv_ack	; read ACK
	btfsc	STATUS,C
	goto	_h_read_err	; ACK error

	call	_h_wait_mes	; wait for measure completion

	call	_h_rcv_byte	; receive msb 8 bits
	movwf	measure
	call	_h_snd_ack	; send ack

	call	_h_rcv_byte	; receive lsb 8 bits
	movwf	measure+1
	call	_h_snd_nack	; send nack - End of sequence

	movfw	measure+1	; add to current measures
	addwf	UH+2,F
	movfw	measure
	skpnc
	incfsz	measure,W
	addwf   UH+1,F          ;
	skpnc
	incf	UH,F

;	movlw	0x5A
;	movwf	measure
	return

_h_read_err
	return
;====================================================================================	
;
; Clear humidity
;
_clear_uh
	clrf	UH
	clrf	UH+1
	clrf	UH+2
	return

;====================================================================================	
;
; Read humidity
;
_read_uh
	movlw	0x05			; read humidity
	goto	_h_read_values
	
;====================================================================================	
;
; End of module
;
	END
