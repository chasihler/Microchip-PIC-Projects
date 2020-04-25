;
;                      Software License Agreement
;
; The software supplied herewith by Microchip Technology Incorporated 
; (the "Company") for its PICmicro® Microcontroller is intended and 
; supplied to you, the Company’s customer, for use solely and 
; exclusively on Microchip PICmicro Microcontroller products. The 
; software is owned by the Company and/or its supplier, and is 
; protected under applicable copyright laws. All rights are reserved. 
;  Any use in violation of the foregoing restrictions may subject the 
; user to criminal sanctions under applicable laws, as well as to 
; civil liability for the breach of the terms and conditions of this 
; license.
;
; THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES, 
; WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED 
; TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
; PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT, 
; IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR 
; CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
;
;
;**********************************************************************
;  Manchester encoding interrupt driven receiver routines
; 
;   Uses: interrupt on TMR0
;   Accepts: Keeloq code words Manchester encoding
;
;   VERSION     1.00    1/16/01     Lucio Di Jasio
;
;**********************************************************************

#define XTAL	    .20     ; clock frequency  (MHz)
#define BRATE       .200    ; Te = 200 (us) 
#define RF_OVERS    .8      ; oversampling factor (4..8)

TIMER_VAL	        equ (XTAL * BRATE) / (4 * RF_OVERS)

    IF (TIMER_VAL > .256) || (TIMER_VAL < .60)
        error "ERROR: Timer period exceeds limits"
    ENDIF
    
; bit timing limits
#define SHORT_HEAD  .3*RF_OVERS     ;  minimum synch header length accepted
#define LONG_HEAD   .6*RF_OVERS     ;  maximum synch header length accepted
#define HIGH_TO     .3*RF_OVERS     ;  bit timeout

#define RF_NBITS	.69     ; number of bits to capture

    CBLOCK  0x20        ; omit the address (0x20) if this include module is
                        ;  not the first to allocate RAM
        W_TEMP          ; context saving W reg.
        STATUS_TEMP     ;   "        "   STATUS reg.
        PCLATH_TEMP     ;   "        "   PCLATH reg.
        FSR_TEMP        ;   "        "   FSR reg.
        
        RFP             ; receive buffer pointer
        RFbitc          ; bit counter
        RFSkip          ; skip sample counter
        RFsamp          ; samples counter
        RFState         ; state variable
        RXFlags         ; various flags
        Te              ; time unit = 1/baud rate

        Buffer0:9       ; receive data buffer
        
; add here extra software timers
        XTMRH           ; example 16 bit software timer
        XTMRL

    ENDC

; radio input
#define RFIN    PORTB,0

; flags
#define RF_Full   RXFlags,0 ; buffer full, receiver idle
#define LastBit   RXFlags,1 ; used for transition detection

;----------------------------------------------------------------------
; Async_ISR
;
; NOTE: place this routine at loc. 0x04
;       context saving can be specific to the PICmicro model used
;       (ref. to PICmicro datasheet for suggested implementation)
;
IntVector
            movwf   W_TEMP          ; context saving, save W first
            swapf   STATUS,W
            bcf     STATUS,RP0      ; assuming use of variables in bank0
            movwf   STATUS_TEMP     ; save status
            movf    PCLATH,W
            movwf   PCLATH_TEMP     ; save PCLATH
            clrf	PCLATH			; assuming Async_ISR located in page0

;----------------------------------------------------------------------            
Async_ISR
            movlw   TIMER_VAL       ; non disruptive timer reload
            subwf   TMR0,F

            bcf     INTCON,T0IF     ; interrupt served

            btfsc   RF_Full         ; if buffer still full
            goto    AsyncRFE        ; idle (exit immediate)

            decfsz  RFSkip,F        ; count the skips
            goto    AsyncRFE        ; delay (exit immediate)
            
            movf    RFState,W       ; launch appropriate state routine
            andlw   03              ; limit to [0..3]
            addwf   PCL,F           ; offset in jump Table
RFTable
            goto    TRFReset        ; 0
            goto    TRFSync         ; 1
            goto    TRFTDetect      ; 2
            goto    TRFSample       ; 3
RFTableEnd

    IF HIGH(RFTable) != HIGH(RFTableEnd) 
        error "RFTable crosses page border"
    ENDIF

;----------------------------------------------------------------------
; State 1
;       Waiting and measuring a Synch Header
;
TRFSync
            btfsc   RFIN            ; wait for a rising edge
            goto    TRFRise
            incf    RFsamp,F        ; while input low count the samples
            movf    RFsamp,W
            btfsc   STATUS,Z
            goto    TRFReset        ; check overflows
            incf    RFSkip,F        ; skip = 1
            goto    AsyncRFE        ; remain in state 0

TRFRise
            movlw   SHORT_HEAD      ; check minimum header length
            subwf   RFsamp,W
            btfss   STATUS,C
            goto    TRFReset        ; procede to reset if too short

            movlw   LONG_HEAD       ; check maximum header length
            subwf   RFsamp,W
            btfsc   STATUS,C
            goto    TRFReset        ; procede to reset if too long

