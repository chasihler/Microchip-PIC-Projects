;------------------------------------------------------------
;	WWVB_162TZX.ASM
;	
;	Decodes WWVB time data 
;	
;	Ammends it's own time with WWVB
;	
;	Calculates & displays HH:MM:SS
;
;	Displays a star * when valid WWVB is rx'd 
;	
;	Calculates & displays MM/DD/YYYY
;	
;	Displays results to 16x2 LCD
;
;	Small graphic indication when valid bit rx'd
;
;	With optional circuitry time sent serially when requested
;
;	Adjustable time zones with offset displayed
;
;	Serial output in local time
;
;	4 MHz XT Osc / Leap year bug fixed
;
;	TMR1 calcs have carry now
;				
;	4/11/11				 
;------------------------------------------------------------
        LIST    P=16F628A
	ERRORLEVEL	-302
        include <P16F628A.INC>
        __CONFIG        h'3f01'
;-------------------------------------------------------------
;               Port Set-up
;           0   1   2   3   4   5   6   7       bit
; port  A   In  Out E   Out TOG Out X   X	
;       B   Out TX  RX  RS  LCD LCD LCD LCD
;------------------------------------------------------------
;
	ORG	0			; start code
	goto	main
	ORG	4
	goto	int_service
;
;------------------------------------------------------------
;
        cblock  0x20
                temp                    ; (20h) 
                count                   ; counter
                mS			;
                pw_mS			;
                sort_temp		;
                p_count			;
                buffer			;
;                
                frame_0			;
                frame_1			;
                frame_2			;
                frame_3			;
                frame_4			;
                frame_5			;
;                
                data_byte0		; results
                data_byte1		;
                data_byte2		;
                data_byte3		; (30h)
                data_byte4		;
                data_byte5		;
;                                                                                                
                hund_per_sec		; 
                offset_tz		; timezone
		rotor			;
                send_reg		; serial TX data
                rx_serial		; serial RX data                
		lcd_data		; LCD display data                                
		lcd_count		;        
;                        
		delay_count		; delay routines
		bit_count		; 
;
		seconds			; timekeeping 
		old_seconds		;
		minutes			;
		old_minutes		; 
		hours			; (40h)
		old_hours		;
		day			; 
		month			;
		wwvb_minutes		;		
		wwvb_hours		;
		wwvb_days_hi		;
		wwvb_days_lo		;
		wwvb_year		;
;
                hex_hi                  ; hex-decimal conversion
                hex_lo			;
                thousands		;
		hundreds		;
                tens                    ; 
                ones                    ; 

;
		ee_addr			; stores/reads EE timezone offset
		ee_data			; (50h)
;
		FLAGS			; 
		FLAGS2			;
		temp_W			; int routine
		temp_S			; (54h)	 
        	endc
;------------------------------------------------------------        			
;
LINE_1		equ	0x80		;LCD addresses
LINE_2		equ	0xC0		;
;
US_60		equ	.195		;60 uS
US_125		equ	.130		;125 uS
MS_5		equ	.20		;5 mS
MS_15		equ	.60		;15 mS
MS_60		equ	.240		;60 mS
;
CR		equ	0x0d		;carriage return
LF		equ	0x0a		;linefeed
REQ_CHAR	equ	CR		;
;
PULSE_0		equ	.20		;0
PULSE_1		equ	.50		;1
PULSE_P		equ	.80		;P
LAST_P		equ	.60		;end of p count
;
HUNDTH_SEC	equ	.5000 +.70	;(2uS per count)+(tweak)
ONESEC		equ	.100		;1 second
;
#define	DATA_IN		PORTA,0		;input from wwvb rx
#define	WWVB_EN		PORTA,1		;WWVB module on/off
#define E_LINE          PORTA,2         ;LCD E
#define	TOGGLE		PORTA,4		;switch input
#define RS_LINE         PORTB,3         ;LCD RS
#define BIT_FLAG	FLAGS,0		;data bit buffer
#define	P_FLAG		FLAGS,1		;rxd P bit
#define	D_FLAG		FLAGS,2		;good data set
#define	ACT_FLAG	FLAGS,3		;pulse status
#define	NEW_DATA	FLAGS,4		;new data available
#define	SEC_FLAG	FLAGS2,0	;LCD update
#define SRX_FLAG	FLAGS2,1	;input change
#define	LEAP_YR		FLAGS2,2	;leap year indicator
#define	SIGN_FLAG	FLAGS2,3	;timezone +/-
;
header_0
	addwf	PCL,F
	dt	" WWVB",0		;5 char long
;
monthly
	addwf	PCL,F
	dt	0,.31,.28,.31,.30,.31,.30	;
	dt	.31,.31,.30,.31,.30,.31		;	
;
bit_wise
	addwf	PCL,F
	dt	2,2,0,0,0,1,0,0,0,0	;
	dt	2,0,0,0,0,1,0,0,0,0	;
	dt	2,0,0,0,0,1,0,0,0,0	;
	dt	2,0,0,0,0,1,0,0,0,0	;
	dt	2,0,0,0,0,1,0,0,0,0	;
	dt	2,0,0,0,0,1,0,0,0,0	;
