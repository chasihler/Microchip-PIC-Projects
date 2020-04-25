;******************************************************************* 
; Function:  Sends alive message, then echoes characters at 9600 bps 
; Processor: PIC16F628 at 4 MHz using internal RC oscillator 
; Hardware:  Testboard K4 
; Filename:  628uart.asm 
; Author:    Lars Petersen, oz1bxm@qsl.net 
; Website:   www.qsl.net/oz1bxm/PIC/pic.htm 
; Credit:    Tony Nixon's test program at
;            www.piclist.com/techref/microchip/16f877/setup.htm 
;******************************************************************* 

        LIST P=16F628, R=DEC    ; Use the PIC16F628 and decimal system 

        #include "P16F628.INC"  ; Include header file 

        __config  _INTRC_OSC_NOCLKOUT & _LVP_OFF & _WDT_OFF & _PWRTE_ON & _BODEN_ON 

        CBLOCK 0x20             ; Declare variable addresses starting at 0x20
	  dataL
        ENDC

        ORG    0x000            ; Program starts at 0x000 
;
; --------------------------------
; SET ANALOG/DIGITAL INPUTS PORT A
; --------------------------------
;
	movlw 7
	movwf CMCON		; CMCON=7 set comperators off         
; 
; ---------------- 
; INITIALIZE PORTS
; ---------------- 
; 
	movlw b'00000000'       ; set up portA
        movwf PORTA	
        
        movlw b'00000100'	; RB2(TX)=1 others are 0
        movwf PORTB

        bsf STATUS,RP0          ; RAM PAGE 1

        movlw 0xFF
        movwf TRISA		; portA all pins input

        movlw b'11110010'	; RB7-RB4 and RB1(RX)=input, others output 
        movwf TRISB

; ------------------------------------
; SET BAUD RATE TO COMMUNICATE WITH PC
; ------------------------------------
; Boot Baud Rate = 9600, No Parity, 1 Stop Bit
;
        movlw 0x19              ; 0x19=9600 bps (0x0C=19200 bps)
        movwf SPBRG
        movlw b'00100100'       ; brgh = high (2)
        movwf TXSTA             ; enable Async Transmission, set brgh

        bcf STATUS,RP0          ; RAM PAGE 0

        movlw b'10010000'       ; enable Async Reception
        movwf RCSTA
;
; ------------------------------------
; PROVIDE A SETTLING TIME FOR START UP
; ------------------------------------
;
        clrf dataL
settle  decfsz dataL,F
        goto settle

        movf RCREG,W
        movf RCREG,W
        movf RCREG,W            ; flush receive buffer
;
; ---------
; MAIN LOOP
; ---------
;
	call message        	; send "16F628 alive"
loop    call receive            ; wait for a char
	call send               ; send the char
        goto loop
;
; -------------------------------------------
; RECEIVE CHARACTER FROM RS232 AND STORE IN W
; -------------------------------------------
; This routine does not return until a character is received.
;
receive btfss PIR1,RCIF         ; (5) check for received data
        goto receive

        movf RCREG,W            ; save received data in W
        return
;
; -------------------------------------------------------------
; SEND CHARACTER IN W VIA RS232 AND WAIT UNTIL FINISHED SENDING 
; -------------------------------------------------------------
;
send    movwf TXREG             ; send data in W 

TransWt bsf STATUS,RP0		; RAM PAGE 1
WtHere  btfss TXSTA,TRMT        ; (1) transmission is complete if hi
        goto WtHere

        bcf STATUS,RP0          ; RAM PAGE 0
        return
;
; -------
; MESSAGE
; -------
;
message movlw 	'1'		
	call	send
	movlw 	'6'
	call	send
	movlw 	'F'
	call	send
	movlw 	'6'
	call	send
	movlw 	'2'
	call	send
	movlw 	'8'
	call	send
	movlw 	' '
	call	send
	movlw 	'a'
	call	send
	movlw 	'l'
	call	send
	movlw 	'i'
	call	send
	movlw 	'v'
	call	send
	movlw 	'e'
	call	send
	movlw 	0x0D	; CR
	call	send
	movlw 	0x0A	; LF 
	call	send
	return

        END