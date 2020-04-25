;title "Roomba System 1"
;#define _version="0.01"
;-------------------------------------------
;
; Roomba Sumo Bot System 1
; by Charles M Douvier
;
;    Software History
;    v 0.01     LED ON, board check
;    v 0.02    pulse counter
;
;--------------------------------------------
;
;list p=16F628A
    LIST R=DEC
#include "p16F628a.inc"
;Macro
    __CONFIG _CP_OFF & _WDT_OFF & _INTOSC_OSC_NOCLKOUT & _BODEN_OFF & _PWRTE_ON
;

	variables	UDATA 0x10
	lcount		RES 1


    org    0

startup:

	NOP
	GOTO init 

    org 4
Int:

init:
	banksel	TXSTA			;initialize USART		
	movlw	B'10100100'		;Master mode, 8-bit, Async, High speed
	movwf	TXSTA
	movlw	.25			;9.6Kbaud @ 4MHz
	movwf	SPBRG
	banksel	RCSTA
	movlw	B'10010000'
	movwf	RCSTA
 
MainCode:
;goodies go here.
    BCF    STATUS,RP0
    BCF    STATUS,RP1
    CLRF    PORTA
    CLRF    PORTA

    MOVLW 0x07 ;Turn comparators off and
    MOVWF CMCON ;enable pins for I/O
    ;functions
    BCF STATUS, RP1
    BSF STATUS, RP0 ;Select Bank1
    MOVLW 0x18 ;Value used to initialize
    ;data direction
    MOVWF TRISA ;Set RA<4:3> as inputs
    ;TRISA<5> always
    ;read as ‘1’.
    ;TRISA<7:6>
    ;depend on oscillator
    ;mode
    MOVLW    0xFF
    MOVWF    TRISB
    BCF    STATUS,RP0
    BCF    STATUS,RP1
LOOP:
    BSF PORTA,0
    BSF PORTA,1

	banksel	TXREG			;move data into TXREG 
	movlw	"x"			;carriage return
	movwf	TXREG
	banksel	TXSTA
	btfss	TXSTA,TRMT		;wait for data TX
	goto	$-1
	banksel	PORTA


    GOTO LOOP
    end