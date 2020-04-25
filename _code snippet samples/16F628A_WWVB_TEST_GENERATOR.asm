;*******************************************************************************
;   PIC 16F628 WWVB Test Gen
;   by Charles M Douvier
;	http://www.iradan.com
;	it'd be cool if you ripped to at least give me props for 
;	helping give you a head start
;
;	The idea is to module a 60Khz sine way with on-off control of a CD4066.
;
;   ver 0.01    hack version first try
;   ver 0.02    blown up 12F629 on port
;   ver 0.03    port to 16F628A to use PICKit2
;   ver 0.04    added extra fine tuning delay
;
;	its probably a violation of FCC rules to use with without running it into 
;	a dummy load or whatever... I'll leave it to you to see if you're breaking
;	any laws using this.
;*******************************************************************************
;                                                                              *
;Device 16F628A
;
; PIN DIAGRAM
;
;   RA0 O   ICSPDAT		RB0	Output to the switch control of a CD4066  
;   RA1 I   ICSPCLK
;   RA2 I   DAC/FUTURE
;
;   RA4 O               RB4 I
;   RA5 O               RB5 O    x
;                       RB6 I
;                       RB7 I
;                                                                              *
;                                                                              *
;*******************************************************************************



;*******************************************************************************
; Processor Inclusion
;*******************************************************************************

  list      p=16F628A           ; list directive to define processor 
  #include <P16F628A.inc>       ; processor specific variable definitions

  errorlevel  -302              ; suppress message 302 from list file 

  __CONFIG   _CP_OFF & _LVP_OFF & _BOREN_OFF & _MCLRE_ON & _WDT_OFF & _PWRTE_ON & _INTOSC_OSC_NOCLKOUT 

;*******************************************************************************
;   Configuration
;*******************************************************************************

;    __CONFIG _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _PWRTE_OFF & _MCLRE_ON & _CP_OFF & _BOREN_OFF & _CLKOUTEN_OFF & _IESO_OFF & _FCMEN_OFF
;    __CONFIG _CONFIG2, _WRT_OFF & _STVREN_OFF & _LVP_OFF

;*******************************************************************************

;
;*******************************************************************************

;
GPR_VAR        UDATA
int_delay1      RES 1
cnt1            RES 1



;*******************************************************************************
; Reset Vector
;*******************************************************************************

;RES_VECT  CODE    0x0000            ; processor reset vector
;    GOTO    START                   ; go to beginning of program

RESET_VECTOR    CODE   0x0000     ; processor reset vector
        goto    START             ; go to beginning of program


;*******************************************************************************
; MAIN PROGRAM
;*******************************************************************************

;MAIN_PROG CODE                      ; let linker place main program

;    banksel         OSCCON
;    movlw           b'01111000'         ; INTOSC 16MHz
;    movwf           OSCCON ;
;    banksel         OSCSTAT ;
;    btfss           OSCSTAT, HFIOFR     ; Running?
;    goto            $-1
;    btfss           OSCSTAT, HFIOFS     ; Stable?
;    goto            $-1
;*******************************************************************************



delay2
           BANKSEL int_delay1
            MOVLW   0x5F
            MOVWF   int_delay1
OLOOP
            BANKSEL TMR0
            CLRF TMR0
            BANKSEL INTCON
            BCF INTCON,T0IF
            MOVLW 0xC0              ; PortB pull-ups are disabled, (c3=1:16)
            MOVWF OPTION_REG        ; Interrupt on rising edge of RB0
            NOP                        ; Timer0 increment from internal clock
            NOP                        ; with a prescaler of 1:4.
            NOP
            NOP                        ; The TMR0 interrupt is disabled, do polling on the overflow bit
            NOP
                                    ;FINE TUNING
            movlw   0xFF
            movwf   cnt1
ILOOP
            DECFSZ  cnt1,F
            GOTO    ILOOP
            movlw   0x58
            movwf   cnt1
ILOOP2
            DECFSZ  cnt1,F
            GOTO    ILOOP2
            NOP
            NOP
            NOP
            NOP
T0_OVFL_WAIT2
            BTFSS INTCON, T0IF
            GOTO T0_OVFL_WAIT2
            BANKSEL int_delay1
            DECFSZ int_delay1,1
            GOTO OLOOP
            NOP
            ;... else

        RETURN

