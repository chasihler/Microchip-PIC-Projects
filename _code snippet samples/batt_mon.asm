	#include <p18f4423.inc>	
	list r=d
	
	cblock 0
	chargecnt:4		;charge counter ("fuel gauge")
	moa,mob,moc,mod		;16x16 multiply operands
	mra,mrb,mrc,mrd		;16x16 multiply result register
	temp1,temp2,temp3	;Some temporary storage space
	bcdres:2
	i_batt:3		;battery current
	v_cel1:3		;cell voltages
	v_cel2:3
	v_cel3:3
	v_cel4:3
	v_cel5:3
	v_cel6:3
	v_cel7:3
	v_cel8:3
	v_pack:3		;pack voltage = sum(v_cell)
	counter:2		;counter for loops, etc
	endc

	org 0
	bra 	start

	org 8
	retfie

hextable	;convert nibble in W to its ASCII hex representation
	andlw	0x0F
	rlncf	wreg,f
	addwf	pcl,f
	dt	"0123456789ABCDEF"
	

start	clrf	bsr		;select the access bank


	;;;;;;;;;;;
	;Oscillator configuration
	;;;;;;;;;;;
	movlw	0x70		;Select 8MHz internal oscillator
	movwf	osccon

	;Wait for oscillator frequency to become stable
	;btfss	osccon,iofs
	;bra	$-2

	;;;;;;;;;;;
	;Set up ports
	;;;;;;;;;;;
	movlw	0x2F		;ADC pins inputs, others are outputs
	movwf	trisa
	movlw	0xCE		;PGM and ADC pins inputs, others are outputs
	movwf	trisb
	movlw	0xC0		;EUSART inputs, others outputs
	movwf	trisc
	movlw	0x00		;this port unused -- future expansion?
	movwf	trisd
	movlw	0x07		;Port E only has 3 pins on these devices
	movwf	trise
	clrf	lata		;all outputs off by default
	clrf	latb
	clrf	latc
	clrf	latd
	clrf	late

	;;;;;;;;;;;
	;PWM initializations (for negative voltage generator)
	;;;;;;;;;;;
	movlw	0x04		;Enable TMR2 - should run at 2MHz
	movwf	t2con
	movlw	.51		;Period of 104 = PWM frequency of 19.23kHz 
	movwf	pr2
	movlw	.26		;50% duty cycle
	movwf	ccpr1l
	movlw	0x0C		;Enable PWM module
	movwf	ccp1con
	
	
	;;;;;;;;;;;
	;Set up the ADC
	;;;;;;;;;;;
	movlw	0x13		;internal V-, external V+, all analog except AN12
	movwf	adcon1
	movlw	0x3E		;Pretty much just the slowest settings possible (left justified)
	movwf	adcon2
	movlw	0x01		;Turn it on and select channel 0
	movwf	adcon0

	;;;;;;;;;;;
	;Set up the EUSART
	;;;;;;;;;;;
	movlw	.25		;19,200 baud
	movwf	spbrg
	movlw	0x24		;enable 8-bit transmit, brgh = 1
	movwf	txsta
	movlw	0x90
	movwf	rcsta		;enable serial port

	;;;;;;;;;;;
	;Initialize the charge counter to 0xFFFFFFFF (fully charged)
	;;;;;;;;;;;
	setf	chargecnt
	setf	chargecnt+1
	setf	chargecnt+2
	setf	chargecnt+3

loop	;The main loop!

	btg	latb,0		;heartbeat -- signal we're alive!

	movlw	.18
	movwf	counter
	lfsr	0,i_batt
