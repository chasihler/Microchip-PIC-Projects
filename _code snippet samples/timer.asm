;		File: Filename.asm
;       [dato] - [måned] - [år]
;
	processor	12f675				;Sets processor
	#include 	P12F675.INC
		
;   Set configuration bits using definitions from the include file, p16f877A.inc
	__config	_INTRC_OSC_NOCLKOUT & _PWRTE_OFF & _WDT_OFF & _CP_OFF & _CPD_OFF

Init:
			banksel	TRISIO
			MOVLW	0xf9			; all input but GPIO1 and GPIO2
			MOVWF	TRISIO
			banksel	GPIO
			MOVLW	0x02	
			MOVWF	GPIO

Timer:		MOVLW	5				; Counting 249 to overflow
			MOVWF	TMR0   		 
			CLRF	OPTION_REG
			BSF		OPTION_REG,0	; timer clock 1/4
			BCF		INTCON,T0IF		; clearing overflow flag 

Main:		BTFSS	INTCON,T0IF		; on Overflow TOIF is set and the program jump the goto
			GOTO	Main
			BTFSC	GPIO,1			; check if GPIO1 is sctiv setting GPIO2 if true
			GOTO	GPIO_2
			BTFSC	GPIO,2			; check if GPIO2 is sctiv setting GPIO1 if true
			GOTO	GPIO_1
			GOTO	Main

GPIO_1:		BSF		GPIO,1			; Setting GPIO1
			BCF		GPIO,2			; Resetting GPIO2
			BCF		INTCON,T0IF		; Restarting timer		
			GOTO	Main			

GPIO_2:		BSF		GPIO,2			; Setting GPIO2
			BCF		GPIO,1			; Resetting GPIO1
			BCF		INTCON,T0IF		; Restarting timer				
			GOTO	Main			


 END		;her slutter programmet...
