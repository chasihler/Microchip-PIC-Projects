;*******************************************************************************
;   Rotary Encoder Test, 2 bit                                                                              *
;   http://www.iradan.com
;
;   RA0:    OUT     TEST LED
;   RA1:    IN      ENC_B INPUT, NOTE: DISCONNECT OR INSURE LOW WHEN PROGRAMMING
;   RA2:    IN      ENC_A INPUT
;   RB5:    OUT     SWITCH STATUS LED
;   RB7:    IN      SWITCH INPUT
;   RC<0:7> OUT     ENCODER "COUNT"
;
;   VERSION 0.1     INITIAL CODE
;
;*******************************************************************************

    errorlevel -230, -302, -303, -313
    LIST R=DEC

#include "p16f1509.inc"

    __CONFIG _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _PWRTE_ON & _CLKOUTEN_OFF
    __CONFIG _CONFIG2, _LVP_OFF & _STVREN_ON

            UDATA_SHR
COUNT       RES .1
ENC_LAST    RES .1
ENC_CURRENT RES .1

#DEFINE ENCA    PORTA,1
#DEFINE ENCB    PORTA,2
#DEFINE ENC_SA  ENC_LAST,0
#DEFINE ENC_SB  ENC_LAST,1
#DEFINE ENCA_NOW    ENC_CURRENT,0
#DEFINE ENCB_NOW    ENC_CURRENT,1

;*******************************************************************************
; Reset Vector
;*******************************************************************************

RES_VECT  CODE    0x0000            ; processor reset vector
    GOTO    START                   ; go to beginning of program

;*******************************************************************************
; MAIN PROGRAM
;*******************************************************************************

MAIN_PROG CODE                      ; let linker place main program

START
    CALL    INIT

    BANKSEL PORTA
    BSF     PORTA,0         ;THIS WAS FOR TESTING.

    GOTO    LOOP

INIT
    CLRF    COUNT

    BANKSEL PORTC
    CLRF    PORTC
    BANKSEL LATC            ;Data Latch
    CLRF LATC               ;
    BANKSEL ANSELC          ;
    CLRF ANSELC             ;Digital IO
    BANKSEL TRISC           ;
    MOVLW   B'00000000'     ;RC<0:7> OUT
    MOVWF   TRISC

    BANKSEL PORTA           ;
    CLRF PORTA              ;Init PORTA
    BANKSEL LATA            ;Data Latch
    CLRF LATA               ;
    BANKSEL ANSELA          ;
    CLRF ANSELA             ;digital I/O
    BANKSEL TRISA           ;
    MOVLW B'00111110'       ;Set RA<0,6:7>out RA<1:5> in
    MOVWF TRISA             ;

    BANKSEL TRISB
    MOVLW   B'11011111'     ;RB<5> OUT, RB<0:4>,<6:7> IN
    MOVWF   TRISB
    CLRF    PORTB

   BANKSEL OSCCON           ;SET OSCILLATOR SPEED
    MOVLW   0x78            ;01111000  / 16MHz
    MOVWF   OSCCON

    CLRF    ENC_CURRENT     ;HOUSE KEEPING
    CLRF    ENC_LAST

    RETURN

LED
    BANKSEL PORTB           ;THIS TURNS ON SWITCH STATUS LED
    BSF PORTB,5
    RETURN

INCR
    INCF    COUNT,f
;   GOTO    RESUME
    RETURN

DECR
    DECF    COUNT,f
;    GOTO    RESUME
    RETURN

DETERMINE_DIRA
    BTFSC   ENCB_NOW        ;OKAY WHICH WAY DID IT TURN?
    CALL    INCR            ;INCREMENT
    BTFSS   ENCB_NOW
    CALL    DECR            ;DECREMENT
    RETURN

TESTLASTA
    BTFSS   ENC_SA          ;SO ENCA WAS HIGH, WAS IT LAST TIME?
    CALL    DETERMINE_DIRA  ;YEP..
    NOP                     ;GUESS NOT, RETURNING
    RETURN

TEST_ENC                    ;I'M ONLY TESTING FOR A HIGH ON ENC A.
    BTFSC   ENCA_NOW        ;THAT MEANS EVERY OTHER TICK ON THE ENCODER
    CALL    TESTLASTA       ;DOES NOTHING
    RETURN

LOOP
    NOP

    BANKSEL PORTB
    BCF PORTB,5             ;TURN OFF SWITCH STATUS LED
    BANKSEL PORTA
    BTFSC   PORTA,5         ;TEST IF SWITCH IS PUSHED
    CALL    LED             ;BRANCH IF ON, SKIP IF NOT
    NOP

    BCF ENCA_NOW            ;CLEAR CONTENTS OF ENCODER "NOW BITS"
    BCF ENCB_NOW

    BANKSEL PORTA
    BTFSC   ENCA
    BSF     ENCA_NOW        ;ENC_A INPUT IS HIGH
    NOP
    BTFSC   ENCB
    BSF     ENCB_NOW        ;ENC_V INPUT IS HIGH

    CALL    TEST_ENC        ;TEST ENC_A

    BANKSEL PORTC           ;DUMP ENCODER COUNT ONTO PORTC
    MOVFW   COUNT
    MOVWF   PORTC

    BCF     ENC_SA          ;SAVE STATUS OF ENCODER A & B INPUTS
    BTFSC   ENCA_NOW
    BSF     ENC_SA

    BCF     ENC_SB
    BTFSC   ENCB_NOW
    BSF     ENC_SB

    BANKSEL PORTA           ;USED FOR TESTING
    BCF     PORTA,0

    GOTO LOOP               ;LOOP FOREVER

    END