;
header_tz
	addwf	PCL,F
	dt	"Offset",0		;6 char long		
;
show_rxing
	addwf	PCL,F
	dt	0x2d,0x3d,0xd0,0x3d,0x2d,0x3d	;	
;							
;
;============================================================
;
main
        movlw   0x07
        movwf   CMCON
	clrf	PORTA
	clrf	PORTB
        bsf     STATUS,RP0              ;switch to bank 1
        movlw   b'00010001'             ;port A in/outputs
        movwf   TRISA
        movlw   b'00000110'             ;port B in/outputs
        movwf   TRISB
;
        movlw   b'00100100'
	movwf	TXSTA			;tx enable, BRGH high
	movlw	.25  			;9k6 data rate
	movwf	SPBRG			;async setup
;	        
        movlw   b'00000001'             ;TMR0/4
        movwf   OPTION_REG
        movlw   b'00100001'             ;TMR1 RCIE int on
        movwf   PIE1                    ;perif int setup
        bcf     STATUS,RP0              ;back to page 0
;
        movlw   b'10010000'		;TX & RX
	movwf	RCSTA			;async serial enabled
        clrf    PIR1                    ;reset perif int
        movlw   b'00000100'             ;post/1, pre/1, on
        movwf   T2CON                   ;timer 2
        movlw   b'01000000'             ;PEIE on
	movwf	INTCON
	movlw	b'00010000'		;T1/2
	movwf	T1CON			;timer 1 setup
;
	call	delay_15mS
        call    init_lcd		;set up LCD display
;
;------------------------------------------------------------
start_here
	bcf	INTCON,GIE		;global ints off
	btfsc	INTCON,GIE
	goto	start_here
	call	clear_the_deck		;reset regs
	bcf	WWVB_EN			;turn on WWVB module	
	call	clear_LCD_display
;
	movlw	.100
	movwf	old_seconds
	movwf	old_minutes
	movwf	old_hours		;insure immediate update
	movlw	ONESEC
	movwf	hund_per_sec		;preload .01 sec counter
	bsf	T1CON,TMR1ON		;timer1 on
	bsf	INTCON,GIE		;ints on
;
;
	btfss	TOGGLE			;test power up toggle
	goto	tz_config		;do cfg
	clrf	ee_addr			;get stored value
	call	ee_read			;	
	movfw	ee_data			;
	andlw	b'00001111'		;lower nibble
	sublw	.12			;
	bnc	load_tz_default		;
	movfw	ee_data			;
	andlw	b'00011111'		;
	movwf	offset_tz		;load value
	goto	wait_new_pulse		;
load_tz_default
	clrf	offset_tz		;default GMT -0
	goto	wait_new_pulse		;
;
;
tz_config
	clrf	lcd_count		;clear pointer
	movlw	LINE_1			;1st position
	call	liter_c			;
tz_loop0
	movfw	lcd_count
	call	header_tz		;display "Offset"
	movwf	lcd_data
	movfw	lcd_data		;test for zero
	bz	tz_looper		;leave if 0 is returned
	call	liter_d
	incf	lcd_count,F
	goto	tz_loop0
tz_looper
	movlw	LINE_1 +8		;2nd 1/2
	call	liter_c			;display
	clrf	lcd_count		;will be counter
	bcf	SEC_FLAG		
get_syncd
	btfss	SEC_FLAG
	goto	get_syncd		;wait for edge
	bcf	SEC_FLAG
;
tz_looper1
	movlw	LINE_1 +8		;2nd 1/2
	call	liter_c			;display
	movlw	"+"
	btfss	lcd_count,4		;
	movlw	"-"
	movwf	lcd_data
	call	liter_d			;send sign
	movfw	lcd_count
	andlw	b'00001111'		;mask hi nibble		
	movwf	hex_lo	
	clrf	hex_hi
	call	hd_convert		;display current
;
	movlw	" "
	movwf	lcd_data
	movfw	tens
	xorlw	"0"			;
	bz	looper1_ones		;if 0, skip next
	movfw	tens
	movwf	lcd_data
looper1_ones
	call	liter_d			;10's
	movfw	ones	
	movwf	lcd_data
	call	liter_d			;1's
input_scan
	btfsc	TOGGLE			;toggle still low?
	goto	store_current_value	;no
	btfss	SEC_FLAG		;yes, sec flag hi yet?
	goto	input_scan		;no
	bcf	SEC_FLAG		;yes, clear
;
	incf	lcd_count,W
	andlw	b'00001111'		;
	xorlw	.13
	bz	swap_sign
	incf	lcd_count,F
	goto	tz_looper1		;display new value
;
swap_sign
	movlw	.16
	addwf	lcd_count,F		;inc hi nibble
	movlw	b'00010000'		;mask all but bit 4
	andwf	lcd_count,F
	goto	tz_looper1		;display new value
;
store_current_value
	movfw	lcd_count
	movwf	offset_tz		;store in working reg
	clrf	ee_addr
	movfw	offset_tz
	movwf	ee_data
	call	ee_write		;store value & sign
;
	call	clear_LCD_display
