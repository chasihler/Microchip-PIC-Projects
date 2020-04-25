;******************************************************************
;*								  *
;*	Filename: thb_maths.asm					  *
;*								  *
;*	J-Paul ROUBELAT - F6FBB - 5 March 2010			  *
;*								  *
;*	Code from Peter Hemsley (www.piclist.com)		  *
;*								  *
;******************************************************************

#include <thb_sensor.inc>

	GLOBAL	_subtract,_add,_multiply,_divide,_sh2rega,_sh2regb,_ush2rega,_ush2regb
	GLOBAL	_ee2rega,_ee2regb
	GLOBAL	_rega2sh,_bt2rega,_bt2regb,_rega2regb,_shlrega,_shrrega
	GLOBAL	REGA3,REGA2,REGA1,REGA0,REGB3,REGB2,REGB1,REGB0
	EXTERN	_e2p_read,e2p_addr

;******************************************************************************
; global variables declaration
;
	UDATA
REGA3		res	1	;msb
REGA2		res	1
REGA1		res	1
REGA0		res	1	;lsb

REGB3		res	1	;msb
REGB2		res	1
REGB1		res	1
REGB0		res	1	;lsb

REGC0		res	1	;lsb
REGC1		res	1
REGC2		res	1
REGC3		res	1	;msb

MTEMP		res	1
MCOUNT		res	1

;******************************************************************************
PGM	CODE

_rega2regb
	movfw	REGA3
	movwf	REGB3
	movfw	REGA2
	movwf	REGB2
	movfw	REGA1
	movwf	REGB1
	movfw	REGA0
	movwf	REGB0
	return

;====================================================================================	
;*** Transfert a byte in W to REGB
; expand the sign if needed
_bt2rega
	clrf	REGA3
	clrf	REGA2
	clrf	REGA1
	movwf	REGA0
	btfss	REGA0,7			; sign = Neg ?
	return
	decf	REGA3,F			; expand sign
	decf	REGA2,F
	decf	REGA1,F
	return

;====================================================================================	
;*** Transfert a byte in W to REGB
; expand the sign if needed
_bt2regb
	clrf	REGB3
	clrf	REGB2
	clrf	REGB1
	movwf	REGB0
	btfss	REGB0,7			; sign = Neg ?
	return
	decf	REGB3,F			; expand sign
	decf	REGB2,F
	decf	REGB1,F
	return

;====================================================================================	
;*** Transfert REGA to a short pointed by W
_rega2sh
	movwf	FSR
	movfw	REGA1			;MSB
	movwf	INDF
	incf	FSR,F
	movfw	REGA0			;LSB
	movwf	INDF
	return

;====================================================================================	
;*** Transfert a short pointed by W to REGA
; expand the sign if needed
_sh2rega
	clrf	REGA3
	clrf	REGA2
	movwf	FSR
	movfw	INDF
	movwf	REGA1
	incf	FSR,F
	movfw	INDF
	movwf	REGA0
	btfss	REGA1,7			; sign = Neg ?
	return
	decf	REGA3,F			; expand sign
	decf	REGA2,F
	return

;====================================================================================	
;*** Transfert a short pointed by W to REGA
; expand the sign if needed
_sh2regb
	clrf	REGB3
	clrf	REGB2
	movwf	FSR
	movfw	INDF
	movwf	REGB1
	incf	FSR,F
	movfw	INDF
	movwf	REGB0
	btfss	REGB1,7			; sign = Neg ?
	return
	decf	REGB3,F			; expand sign
	decf	REGB2,F
	return

;====================================================================================	
;*** Transfert a short pointed by W to REGA
; keep it unsigned
_ush2rega
	clrf	REGA3
	clrf	REGA2
	movwf	FSR
	movfw	INDF
	movwf	REGA1
	incf	FSR,F
	movfw	INDF
	movwf	REGA0
	return

;====================================================================================	
;*** Transfert a short pointed by W to REGA
; keep it unsigned
_ush2regb
	clrf	REGB3
	clrf	REGB2
	movwf	FSR
	movfw	INDF
	movwf	REGB1
	incf	FSR,F
	movfw	INDF
	movwf	REGB0
	return

;====================================================================================	
;*** Transfert a short from E2PROM pointed by W to REGA
; expand the sign if needed
_ee2rega
	clrf	REGA3
	clrf	REGA2
	movwf	e2p_addr
	call	_e2p_read
	movwf	REGA1
	incf	e2p_addr,F
	call	_e2p_read
	movwf	REGA0
	btfss	REGA1,7			; sign = Neg ?
	return
	decf	REGA3,F			; expand sign
	decf	REGA2,F
	return