TRCalibra
            bcf     STATUS,C        ; RFSamp measure THeader
            rrf     RFsamp,F        ; divide by two
            bcf     STATUS,C
            rrf     RFsamp,W        ; divide by four
            movwf   Te              ; Te = 1/4 THeader
            movwf   RFsamp
            bcf     STATUS,C
            rrf     RFsamp,W        ; RFsamp = 1/8 THeader
            addwf   Te,F            ; Te = 1/4THeader + 1/8THeader (that is 1.5xTE)
            incf    Te,F            ; round it up

TRinit      
            bsf     LastBit         ; init for detection of falling edges 
            clrf    RFsamp          ; reset sample counter
            incf    RFState,F       ; move on to transition detection(2)
            incf    RFSkip,F        ; skip=1
            goto    AsyncRFE        ; done

;----------------------------------------------------------------------
; State 2
;
; Transition Detection
;
TRFTDetect
            btfsc   LastBit         ; depending on last value of input
            goto    TRFUSET

; last bit = 0
            btfsc   RFIN            ; detect a transition
            goto    TRFTransition
            goto    TRFNOTransition

; last bit = 1
TRFUSET
            btfss   RFIN            ; detect a transition
            goto    TRFTransition
            goto    TRFNOTransition

TRFTransition                       ; transition detected
            incf    RFState,F       ; move on to Sampling (State 3)
            clrf    RFsamp
            movf    Te,W            ; skip = 1.5xTE
            movwf   RFSkip
            goto    AsyncRFE        ; done
            
TRFNOTransition                     ; no transition detected
            incf    RFSkip,F        ; skip =1
            incf    RFsamp,F        ; keep counting time between transitions
            movlw   HIGH_TO         ; check against Timeout value
            subwf   RFsamp,W
            btfss   STATUS,C        ; if timeout fall through Reset                     
            goto    AsyncRFE        ; done 

;----------------------------------------------------------------------
; State 0,
;
;  Reset receiver
;
TRFReset
            movlw   1
            movwf   RFState         ; move on to TRFSync (1)
            clrf    RFsamp          ; reset sample counter
            movlw   Buffer0         ; reset buffer pointer 
            movwf   RFP
            clrf    RFbitc          ; reset bit counter
            incf    RFSkip,F        ; skip =1 
            goto    AsyncRFE        ; done

;----------------------------------------------------------------------
; State 3,
;
;  Data Sampling
; 
TRFSample
            incf    RFSkip,F        ; skip = 1
            decf    RFState,F       ; next will be TDetect again (2)

            bcf     LastBit         ; update LastBit for next transition 
            btfsc   RFIN
            bsf     LastBit

            movf    FSR,W           ; save FSR
            movwf   FSR_TEMP
            movf    RFP,W           ; load current pointer
            movwf   FSR             ; 

            bcf     STATUS,C
            btfsc   RFIN            ; copy data bit in CARRY
            bsf     STATUS,C
            rrf     INDF,F          ; roll in the buffer 

            movf    FSR_TEMP,W      ; restore FSR
            movwf   FSR
            incf    RFbitc,F        ; count the bit

            movlw   07              ; every 8 bit 
            andwf   RFbitc,W
            btfsc   STATUS,Z
            incf    RFP,F           ; increment the pointer
            
            movf    RFbitc,W        ; check all bit in already
            xorlw   RF_NBITS
            btfss   STATUS,Z
            goto    AsyncRFE        ; not yet, done

TRFFull                             ; received them all
            movlw   TRFReset        ; next state will be Reset
            movwf   RFState         
            bsf     RF_Full         ; set the buffer full flag 
            goto    AsyncRFE        ; done
AsyncRFE  
  
;----------------------------------------------------------------------
; Example: how to use the interrupt period to derive more software timers
;          showing a 16 bit TIMER that ticks at every interrupt period
; 1 tick (us) = 4 x TIMER_VAL / XTAL (MHz) 
;
            incf    XTMRH,F         ; update 16 bit timer
            incfsz  XTMRL,F
            decf    XTMRH,F

;----------------------------------------------------------------------
; Context Restore
; 
ExitInt     
            movf    PCLATH_TEMP,W   ; restore PCLATH (page)
            movwf   PCLATH          
            swapf   STATUS_TEMP,W   ; restore status and bank
            movwf   STATUS
            swapf   W_TEMP,F        ; restore W reg.
            swapf   W_TEMP,W
            retfie                  ; exit re-enabling interrupts

;----------------------------------------------------------------------
; Init Receiver
;
; Example code for proper interrupt and timer set up
;
InitRX
            clrf    RXFlags
            clrf    INTCON          ; disable interrupts
            clrf    RFState         ; init with Reset state
			clrf    XTMRH			; init demo software timer
            clrf    XTMRL
            movlw   1
            movwf   RFSkip          ; init skip
 
            bsf     STATUS,RP0      ; bank 1
            movlw   b'10001111'     ; init timer prescaler 1:1
            movwf   OPTION_REG      ; prescaler assigned to WDT 1:128
            bcf     STATUS,RP1      ; bank 0

            movlw   -TIMER_VAL      ; init timer
            movwf   TMR0

            bsf     INTCON,T0IE     ; enable interrupts on TMR0 overflow
            bsf     INTCON,GIE      ; global interrupt enable
            return