clear_loop
	clrf	postinc0
	decfsz	counter,f
	bra	clear_loop

	
	movlw	0x0D		;send a carriage return
	call	txbyte
	movlw	0x0A
	call	txbyte


	movlw	0x01		;Select channel 0 (current)
	movwf	adcon0
	call	delay		;wait about a millisecond
	bsf	adcon0,go
	call	adcwait		;wait for conversion to complete
	movff	adresh,i_batt+1	;save the result
	movff	adresl,i_batt+2
	call	convert

	;Update the charge counter
	;Invert bits and add one to "negativize" current reading
	movff	i_batt+2,temp3
	movff	i_batt+1,temp2
	setf	temp1
	comf	temp3,f
	comf	temp2,f
	movlw	0x01
	addwf	temp3,f
	movlw	0x00
	addwfc	temp2,f
	addwfc	temp1,f
	movf	temp3,w
	addwf	chargecnt+3,f
	movf	temp2,w
	addwfc	chargecnt+2,f
	movf	temp1,w
	addwfc	chargecnt+1,f
	movlw	0xFF
	addwfc	chargecnt,f

	movlw	0x20		;send a couple of spaces
	call	txbyte
	movlw	0x20
	call	txbyte

	movlw	0x05		;Select channel 1 (cell 1 voltage)
	movwf	adcon0
	call	delay		;wait about a millisecond
	bsf	adcon0,go
	call	adcwait		;wait for conversion to complete
	movff	adresh,v_cel1+1	;save the result
	movff	adresl,v_cel1+2	
	call	convert

	movlw	0x20		;send a couple of spaces
	call	txbyte
	movlw	0x20
	call	txbyte

	movlw	0x09		;Select channel 2 (cell 2 voltage)
	movwf	adcon0
	call	delay		;wait about a millisecond
	bsf	adcon0,go
	call	adcwait		;wait for conversion to complete
	movff	adresh,v_cel2+1	;save the result
	movff	adresl,v_cel2+2
	; New code for testing -- subtract the Channel 1 reading before display
	;Negate Cell 1 reading
	comf	v_cel1+2
	comf	v_cel1+1
	movlw	1
	addwf	v_cel1+2
	movlw	0
	addwfc	v_cel1+1
	;Subtract cell 1 voltage from cell 2 voltage
	movf	v_cel1+2,w
	addwf	adresl,f
	movf	v_cel1+1,w
	addwfc	adresh,f
	;End new section
	call	convert

	movlw	0x20		;send a couple of spaces
	call	txbyte
	movlw	0x20
	call	txbyte

	movlw	0x11		;Select channel 4
	movwf	adcon0
	call	delay		;wait about a millisecond
	bsf	adcon0,go
	call	adcwait		;wait for conversion to complete
	movff	adresh,v_cel3+1	;save the result
	movff	adresl,v_cel3+2
	; New code for testing -- subtract the Channel 2 reading before display
	;Negate Cell 2 reading
	comf	v_cel2+2
	comf	v_cel2+1
	movlw	1
	addwf	v_cel2+2
	movlw	0
	addwfc	v_cel2+1
	;Subtract cell 2 voltage from cell 4 voltage
	movf	v_cel2+2,w
	addwf	adresl,f
	movf	v_cel2+1,w
	addwfc	adresh,f
	;End new section
	call	convert

	movlw	0x20		;send a couple of spaces
	call	txbyte
	movlw	0x20
	call	txbyte

	movlw	0x15		;Select channel 5
	movwf	adcon0
	call	delay		;wait about a millisecond
	bsf	adcon0,go
	call	adcwait		;wait for conversion to complete
	movff	adresh,v_cel4+1	;save the result
	movff	adresl,v_cel4+2	
	; New code for testing -- subtract the Channel 4 reading before display
	;Negate Cell 3 reading
	comf	v_cel3+2
	comf	v_cel3+1
	movlw	1
	addwf	v_cel3+2
	movlw	0
	addwfc	v_cel3+1
	;Subtract cell 3 voltage from cell 4 voltage
	movf	v_cel3+2,w
	addwf	adresl,f
	movf	v_cel3+1,w
	addwfc	adresh,f
	;End new section
	call	convert

	movlw	0x20		;send a couple of spaces
	call	txbyte
	movlw	0x20
	call	txbyte

	movlw	0x19		;Select channel 6
	movwf	adcon0
	call	delay		;wait about a millisecond
	bsf	adcon0,go
	call	adcwait		;wait for conversion to complete
	movff	adresh,v_cel5+1	;save the result
	movff	adresl,v_cel5+2	
	; New code for testing -- subtract the Channel 5 reading before display
	;Negate Cell 5 reading
	comf	v_cel4+2
	comf	v_cel4+1
	movlw	1
	addwf	v_cel4+2
	movlw	0
	addwfc	v_cel4+1
	;Subtract cell 5 voltage from cell 6 voltage
	movf	v_cel4+2,w
	addwf	adresl,f
	movf	v_cel4+1,w
	addwfc	adresh,f
	;End new section
	call	convert

	movlw	0x20		;send a couple of spaces
	call	txbyte
	movlw	0x20
	call	txbyte

	movlw	0x1D		;Select channel 7
	movwf	adcon0
	call	delay		;wait about a millisecond
	bsf	adcon0,go
	call	adcwait		;wait for conversion to complete
	movff	adresh,v_cel6+1	;save the result
	movff	adresl,v_cel6+2	
	; New code for testing -- subtract the Channel 6 reading before display
	;Negate Cell 6 reading
	comf	v_cel5+2
	comf	v_cel5+1
	movlw	1
	addwf	v_cel5+2
	movlw	0
	addwfc	v_cel5+1
	;Subtract cell 6 voltage from cell 7 voltage
	movf	v_cel5+2,w
	addwf	adresl,f
	movf	v_cel5+1,w
	addwfc	adresh,f
	;End new section
	call	convert

	movlw	0x20		;send a couple of spaces
	call	txbyte
	movlw	0x20
	call	txbyte

	movlw	0x21		;Select channel 8
	movwf	adcon0
	call	delay		;wait about a millisecond
	bsf	adcon0,go
	call	adcwait		;wait for conversion to complete
	;Software offsets (newest testing method)
	movlw	low(300)
	addwf	adresl,f
	movlw	high(300)
	addwfc	adresh,f
	movff	adresh,v_cel7+1	;save the result
	movff	adresl,v_cel7+2	
	; New code for testing -- subtract the Channel 4 reading before display
	;Negate Cell 3 reading
	comf	v_cel6+2
	comf	v_cel6+1
	movlw	1
	addwf	v_cel6+2
	movlw	0
	addwfc	v_cel6+1
	;Subtract cell 3 voltage from cell 4 voltage
	movf	v_cel6+2,w
	addwf	adresl,f
	movf	v_cel6+1,w
	addwfc	adresh,f
	;End new section
	call	convert

	movlw	0x20		;send a couple of spaces
	call	txbyte
	movlw	0x20
	call	txbyte

	movlw	0x25		;Select channel 9
	movwf	adcon0
	call	delay		;wait about a millisecond
	bsf	adcon0,go
	call	adcwait		;wait for conversion to complete
	movff	adresh,v_cel8+1	;save the result
	movff	adresl,v_cel8+2	
	; New code for testing -- subtract the Channel 4 reading before display
	;Negate Cell 3 reading
	comf	v_cel7+2
	comf	v_cel7+1
	movlw	1
	addwf	v_cel7+2
	movlw	0
	addwfc	v_cel7+1
	;Subtract cell 3 voltage from cell 4 voltage
	movf	v_cel7+2,w
	addwf	adresl,f
	movf	v_cel7+1,w
	addwfc	adresh,f
	;End new section
	call	convert

	movlw	0x20		;send a couple of spaces
	call	txbyte
	movlw	0x20
	call	txbyte

	;Send the charge counter value to the terminal
	swapf	chargecnt,w
	call	hextable
	call	txbyte
	movf	chargecnt,w
	call	hextable
	call	txbyte
	swapf	chargecnt+1,w
	call	hextable
	call	txbyte
	movf	chargecnt+1,w
	call	hextable
	call	txbyte
	swapf	chargecnt+2,w
	call	hextable
	call	txbyte
	movf	chargecnt+2,w
	call	hextable
	call	txbyte
	swapf	chargecnt+3,w
	call	hextable
	call	txbyte
	movf	chargecnt+3,w
	call	hextable
	call	txbyte

	clrf	temp1		;use this as a counter -- we'll wait about 0.25 second