;====================================================================================	
;*** Transfert a short from E2PROM pointed by W to REGA
; expand the sign if needed
_ee2regb
	clrf	REGB3
	clrf	REGB2
	movwf	e2p_addr
	call	_e2p_read
	movwf	REGB1
	incf	e2p_addr,F
	call	_e2p_read
	movwf	REGB0
	btfss	REGB1,7			; sign = Neg ?
	return
	decf	REGB3,F			; expand sign
	decf	REGB2,F
	return

;====================================================================================	
;*** shift left REGA by W times
_shlrega
	movwf	MCOUNT
_shlrega1
	bcf	STATUS,C
	rlf	REGA0,F
	rlf	REGA1,F
	rlf	REGA2,F
	rlf	REGA3,F
	decfsz	MCOUNT,F
	goto	_shlrega1
	return

;====================================================================================	
;*** shift right REGA by W times
;kepp the sign
_shrrega
	movwf	MCOUNT
_shrrega1
	bcf	STATUS,C
	rrf	REGA3,F
	rrf	REGA2,F
	rrf	REGA1,F
	rrf	REGA0,F
	btfsc	REGA3,6
	bsf	REGA3,7		; keep sign
	decfsz	MCOUNT,F
	goto	_shrrega1
	return

;;====================================================================================	
;;*** shift left REGB by W times
;_shlregb
;	movwf	MCOUNT
;_shlregb1
;	bcf	STATUS,C
;	rlf	REGB0,F
;	rlf	REGB1,F
;	rlf	REGB2,F
;	rlf	REGB3,F
;	decfsz	MCOUNT,F
;	goto	_shlregb1
;	return
;
;;====================================================================================	
;;*** shift right REGB by W times
;_shrregb
;	movwf	MCOUNT
;_shrregb1
;	bcf	STATUS,C
;	rrf	REGB3,F
;	rrf	REGB2,F
;	rrf	REGB1,F
;	rrf	REGB0,F
;	decfsz	MCOUNT,F
;	goto	_shrregb1
;	return
;
;====================================================================================	
;*** 32 BIT SIGNED SUTRACT ***
;REGA - REGB -> REGA
;Return carry set if overflow

_subtract
	call	_negateb	;Negate REGB
	skpnc
	return			;Overflow


;====================================================================================	
;*** 32 BIT SIGNED ADD ***
;REGA + REGB -> REGA
;Return carry set if overflow

_add
	movf	REGA3,w		;Compare signs
	xorwf	REGB3,w
	movwf	MTEMP

	call	_addba		;Add REGB to REGA

	clrc			;Check signs
	movf	REGB3,w		;If signs are same
	xorwf	REGA3,w		;so must result sign
	btfss	MTEMP,7		;else overflow
	addlw	0x80
	return

;====================================================================================	
;*** 32 BIT SIGNED MULTIPLY ***
;REGA * REGB -> REGA
;Return carry set if overflow

_multiply
	clrf	MTEMP		;Reset sign flag
	call	_absa		;Make REGA positive
	skpc
	call	_absb		;Make REGB positive
	skpnc
	return			;Overflow

	call	_movac		;Move REGA to REGC
	call	_clra		;Clear product

	movlw	D'31'		;Loop counter
	movwf	MCOUNT

_muloop	call	_slac		;Shift left product and multiplicand
	
	rlf	REGC3,w		;Test MSB of multiplicand
	skpnc			;If multiplicand bit is a 1 then
	call	_addba		;add multiplier to product

	skpc			;Check for overflow
	rlf	REGA3,w
	skpnc
	return

	decfsz	MCOUNT,f	;Next
	goto	_muloop

	btfsc	MTEMP,0		;Check result sign
	call	_negatea	;Negative
	return

;====================================================================================	
;*** 32 BIT SIGNED DIVIDE ***
;REGA / REGB -> REGA
;Remainder in REGC
;Return carry set if overflow or division by zero

_divide
	clrf	MTEMP		;Reset sign flag
	movf	REGB0,w		;Trap division by zero
	iorwf	REGB1,w
	iorwf	REGB2,w
	iorwf	REGB3,w
	sublw	0
	skpc
	call	_absa		;Make dividend (REGA) positive
	skpc
	call	_absb		;Make divisor (REGB) positive
	skpnc
	return			;Overflow

	clrf	REGC0		;Clear remainder
	clrf	REGC1
	clrf	REGC2
	clrf	REGC3
	call	_slac		;Purge sign bit

	movlw	D'31'		;Loop counter
	movwf	MCOUNT