MARKER
    BANKSEL PORTB
    BCF     PORTB,0
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2

    CALL    delay2
    CALL    delay2
    CALL    delay2              ;800ms off
    BANKSEL PORTB
    BSF     PORTB,0
    CALL    delay2
    CALL    delay2              ;200ms on
    RETURN

ONE
    BANKSEL PORTB
    BCF     PORTB,0
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2              ;500ms off
    BANKSEL PORTB
    BSF     PORTB,0
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2              ;500ms on
    RETURN

ZERO
    BANKSEL PORTB
    BCF     PORTB,0
    CALL    delay2
    CALL    delay2              ;200ms off
    BANKSEL PORTB
    BSF     PORTB,0
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2
    CALL    delay2              ;800ms on
    RETURN

TIMING_PULSE
    BANKSEL PORTB
    BSF     PORTB,0
    CALL    delay2
    BANKSEL PORTB
    BCF     PORTB,0
    CALL    delay2
;    CALL    delay2
;    CALL    delay2
;    CALL    delay2

;    CALL    delay2
;    CALL    delay2
;    CALL    delay2
;    CALL    delay2
;    CALL    delay2

    RETURN

START

    BANKSEL PORTA
    CLRF    PORTA
    BANKSEL TRISA           ;All Outputs
    CLRF    TRISA

    BANKSEL PORTB
    CLRF    PORTB
    BANKSEL TRISB
    CLRF    TRISB           ;All Outputs


    movlw b'11000011'       ;configure TMR0
    BANKSEL OPTION_REG
    movwf OPTION_REG


LOOP
    ;
    BANKSEL PORTB
;    CALL    TIMING_PULSE
;    GOTO    LOOP

;    BCF 
;    BCF     PORTB,0
;0
    CALL    MARKER                      ;MARKER FRAME REFERENCE BIT
    CALL    ONE                         ;40min
    CALL    ZERO                        ;20min
    CALL    ZERO                        ;10min
    CALL    ZERO                        ;Reserved
    CALL    ZERO                        ;8mins
    CALL    ZERO                        ;4mins
    CALL    ONE                         ;2mins
    CALL    ZERO                        ;1mins
    CALL    MARKER                      ;MARKER 1
;10
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;20hours
    CALL    ZERO                        ;10hours
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;8hours
    CALL    ONE                         ;4hours
    CALL    ONE                         ;2hours
    CALL    ZERO                        ;1hour
    CALL    MARKER                      ;MARKER 2
;20
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;200 day of year
    CALL    ONE                         ;100 day of year
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;80 day of year
    CALL    ONE                         ;40 day of year
    CALL    ZERO                        ;20 day of year
    CALL    ONE                         ;10 day of year
    CALL    MARKER                      ;MARKER 3
;30
    CALL    ONE                         ;8 day of year
    CALL    ZERO                        ;4 day of year
    CALL    ZERO                        ;2 day of year
    CALL    ZERO                        ;1 day of year
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;UTI Sign +
    CALL    ZERO                        ;UTI Sign -
    CALL    ZERO                        ;UTI Sign +
    CALL    MARKER                      ;MARKER 4
;40
    CALL    ZERO                        ;UTI Corr 0.8s
    CALL    ZERO                        ;UTI Corr 0.4s
    CALL    ZERO                        ;UTI Corr 0.2s
    CALL    ZERO                        ;UTI Corr 0.1s
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;80 year
    CALL    ZERO                        ;40 year
    CALL    ZERO                        ;20 year
    CALL    ONE                         ;10 year
    CALL    MARKER                      ;MARKER 5
;50
    CALL    ZERO                        ;8 year
    CALL    ONE                         ;4 year
    CALL    ONE                         ;2 year
    CALL    ZERO                        ;1 year
    CALL    ZERO                        ;RESERVED
    CALL    ZERO                        ;LEAP YEAR TRUE
    CALL    ZERO                        ;LEAP SEC WARN
    CALL    ONE                        ;DST
    CALL    ONE                        ;DST
    CALL    MARKER                        ;FRAME BIT P0

    GOTO LOOP                           ;loop forever
    END