;
;	End of configure routine			
;------------------------------------------------------------
;	Start of working program
;
wait_new_pulse
	call	do_rx_indicate		;on screen indicator
	btfsc	NEW_DATA		;flag set?
	goto	P0
	btfsc	SEC_FLAG
	call	update_time_display	;update every second	
	goto	wait_new_pulse		;no, loop back
;
P0	
	bcf	P_FLAG			;clear P_FLAG
	movlw	PULSE_0 -2		;
	subwf	pw_mS,W			;pw_mS - 0 min
	bnc	bad_pulse		;pw_mS < 0 min
	movlw	PULSE_0 +2
	movwf	sort_temp
	movfw	pw_mS
	subwf	sort_temp,W		;0 max - pw_mS	
	bnc	P1			;pulse wider than 0 max
	bcf	BIT_FLAG		;bit 0, flag clear
	goto	grinder
;
P1
	movlw	PULSE_1 -4
	subwf	pw_mS,W			;pw_mS - 1 min
	bnc	bad_pulse		;pw_mS < 1 min
	movlw	PULSE_1 +4
	movwf	sort_temp
	movfw	pw_mS
	subwf	sort_temp,W		;1 max - pw_mS	
	bnc	P2			;pulse wider than 1 max
	bsf	BIT_FLAG		;bit 1, flag set
	goto	grinder
;		
P2
	movlw	PULSE_P -6
	subwf	pw_mS,W			;pw_mS - P min
	bnc	bad_pulse		;pw_mS < P min
	movlw	PULSE_P +6
	movwf	sort_temp
	movfw	pw_mS
	subwf	sort_temp,W		;P max - pw_mS	
	bnc	bad_pulse		;pulse wider than P max
	bsf	P_FLAG			;good P pulse, set P_FLAG
