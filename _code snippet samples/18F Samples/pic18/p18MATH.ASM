;************************************************************************
;*	Microchip Technology Inc. 2002					*
;*	Assembler version: 2.0000					*
;*	Filename: 							*
;*		p18math.asm (main routine)   				*
;*	March 14,2002							*
;*      Designed to run at 4MHz                                         *
;* 	PICDEM 2 PLUS DEMO code                                   	*
;************************************************************************

	list	p=18f452
	#include	p18f452.inc

	#define	_C	STATUS,0

MATH_VAR	UDATA
AARGB0		RES 1
AARGB1		RES 1
AARGB5		RES 1
BARGB0		RES 1
BARGB1		RES 1
REMB0		RES 1
REMB1		RES 1
TEMP		RES 1
LOOPCOUNT	RES 1

	GLOBAL	AARGB0, AARGB1, BARGB0

PROG2	CODE
;---------------- 8 * 8 UNSIGNED MULTIPLY -----------------------

;       Max Timing:     3+12+6*8+7 = 70 clks
;       Min Timing:     3+7*6+5+3 = 53 clks
;       PM: 19            DM: 4
UMUL0808L
		CLRF    AARGB1
                MOVLW   0x08
                MOVWF   LOOPCOUNT
                MOVF    AARGB0,W

LOOPUM0808A
                RRCF     BARGB0, F
                BTFSC   _C
                bra    LUM0808NAP
                DECFSZ  LOOPCOUNT, F
                bra    LOOPUM0808A

                CLRF    AARGB0
                RETLW   0x00

LUM0808NAP
                BCF     _C
                bra    LUM0808NA

LOOPUM0808
                RRCF             BARGB0, F
                BTFSC   _C
                ADDWF   AARGB0, F
LUM0808NA       RRCF    AARGB0, F
                RRCF    AARGB1, F
                DECFSZ          LOOPCOUNT, F
                bra            LOOPUM0808
		return
		GLOBAL	UMUL0808L
;----------------  16/8 UNSIGNED DIVIDE	  ------------------------
              
;       Max Timing: 2+7*12+11+3+7*24+23 = 291 clks
;       Min Timing: 2+7*11+10+3+7*17+16 = 227 clks
;       PM: 39                                  DM: 7

UDIV1608L
		GLOBAL		UDIV1608L
		CLRF            REMB0
                MOVLW           8
                MOVWF           LOOPCOUNT

LOOPU1608A      RLCF             AARGB0,W
                RLCF             REMB0, F
                MOVF            BARGB0,W
                SUBWF           REMB0, F

                BTFSC           _C
                bra            UOK68A          
                ADDWF           REMB0, F
                BCF             _C
UOK68A          RLCF             AARGB0, F

                DECFSZ          LOOPCOUNT, F
                bra            LOOPU1608A

                CLRF            TEMP

                MOVLW           8
                MOVWF           LOOPCOUNT

LOOPU1608B      RLCF             AARGB1,W
                RLCF             REMB0, F
                RLCF             TEMP, F
                MOVF            BARGB0,W
                SUBWF           REMB0, F
                CLRF            AARGB5
                CLRW
                BTFSS           _C
                INCFSZ          AARGB5,W
                SUBWF           TEMP, F

                BTFSC           _C
                bra            UOK68B          
                MOVF            BARGB0,W
                ADDWF           REMB0, F
                CLRF            AARGB5
                CLRW
                BTFSC           _C
                INCFSZ          AARGB5,W
                ADDWF           TEMP, F

                BCF             _C
UOK68B          RLCF             AARGB1, F

                DECFSZ          LOOPCOUNT, F
                bra            LOOPU1608B
		return
		GLOBAL	UDIV1608L

		end