main_delay
	call	delay
	decfsz	temp1,f
	bra	main_delay
		


	bra	loop		;Do this forever.	
	

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;Subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

txbyte	btfss	txsta,trmt
	bra	$-2
	movwf	txreg
	return

rxbyte	btfss	pir1,rcif
	bra	$-2
	movf	rcreg,w
	return

delay	movlw	0x3
	movwf	counter
delay1	clrf	counter+1
	decfsz	counter+1,f
	bra	$-2
	decfsz	counter,f
	bra	delay1
	return

adcwait	btfsc	adcon0,go
	bra	$-2
	return

convert	

	;Three main tasks:
	; (1) Scales the result (converts to volts)
	; (2) Converts fixed point to packed BCD
	; (3) Prints to serial terminal
	;Only input arguments are in ADRESH:ADRESL.
	;Code uses temp1, temp2, temp3 for sotrage of intermediate values.
	clrf	temp1			;This should be zero for later use
	movff	adresh,temp2		;Copy the ADC result for further processing
	movff	adresl,temp3

	;Step 1:  Multiply the ADC reading by 8.
	bcf	status,c		;Clear the carry bit
	rlcf	temp3,f			;Shift our copied result left twice (24 bits)
	rlcf	temp2,f
	rlcf	temp1,f
	rlcf	temp3,f
	rlcf	temp2,f
	rlcf	temp1,f
	rlcf	temp3,f
	rlcf	temp2,f
	rlcf	temp1,f

	;Step 2:  Convert to packed BCD
	call	bin_bcd
	swapf	bcdres,w
	call	hextable
	call	txbyte
	movlw	'.'
	call	txbyte
	movf	bcdres,w
	call	hextable
	call	txbyte
	swapf	bcdres+1,w
	call	hextable
	call	txbyte