_dvloop	call	_slac		;Shift dividend (REGA) msb into remainder (REGC)

	movf	REGB3,w		;Test if remainder (REGC) >= divisor (REGB)
	subwf	REGC3,w
	skpz
	goto	_dtstgt
	movf	REGB2,w
	subwf	REGC2,w
	skpz
	goto	_dtstgt
	movf	REGB1,w
	subwf	REGC1,w
	skpz
	goto	_dtstgt
	movf	REGB0,w
	subwf	REGC0,w
_dtstgt	skpc			;Carry set if remainder >= divisor
	goto	_dremlt

	movf	REGB0,w		;Subtract divisor (REGB) from remainder (REGC)
	subwf	REGC0,f
	movf	REGB1,w
	skpc
	incfsz	REGB1,w
	subwf	REGC1,f
	movf	REGB2,w
	skpc
	incfsz	REGB2,w
	subwf	REGC2,f
	movf	REGB3,w
	skpc
	incfsz	REGB3,w
	subwf	REGC3,f
	clrc
	bsf	REGA0,0		;Set quotient bit

_dremlt	decfsz	MCOUNT,f	;Next
	goto	_dvloop

	btfsc	MTEMP,0		;Check result sign
	call	_negatea	;Negative
	return

;UTILITY ROUTINES


;====================================================================================	
;Add REGB to REGA (Unsigned)
;Used by add, multiply,

_addba	movf	REGB0,w		;Add lo byte
	addwf	REGA0,f

	movf	REGB1,w		;Add mid-lo byte
	skpnc			;No carry_in, so just add
	incfsz	REGB1,w		;Add carry_in to REGB
	addwf	REGA1,f		;Add and propagate carry_out

	movf	REGB2,w		;Add mid-hi byte
	skpnc
	incfsz	REGB2,w
	addwf	REGA2,f

	movf	REGB3,w		;Add hi byte
	skpnc
	incfsz	REGB3,w
	addwf	REGA3,f
	return


;====================================================================================	
;Move REGA to REGC
;Used by multiply, sqrt

_movac	movf	REGA0,w
	movwf	REGC0
	movf	REGA1,w
	movwf	REGC1
	movf	REGA2,w
	movwf	REGC2
	movf	REGA3,w
	movwf	REGC3
	return


;====================================================================================	
;Clear REGB and REGA
;Used by sqrt

_clrba	clrf	REGB0
	clrf	REGB1
	clrf	REGB2
	clrf	REGB3

;====================================================================================	
;Clear REGA
;Used by multiply, sqrt

_clra	clrf	REGA0
	clrf	REGA1
	clrf	REGA2
	clrf	REGA3
	return


;====================================================================================	
;Check sign of REGA and convert negative to positive
;Used by multiply, divide, bin2dec, round

_absa	rlf	REGA3,w
	skpc
	return			;Positive

;====================================================================================	
;Negate REGA
;Used by absa, multiply, divide, bin2dec, dec2bin, round

_negatea
	movf	REGA3,w		;Save sign in w
	andlw	0x80

	comf	REGA0,f		;2's complement
	comf	REGA1,f
	comf	REGA2,f
	comf	REGA3,f
	incfsz	REGA0,f
	goto	_nega1
	incfsz	REGA1,f
	goto	_nega1
	incfsz	REGA2,f
	goto	_nega1
	incf	REGA3,f
_nega1
	incf	MTEMP,f		;flip sign flag
	addwf	REGA3,w		;Return carry set if -2147483648
	return


;====================================================================================	
;Check sign of REGB and convert negative to positive
;Used by multiply, divide

_absb	rlf	REGB3,w
	skpc
	return			;Positive

;====================================================================================	
;Negate REGB
;Used by absb, subtract, multiply, divide

_negateb
	movf	REGB3,w		;Save sign in w
	andlw	0x80

	comf	REGB0,f		;2's complement
	comf	REGB1,f
	comf	REGB2,f
	comf	REGB3,f
	incfsz	REGB0,f
	goto	_negb1
	incfsz	REGB1,f
	goto	_negb1
	incfsz	REGB2,f
	goto	_negb1
	incf	REGB3,f
_negb1
	incf	MTEMP,f		;flip sign flag
	addwf	REGB3,w		;Return carry set if -2147483648
	return


;====================================================================================	
;Shift left REGA and REGC
;Used by multiply, divide, round

_slac	rlf	REGA0,f
	rlf	REGA1,f
	rlf	REGA2,f
	rlf	REGA3,f
_slc	rlf	REGC0,f
	rlf	REGC1,f
	rlf	REGC2,f
	rlf	REGC3,f
	return

;====================================================================================	
;
; End of module
;
	END