;	
grinder
	bcf	NEW_DATA		;clear flag

	movfw	p_count			;pointer to W
	call	bit_wise		;lookup next
	movwf	buffer			;store W in buffer
	movfw	buffer			;test contents of buffer
	bz	make_byte		;W = 0
	movlw	1
	xorwf	buffer,W		;
	bz	skip_bit		;W = 1			`
	movlw	2	
	xorwf	buffer,W		;
	bnz	bad_pulse		;huh?? W > 2
	btfss	P_FLAG			;expect PFLAG set
	goto	bad_pulse		;no P flag, reset
	incf	p_count,F		;inc pointer
	incf	rotor,F			;
	clrf	buffer			;reset bit storage
	goto	wait_new_pulse	
;
make_byte
	btfsc	P_FLAG			;test for P pulse
	goto	bad_pulse		;seen P flag, reset
	call	bits2bytes		;roll result into array				
	incf	p_count,F		;inc p counter
	clrf	buffer			;reset bit storage
	incf	rotor,F			;
	movlw	LAST_P			;test for p count end
	xorwf	p_count,W
	skpz
	goto	wait_new_pulse
;	
	movfw	data_byte0		;xfer data array
	movwf	frame_0			;
	movfw	data_byte1		;
	movwf	frame_1			;
	movfw	data_byte2		;
	movwf	frame_2			;
	movfw	data_byte3		;
	movwf	frame_3			;
	movfw	data_byte4		;
	movwf	frame_4			;
	movfw	data_byte5		;
	movwf	frame_5			;
	clrf	p_count			;reset p counter
;
	bsf	D_FLAG			;good data set
	goto	convert_BCD		;						
;
skip_bit	
	btfsc	P_FLAG			;test for P pulse
	goto	bad_pulse		;seen P flag, reset
	incf	p_count,F		;inc p counter
	clrf	buffer			;reset bit storage
	incf	rotor,F			;
	goto	wait_new_pulse	
;	
bad_pulse				;out of spec pulse
	bcf	NEW_DATA		;clear flag
	clrf	p_count			;reset p counter
	bcf	D_FLAG			;clear flag	
	goto	wait_new_pulse		;start over		
;
;------------------------------------------------------------
;	Hex data is converted to usable ASCII
;
convert_BCD
	clrf	wwvb_minutes
	clrf	wwvb_hours
	clrf	wwvb_days_hi
	clrf	wwvb_days_lo
	clrf	wwvb_year
;	
	movlw	.1		
	btfsc	frame_0,0	
	addwf	wwvb_minutes,F		;calc wwvb minutes
	movlw	.2
	btfsc	frame_0,1
	addwf	wwvb_minutes,F
	movlw	.4
	btfsc	frame_0,2
	addwf	wwvb_minutes,F	
	movlw	.8
	btfsc	frame_0,3
	addwf	wwvb_minutes,F
	movlw	.10
	btfsc	frame_0,4
	addwf	wwvb_minutes,F		
	movlw	.20
	btfsc	frame_0,5
	addwf	wwvb_minutes,F
	movlw	.40
	btfsc	frame_0,6
	addwf	wwvb_minutes,F	
;	
	movlw	.1
	btfsc	frame_1,0
	addwf	wwvb_hours,F		;calc wwvb hours
	movlw	.2
	btfsc	frame_1,1
	addwf	wwvb_hours,F	
	movlw	.4
	btfsc	frame_1,2
	addwf	wwvb_hours,F	
	movlw	.8
	btfsc	frame_1,3
	addwf	wwvb_hours,F	
	movlw	.10
	btfsc	frame_1,4
	addwf	wwvb_hours,F	
	movlw	.20
	btfsc	frame_1,5
	addwf	wwvb_hours,F	
;
	movlw	.1
	btfsc	frame_3,4
	addwf	wwvb_days_lo,F		;calc wwvb days
	movlw	.2
	btfsc	frame_3,5
	addwf	wwvb_days_lo,F
	movlw	.4
	btfsc	frame_3,6
	addwf	wwvb_days_lo,F
	movlw	.8
	btfsc	frame_3,7
	addwf	wwvb_days_lo,F			
;
	movlw	.10
	btfsc	frame_2,0
	addwf	wwvb_days_lo,F
	movlw	.20
	btfsc	frame_2,1
	addwf	wwvb_days_lo,F
	movlw	.40
	btfsc	frame_2,2
	addwf	wwvb_days_lo,F
	movlw	.80
	btfsc	frame_2,3
	addwf	wwvb_days_lo,F
;			
	movlw	.100
	btfss	frame_2,4
	goto	wwvb_200		;
	addwf	wwvb_days_lo,F
	skpnc
	incf	wwvb_days_hi,F		;inc on overflow of low byte
wwvb_200		
	movlw	.200
	btfsc	frame_2,5
	addwf	wwvb_days_lo,F
	skpnc
	incf	wwvb_days_hi,F		;inc on overflow of low byte
;
	movlw	.1
	btfsc	frame_5,4
	addwf	wwvb_year,F		;calc wwvb year
	movlw	.2
	btfsc	frame_5,5
	addwf	wwvb_year,F
	movlw	.4
	btfsc	frame_5,6
	addwf	wwvb_year,F
	movlw	.8
	btfsc	frame_5,7
	addwf	wwvb_year,F			
;
	movlw	.10
	btfsc	frame_4,0
	addwf	wwvb_year,F
	movlw	.20
	btfsc	frame_4,1
	addwf	wwvb_year,F
	movlw	.40
	btfsc	frame_4,2
	addwf	wwvb_year,F
	movlw	.80
	btfsc	frame_4,3
	addwf	wwvb_year,F	
;
month_calc
	bsf	LEAP_YR
	btfss	frame_5,3		;1=leap year(4-11-11)
	bcf	LEAP_YR			;leap year test
;
	movfw	wwvb_minutes
	movwf	minutes
	movfw	wwvb_hours
	movwf	hours
;
;	adding/subtracting timezone
;
	movf	offset_tz,F		;zero test
	bz	zulu_solong		;if TZ = 0 skip next
	btfss	offset_tz,4		;test sign
	goto	zulu_minus
zulu_plus
	movfw	offset_tz
	andlw	b'00001111'
	addwf	hours,F			;check for >23
	movfw	hours
	sublw	.24			;24 - W
	bz	at_zulu
	goto	tz_work_done		;done, start over
;
	movlw	.24
	subwf	hours,F			;hours - 24
	goto	tz_work_done		;start over
;
at_zulu
	clrf	hours			;reset to zero
	goto	tz_work_done		;start over
;
zulu_minus
	movfw	offset_tz
	andlw	b'00001111'
	subwf	hours,W	
	bz	at_zulu			;zero, offset = hours
	bnc	sub_w_hours		;offset > hours
	movfw	offset_tz
	andlw	b'00001111'
	subwf	hours,F			;hours - offset
	goto	tz_work_done
sub_w_hours
	movfw	hours
	subwf	offset_tz,W		;offset - hours = delta
	sublw	.24
	movwf	hours			;24 - delta
;
;
tz_work_done
	btfsc	offset_tz,4		;test sign
	goto	zulu_add
zulu_away
	movfw	wwvb_hours
	subwf	hours,W
	bnc	zulu_solong		;wwvbhours > hours
	movf	wwvb_days_lo,F		;check for 0
	bnz	zulu_away1		;not 0
	movf	wwvb_days_hi,F		;0, check hi byte
	bz	zulu_solong		;hi byte 0, exit no dec
	decf	wwvb_days_hi,F		;hi byte -1
zulu_away1
	decf	wwvb_days_lo,F		;subtract a day
	goto	zulu_solong
zulu_add
	movfw	wwvb_hours
	subwf	hours,W
	bc	zulu_solong		;
	movlw	.1
	addwf	wwvb_days_lo,F		;add a day
	skpnc
	incf	wwvb_days_hi,F	
zulu_solong
;
	movfw	wwvb_days_hi
	movwf	hex_hi
	movfw	wwvb_days_lo
	movwf	hex_lo	
	clrf	count
month_calc_loop
	movfw	count
	call	monthly			;lookup table to W
	movwf	buffer			;buffer W
	movlw	.2
	xorwf	count,W			;test for Feb
	bnz	L0			;not Feb (2)
	btfsc	LEAP_YR			;test leap year flag (1=LY)
	incf	buffer,F		;add one day
L0
	movfw	buffer			;buffer to W	
	subwf	hex_lo,F
	bz	M2			;
	bc	M0			;lookup <  days_lo
	movfw	hex_hi			;hex_hi = 0?
	bz	M1			;yes	
	decf	hex_hi,F		;lookup = > days_lo	
M0
	incf	count,F			;no
	goto	month_calc_loop		;lookup days < days_hi,lo
M1
	movfw	count			;
	movwf	month			;count = month
	movfw	buffer
	addwf 	hex_lo,W		;remainder = day	
	movwf	day
	goto	update_time
M2
	movfw	hex_hi			;hex_hi = 0?
	bz	M3			;yes	
	goto	M0
M3
	movfw	count			;
	movwf	month			;count = month
	movfw	buffer
	movwf	day
;	
;	
update_time
	movlw	.59
	movwf	seconds			;
	clrf	old_seconds		;set up to roll	

	goto	wait_new_pulse	
;
;************************************************************
;
int_service
        movwf   temp_W
        swapf   STATUS,W
        movwf   temp_S
	bcf	STATUS,RP0		;bank 0
;
	btfss	PIR1,TMR1IF		;
	goto	serial_receive_handler	;serial interrupt	
;
	bcf	PIR1,TMR1IF		;clear flag
	movlw	high HUNDTH_SEC
	subwf	TMR1H,F			;1/100th sec timeout
	movlw	low HUNDTH_SEC
	subwf	TMR1L,F
	skpc
	decf	TMR1H,F			;carry
	incf	mS,F			;inc pulse timer
;
	btfss	ACT_FLAG		;get pulse status
	goto	D1			;pulse is inactive
	btfss	DATA_IN			;pulse active, check
	goto	D2			;still low & counting
	movfw	mS			;pulse ended
	movwf	pw_mS			;buffer result	
	bcf	ACT_FLAG		;PW captured, clear PULSE
	bsf	NEW_DATA		;set new data flag
	goto	D2			;
D1
	btfsc	DATA_IN			;data still hi?
	goto	D2			;yes, exit
	clrf	mS			;no, zero timer
	bsf	ACT_FLAG		;set status flag
D2					
 	decfsz	hund_per_sec,F		;1 second timeout
	goto 	outta_here
;
	movlw	ONESEC
	movwf	hund_per_sec		;reload 1 second timeout
	incf	seconds,W
	bsf	SEC_FLAG		;signal LCD update	
	xorlw	.60
	bz	another_minute
	incf	seconds,F		;inc seconds
	goto 	outta_here
another_minute	
	clrf	seconds			;>59 seconds
	incf	minutes,W
	xorlw	.60
	bz	another_hour
	incf	minutes,F		;inc minutes
	goto 	outta_here
another_hour	
	clrf	minutes
	incf	hours,W
	xorlw	.24
	bz	another_day
	incf	hours,F			;inc hours	
	goto 	outta_here
another_day	
	clrf	hours
	btfsc	D_FLAG			;test valid data
	goto 	outta_here		;valid, skip next
	movlw	.1
	addwf	wwvb_days_lo,F
	skpnc
	incf	wwvb_days_hi,F		;inc day
;	call	month_calculation
	goto 	outta_here	
;
serial_receive_handler
	btfss	PIR1,RCIF
	goto	outta_here
        movlw   b'00000110'
        andwf   RCSTA,W                 ;check error flags
        bnz     serial_error
        movfw   RCREG
        movwf   rx_serial               ;xfer data byte to rx_serial
	bsf	SRX_FLAG		;
	xorlw	REQ_CHAR		;SRX_FLAG set if REQ_CHAR seen
	skpz
	bcf	SRX_FLAG		;
        goto    outta_here
serial_error
        btfss   RCSTA,FERR
        goto    check_orrun
        movfw   RCREG                   ;read byte, don't store
check_orrun
        btfss   RCSTA,OERR
        goto    outta_here
        bcf     RCSTA,CREN
        nop
        bsf     RCSTA,CREN              ;cycle CREN bit to reset	
;
outta_here
        swapf   temp_S,W
        movwf   STATUS
        swapf   temp_W,F
        swapf   temp_W,W
        retfie                          ;return to program
;
;============================================================	
;	Subroutines
;============================================================
;
clear_the_deck
	movlw	0x20
	movwf 	FSR
clearing_loop
	clrf	INDF
	incf	FSR,W
	xorlw	0x7f
	bz	deck_clear
	incf	FSR,F
	goto	clearing_loop
deck_clear	
	return
;	
;------------------------------------------------------------
;
bits2bytes
	setc
	btfss	BIT_FLAG
	clrc
	rlf	data_byte5,F		;rotate bits in
	rlf	data_byte4,F		;to data registers
	rlf	data_byte3,F		;
	rlf	data_byte2,F		;
	rlf	data_byte1,F		;
	rlf	data_byte0,F		;
	return													
;			
;------------------------------------------------------------
;               hex to decimal convert hex registers (hi-lo)
;------------------------------------------------------------
;
hd_convert
        movlw   0x30                    	;all zero ascii
        movwf   ones
        movwf   tens
        movwf   hundreds
        movwf   thousands
thou_hd
        movlw   0x03
        subwf   hex_hi,W
        bnc     hund_hd
        movlw   0x03
        subwf   hex_hi,F
        movlw   0xe8
        subwf   hex_lo,W
        bnc     thou_check
do_lo1
        movlw   0xe8
        subwf   hex_lo,F
        incf    thousands,F             	;1000's + 1
        goto    thou_hd
thou_check
        movfw   hex_hi
        skpnz
        goto    add_back_1k
        decf    hex_hi,F
        goto    do_lo1
add_back_1k
        movlw   0x03
        addwf   hex_hi,F
hund_hd
        movlw   0x64
        subwf   hex_lo,W
        bnc     hund_check
do_lo2
        movlw   0x64
        subwf   hex_lo,F
        incf    hundreds,F              	;100's + 1
        goto    hund_hd
hund_check
        movfw   hex_hi
        skpnz
        goto    tens_hd
        decf    hex_hi,F
        goto    do_lo2
tens_hd
        movlw   0x0a
        subwf   hex_lo,F
        bnc     add_back_10
        incf    tens,F                  	;10's + 1
        goto    tens_hd
add_back_10
        movlw   0x0a
        addwf   hex_lo,F
        movfw   hex_lo
        addwf   ones,F                  	;remainder = 1's
        return
;
;------------------------------------------------------------
;	EEMEM read write
ee_write
	movlw	EEADR
	movwf	FSR
	movfw	ee_addr
	movwf	INDF
	movlw	EEDATA
	movwf	FSR
	movfw	ee_data
	movwf	INDF
;
	bsf	STATUS,RP0
	bsf	EECON1,WREN
off_int
	bcf	INTCON,GIE		;ints off
	btfsc	INTCON,GIE		;insure int are off
	goto	off_int				
	movlw	0x55
	movwf	EECON2
	movlw	0xaa
	movwf	EECON2
	bsf	EECON1,WR
ee_wr_wait
	btfsc	EECON1,WR
	goto	ee_wr_wait
	bcf	EECON1,WREN
	bsf	INTCON,GIE		;ints on
	bcf	STATUS,RP0
	return
ee_read
	movlw	EEADR
	movwf	FSR
	movfw	ee_addr
	movwf	INDF
	bsf	STATUS,RP0
	bsf	EECON1,RD
	bcf	STATUS,RP0
	movlw	EEDATA
	movwf	FSR
	movfw	INDF
	movwf	ee_data	
	return
;        
;------------------------------------------------------------
;
update_time_display
	bcf	SEC_FLAG		;clear flag
	movfw	hours
	xorwf	old_hours,W		;hours change?
	bz	check_minutes		;no, check minutes
	movlw	LINE_1 +.0		;yes
	call	liter_c			;write to 0 position
	movfw	hours
	movwf	old_hours		;update old hours
	movwf	hex_lo
	clrf	hex_hi
	call	hd_convert		;hours to LCD
	movfw	tens
	movwf	lcd_data
	call	liter_d	
	movfw	ones
	movwf	lcd_data
	call	liter_d	
	movlw	":"
	movwf	lcd_data
	call	liter_d			;insert semicolon
;	
check_minutes	
	movfw	minutes
	xorwf	old_minutes,W		;minutes change?
	bz	check_seconds		;no, check seconds
	movlw	LINE_1 +.3		;yes
	call	liter_c			;write to 3 position
	movfw	minutes
	movwf	old_minutes		;update old minutes	
	movwf	hex_lo
	clrf	hex_hi
	call	hd_convert
	movfw	tens			;minutes to LCD
	movwf	lcd_data
	call	liter_d	
	movfw	ones
	movwf	lcd_data
	call	liter_d
	movlw	":"
	movwf	lcd_data
	call	liter_d			;insert semicolon
	movlw	LINE_1 +.10
	movwf	lcd_data
	call	liter_c			;to R side
	call	insert_space
	call	insert_space
	call	insert_space		;clear old
;
;	Insert time zone offset +/-12
;
	movlw	LINE_1 +.10
	movwf	lcd_data
	call	liter_c			;back to 1st char
;	
	movlw	"+"
	btfss	offset_tz,4		;
	movlw	"-"
	movwf	lcd_data
	call	liter_d			;send sign
	movfw	offset_tz
	andlw	b'00001111'		;mask hi nibble		
	movwf	hex_lo	
	clrf	hex_hi
	call	hd_convert		;display current
;
	movfw	tens
	xorlw	"0"			;
	bz	little_ones		;if 0, skip 10's
	movfw	tens
	movwf	lcd_data
	call	liter_d			;10's
little_ones
	movfw	ones	
	movwf	lcd_data
	call	liter_d			;1's
;
	movlw	LINE_1 +.15
	call	liter_c			;write to 16 position		
	movlw	"*"
	btfss	D_FLAG			;check on the minute
	movlw	" "
	movwf	lcd_data		;display "*" if fresh wwvb
	call	liter_d	
;
update_MDY_display
	call	second_line_display	;LCD line #2
	movfw	month			;display month
	movwf	hex_lo
	clrf	hex_hi
	call 	hd_convert
	movfw	tens
	movwf 	lcd_data
	call	liter_d
	movfw	ones
	movwf	lcd_data
	call	liter_d
	movlw	"/"
	movwf	lcd_data
	call	liter_d
	movfw	day			;display day
	movwf	hex_lo
	clrf	hex_hi
	call 	hd_convert
	movfw	tens
	movwf 	lcd_data
	call	liter_d
	movfw	ones
	movwf	lcd_data
	call	liter_d	
	movlw	"/"
	movwf	lcd_data
	call	liter_d
	movlw	"2"			;display year
	movwf	lcd_data
	call	liter_d
	movlw	"0"
	movwf	lcd_data
	call	liter_d	
	movfw	wwvb_year		
	movwf	hex_lo
	clrf	hex_hi
	call 	hd_convert
	movfw	tens
	movwf 	lcd_data
	call	liter_d
	movfw	ones
	movwf	lcd_data
	call	liter_d			;
;
	clrf	lcd_count		;clear pointer
display_wwvb
	movfw	lcd_count
	call	header_0		;display " WWVB"
	movwf	lcd_data
	movfw	lcd_data		;test for zero
	bz	check_seconds		;leave if 0 is returned
	call	liter_d
	incf	lcd_count,F
	goto	display_wwvb
;
check_seconds
	movlw	LINE_1 +.6
	movwf	lcd_data
	call	liter_c			;write to 6 position
	movfw	seconds			;
	movwf	hex_lo
	clrf	hex_hi
	call	hd_convert
	movfw	tens			;seconds to LCD
	movwf	lcd_data
	call	liter_d	
	movfw	ones
	movwf	lcd_data
	call	liter_d
;
do_rx_indicate
	movlw	LINE_1 +.13
	movwf	lcd_data
	call	liter_c			;write to 13 position
	movlw	b'00000101'
	subwf	rotor,W			;
	skpnc	
	clrf	rotor			;>4, reset
	movfw	rotor			;
	call	show_rxing		;for lookup table
	movwf	lcd_data
	call	liter_d			;overwrite old
;
	btfss	SRX_FLAG		;do serial if SRX_FLAG set
	return
;
;------------------------------------------------------------
;
;	ISO 8601 format output
;
rs232_time
	bcf	SRX_FLAG		;clear flag
	movlw	"2"
	movwf	send_reg
	call	do_serial_send		;send leading 20xx
	movlw	"0"
	movwf	send_reg
	call	do_serial_send		;
	
	movfw	wwvb_year		;convert year to ASCII
	movwf	hex_lo
	clrf	hex_hi
	call	hd_convert
	movfw	tens			;day tens
	movwf	send_reg
	call	do_serial_send	
	movfw	ones
	movwf	send_reg
	call	do_serial_send		;day ones
	movlw	"-"
	movwf	send_reg
	call	do_serial_send		;send dash	
;
	movfw	month			;convert month to ASCII
	movwf	hex_lo
	clrf	hex_hi
	call	hd_convert
	movfw	tens			;month tens
	movwf	send_reg
	call	do_serial_send	
	movfw	ones
	movwf	send_reg
	call	do_serial_send		;month ones
	movlw	"-"
	movwf	send_reg
	call	do_serial_send		;send dash
;
	movfw	day			;convert day to ASCII
	movwf	hex_lo
	clrf	hex_hi
	call	hd_convert
	movfw	tens			;day tens
	movwf	send_reg
	call	do_serial_send	
	movfw	ones
	movwf	send_reg
	call	do_serial_send		;day ones
	movlw	" "
	movwf	send_reg
	call	do_serial_send		;space
;
	movfw	hours
	movwf	hex_lo
	clrf	hex_hi
	call	hd_convert		;hours
	movfw	tens
	movwf	send_reg
	call	do_serial_send
	movfw	ones
	movwf	send_reg
	call	do_serial_send
	movlw	":"
	movwf	send_reg
	call	do_serial_send		;insert semicolon
;
	movfw	minutes
	movwf	hex_lo
	clrf	hex_hi
	call	hd_convert
	movfw	tens			;minutes
	movwf	send_reg
	call	do_serial_send
	movfw	ones
	movwf	send_reg
	call	do_serial_send
;
	btfss	TOGGLE
	goto	zulu_wrapup
;
	movlw	":"
	movwf	send_reg
	call	do_serial_send		;insert semicolon	
	movfw	seconds			;optional seconds field
	movwf	hex_lo
	clrf	hex_hi
	call	hd_convert
	movfw	tens			;seconds
	movwf	send_reg
	call	do_serial_send
	movfw	ones
	movwf	send_reg
	call	do_serial_send
zulu_wrapup
	movlw	" "
	movwf	send_reg
	call	do_serial_send		;space
	movlw	"*"
	btfss	D_FLAG			;check for lock
	movlw	" "
	movwf	send_reg
	call	do_serial_send		;indicate lock w/star	
	movlw	CR
	movwf	send_reg
	call	do_serial_send		;send carriage return	
;	movlw	LF
;	movwf	send_reg
;	call	do_serial_send		;send line feed	
;		
	return
;
;------------------------------------------------------------
;               LCD Routines
;------------------------------------------------------------
init_lcd
        bcf     E_LINE                  ;E line low
        bcf     RS_LINE                 ;RS low, control
        call    delay_15mS              ;delay 15 mS
        movlw   0x30                    ;8-bit, 5X7 mode
        movwf   lcd_data
        call    flipbit
        call    pulse
	call	delay_15mS
        movlw   0x20                    ;4-bit, 5x7 mode
        movwf   lcd_data
        call    flipbit
        call    pulse
	call	delay_15mS
        movlw   0x28                    ;4-bit, 5x7 mode
        call    liter_c                 ;send both nybbles
        movlw   0x0c                    ;display on, cursor off
        call    liter_c
        movlw   0x06                    ;increment mode
        call    liter_c
        movlw   0x01                    ;clear display
        call    liter_c
 	call	delay_15mS
        return
;------------------------------------------------------------
;	0x80 already added
;
first_line_display
        movlw   LINE_1
        call    liter_c                 ;
        return
;
second_line_display
        movlw   LINE_2
        call    liter_c                 ;
        return
;
;------------------------------------------------------------
;	Line_clear does whole line 
;	Line_stuff clears W number of chars
;
LCD_line_clear
	movlw	.20			;countdown from 20
LCD_line_stuff	
	movwf	lcd_count		;countdown from W 
line_clear_loop	
	movlw	" "
	movwf	lcd_data
	call	liter_d
	decfsz	lcd_count,F
	goto	line_clear_loop
	return
;
;------------------------------------------------------------
;
insert_space
	movlw	" "			;space to W
	movwf	lcd_data
	call	liter_d			;W to LCD
	return
;	       
;------------------------------------------------------------
;		sends control byte to LCD
liter_c					
        movwf   lcd_data                ;store w in lcd_data
	bcf	RS_LINE			;control
        call    delay_125uS
        call    flipbit                 ;hi nibble to PORTB
	call	pulse
        swapf   lcd_data,F              ;swap MS and LS nybbles
        call    flipbit                 ;output what was LS nybble
	call	pulse
	return
;------------------------------------------------------------
;		sends lcd_data to LCD
liter_d					
;       movwf   lcd_data                ;store w in lcd_data
	bsf	RS_LINE			;data
        call    delay_125uS
        call    flipbit                 ;hi nibble to PORTB
	call	pulse
        swapf   lcd_data,F              ;swap MS and LS nybbles
        call    flipbit                 ;output what was LS nybble
	call	pulse
	return
;------------------------------------------------------------
;		clears upper nibble of PORTB then writes
;		upper nibble of lcd_data to PORTB
flipbit
	movlw	0x0f
	andwf	PORTB,F
	movfw	lcd_data
	andlw	0xf0
	iorwf 	PORTB,F
	return
;------------------------------------------------------------
pulse
        bsf     E_LINE                  ;pulse E line
        nop                             ;delay
        bcf     E_LINE
        call    delay_125uS             ;delay 125 uS
        return
;------------------------------------------------------------
clear_LCD_display
        movlw   0x01                    ;clear display
        call    liter_c                 ;send to display
        call    delay_15mS              ;allow 15 mS to clear
        return
;------------------------------------------------------------
;	Sends serial data w/USART module
;
do_serial_send
        btfss   PIR1,TXIF		;wait for clear register
        goto    do_serial_send
	movfw	send_reg
	movwf	TXREG			;send it
	nop
	return
;
;------------------------------------------------------------
;
delay_60uS
	movlw	.1
	movwf	delay_count
	movlw	US_60
	movwf	TMR2
	goto	T2_delay_loop
;
delay_125uS
	movlw	.1
	movwf	delay_count
	movlw	US_125
	movwf	TMR2
	goto	T2_delay_loop
;
delay_500uS
	movlw	.2
	movwf	delay_count
	clrf	TMR2
	goto	T2_delay_loop
;
delay_5mS
	movlw	MS_5
	movwf	delay_count
	clrf	TMR2
	goto	T2_delay_loop	
;
delay_15mS
	movlw	MS_15
	movwf	delay_count
	clrf	TMR2
	goto	T2_delay_loop
;
delay_60mS
	movlw	MS_60	
	movwf	delay_count
	clrf	TMR2
;
T2_delay_loop
	bcf	PIR1,TMR2IF		;256 uS per rollover
T22	
	btfss	PIR1,TMR2IF
	goto	T22
	decfsz	delay_count,F
	goto	T2_delay_loop
	return
;	
;------------------------------------------------------------
	end
;------------------------------------------------------------
;
;Copyright (c) 2010, Mike Berg aka N0QBH
;All rights reserved.
;
;Redistribution and use in source and binary forms, with or without modification, 
;are permitted provided that the following conditions are met:
;
;Redistributions of source code must retain the above copyright notice, 
;this list of conditions and the following disclaimer. 
;Redistributions in binary form must reproduce the above copyright notice, 
;this list of conditions and the following disclaimer in the documentation 
;and/or other materials provided with the distribution. 
;Neither the name of the <ringolake.com> nor the names of its contributors 
;may be used to endorse or promote products derived from this software without 
;specific prior written permission. 
;THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
;AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
;THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR 
;PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS 
;BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
;CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
;SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
;INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
;CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
;ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
;THE POSSIBILITY OF SUCH DAMAGE.