;	movf	bcdres+1,w
;	call	hextable
;	call	txbyte
	return	


	;Step 2:  Display the integer part
	;temp1 now contains the integer part of the voltage.  Easy, huh?
	swapf	temp1,w
	call	hextable
	call	txbyte
	movf	temp1,w
	call	hextable
	call	txbyte
	movlw	'.'
	call	txbyte

	;Step 3:  Convert the fractional part to packed BCD
;	movff	temp2,moa
;	movff	temp3,mob
;	movlw	high(.1000)
;	movwf	moc
;	movlw	low(.1000)
;	movwf	mod
;	call	mul16
;	movff	mra,temp2
;	movff	mrb,temp3

	;Step 4:  Align the fraction part for binary->BCD conversion
	rlcf	temp3,f
	rlcf	temp2,f
	rlcf	temp3,f
	rlcf	temp2,f
	rlcf	temp3,f
	rlcf	temp2,f
	rlcf	temp3,f
	rlcf	temp2,f
	rlcf	temp3,f
	rlcf	temp2,f
	rlcf	temp3,f
	rlcf	temp2,f			;The should-be MSB of the data is now properly positioned
					;for a BCD conversion

	;Step 5:  Convert fractional part to packed BCD
	call	bin_bcd

	;Step 6:  Display the fractional part
	movf	bcdres,w
	call	hextable
	call	txbyte
	swapf	bcdres+1,w
	call	hextable
	call	txbyte
	movf	bcdres+1,w
	call	hextable
	call	txbyte

	;Step 7:  Done!
	return

bin_bcd	;Converts the 10-bit left-aligned number in temp2:temp3 to packed BCD
	;in bcdres[2]
	clrf	bcdres
	clrf	bcdres+1
	movlw	12
	movwf	counter
bin_bcd_loop
	;This algorithm borrowed from an old Microchip forum post
	;Unfortunately, I don't remember which one -- I found the method years ago
	;and memorized it.
	bcf	status,c
	rlcf	temp3,f
	rlcf	temp2,f
	movf	bcdres+1,w
	addwfc	bcdres+1,w
	daw	bcdres+1
	movwf	bcdres+1
	movf	bcdres,w
	addwfc	bcdres,w
	daw
	movwf 	bcdres
	decfsz	counter,f
	bra	bin_bcd_loop
	return

mul16	;16x16 unsigned multiply routine
	;multiplicands in moa:mob and moc:mod
	;result in mra:mrd
	movf	moa,w
	mulwf	moc
	movff	prodh,mra
	movff	prodl,mrb
	movf	mob,w
	mulwf	mod
	movff	prodh,mrc
	movff	prodl,mrd
	movf	moa,w
	mulwf	mod
	movf	prodl,w
	addwf	mrc,f
	movf	prodh,w
	addwfc	mrb,f
	movlw	0
	addwfc	mra,f
	movf	mob,w
	mulwf	moc
	movf	prodl,w
	addwf	mrc,f
	movf	prodh,w
	addwfc	mrb,f
	movlw	0
	addwfc	mra,f
	return
	
	bra $

	end