;                                                                              *
;*******************************************************************************
;                                                                              *
;    Filename:      main.asm                                                   *
;    Date:          Sept 29 2013                                               *
;    File Version:  1.0                                                        *
;    Author:        Charles Douvier                                            *
;    Company:                                                                  *
;    Description:   Test of PWM1                                               *
;
;    Device 16F1509
;
;
;
;
; PIN DIAGRAM
;
;   RA0                             RC0
;   RA1                             RC1
;   RA2                             RC2     STATUS LED
;                                   RC3
;   RA4             RB4             RC4
;   RA5             RB5             RC5     PWM1
;                   RB6             RC6
;                   RB7             RC7
;
;------------------------------------------------------------           *
;*******************************************************************************
;                                                                              *
;    Notes: In the MPLAB X Help, refer to the MPASM Assembler documentation    *
;    for information on assembly instructions.                                 *
;                                                                              *
;*******************************************************************************
;                                                                              *
;    Known Issues: This template is designed for relocatable code.  As such,   *
;    build errors such as "Directive only allowed when generating an object    *
;    file" will result when the 'Build in Absolute Mode' checkbox is selected  *
;    in the project properties.  Designing code in absolute mode is            *
;    antiquated - use relocatable mode.                                        *
;                                                                              *
;*******************************************************************************
;                                                                              *
;    Revision History:
;           2013-09-28  Initial                                                *
;                                                                              *
;*******************************************************************************

 

;*******************************************************************************
; Processor Inclusion
;
;*******************************************************************************

    errorlevel -230, -302, -303, -313
    LIST R=DEC

#include "p16f1509.inc"

;*******************************************************************************
;
; Word Setup
;
;
;*******************************************************************************
    __CONFIG _CONFIG1, _FOSC_INTOSC & _WDTE_OFF & _PWRTE_OFF & _CLKOUTEN_OFF
    __CONFIG _CONFIG2, _LVP_OFF & _STVREN_ON

;*******************************************************************************
;
;  Variable Definitions
;
;*******************************************************************************

; TODO PLACE VARIABLE DEFINITIONS GO HERE

;*******************************************************************************
; Reset Vector
;*******************************************************************************

RES_VECT  CODE    0x0000            ; processor reset vector
    GOTO    START                   ; go to beginning of program

;*******************************************************************************

; TODO INSERT ISR HERE

;*******************************************************************************
; MAIN PROGRAM
;*******************************************************************************

MAIN_PROG CODE                      ; let linker place main program

INIT:
                            ;RC5 = PWM1
    BANKSEL LATA            ;Data Latch
    CLRF LATA               ;
    BANKSEL ANSELC          ;
    CLRF ANSELC             ;Digital IO
    BANKSEL PORTC           ;
    BCF PORTC,5             ;Clear PWM1
    BANKSEL TRISC           ;Set all PORTC to outputs
    CLRF TRISC
    BANKSEL PORTC
    BSF PORTC,2

    BANKSEL OSCCON
    MOVLW   0x78            ;16MHZ Clock
    MOVWF   OSCCON

    BANKSEL PWM1CON         ;
    CLRF PWM1CON            ;Disable PWM bits

    BANKSEL PR2
    MOVLW 0xFF
    MOVWF PR2               ;Load PR2 with 0xFF
                            ;Timer/PR set up is 973Hz
    BANKSEL PWM1DCH
    CLRF PWM1DCH
    BANKSEL PWM1DCL
    CLRF PWM1DCL

    BSF PORTC,2

    BANKSEL PWM1DCH
    MOVLW   0x6F
    MOVWF   PWM1DCH

                            ;copied code, havent check this yet.
    ;ENABLE INTERRUPT
    BANKSEL PIE1
    BSF PIE1,1

    bcf        PIR1,2
                    ;-----------------------------------
                    ;Configure and start Timer2
;CONFIGURE TIMER.. copied code review.
    BANKSEL T2CON
    MOVLW B'00000110'   ;MOVLW B'00000110'
    MOVWF T2CON
                    ;Enable PWM output pin and wait until Timer2
                    ;overflows, TMR2IF bit of the PIR1 register is set.
                    ;See note below.

    btfss    PIR1,1                ;Test for T2 interrupt
    goto    $-1

                    ;Enable the PWMx pin output driver(s) by clearing
                    ;the associated TRIS bit(s) and setting the
                    ;PWMxOE bit of the PWMxCON register.

                    ;Configure the PWM module by loading the
                    ;PWMxCON register with the appropriate values.
    BANKSEL TRISC          ;Enable Outputs
    CLRF    TRISC

    BANKSEL PWM1CON        ;Enable PWM1
    MOVLW    B'11000000'
    MOVWF    PWM1CON
    RETURN
START

    CALL INIT
    BANKSEL PORTC            ;select PORTC for heatbeat

LOOP:
    BSF PORTC,2              ;heart beat
    MOVLW 0x55
    NOP                      ;future code
    BCF PORTC,2
    GOTO LOOP                ;loop forever

    END