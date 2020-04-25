;************************************************************************

#include <p18f4423.inc>	
	list p=18lf4423
	
;	cblock 0
;		Temp
;	end c

	org 0
	bra start
	
	org 8
	retfie
	
	
	
	start

;	port setup
	movlw	0xCE		;PGM and ADC pins inputs, others are outputs
	movwf	TRISB
	clrf	LATB

	movlw	b'00000010' 
	movwf	PORTB
	end


