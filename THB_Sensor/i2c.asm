;******************************************************************
;*								  *
;*	Filename: i2c.inc					  *
;*								  *
;*	I2C routines						  *
;*								  *
;*	J-Paul ROUBELAT - F6FBB - 5 March 2010			  *
;*								  *
;******************************************************************

#include <thb_sensor.inc>

	GLOBAL	_Read_BMP,_Write_BMP,_Seq_Read_BMP
	GLOBAL	I2C_Addr,I2C_Len

;******************************************************************************

	UDATA

I2C_OutByte	res	1	; i2c Byte storage
I2C_Count	res	1	; i2c bit counter
I2C_Flags	res	1	; flags for i2c transferts
I2C_InByte	res	1	; i2c input byte
I2C_Save	res	1	; i2c temporary storage
I2C_Addr	res	1	; i2c address
I2C_Len		res	1	; length of the data

;******************************************************************************

PGM	CODE

;******************************************************************************
; _Write_BMP
; write W register to address I2C_Adr
;
_Write_BMP
		movwf	I2C_Save		; save W

		call	_I2C_Start

		call	_I2C_Set_Write
		btfsc	I2C_Flags,0
		goto	_Error_Routine

		call	_I2C_Addr
		btfsc	I2C_Flags,0
		goto	_Error_Routine

        	movfw	I2C_Save		; send the actual data
        	call	_I2C_Out
        	call	_I2C_Nak
		btfsc	I2C_Flags,0
		goto	_Error_Routine

        	call	_I2C_Stop

        	return

;******************************************************************************
; _Read_BMP
; reads data at location specified in i2c_Addr
; returns result in W
;
_Read_BMP
		call	_I2C_Start

		call	_I2C_Set_Write
		btfsc	I2C_Flags,0
		goto	_Error_Routine

		call	_I2C_Addr
		btfsc	I2C_Flags,0
		goto	_Error_Routine

        	call 	_I2C_Start		; note there is no STOP

		call	_I2C_Set_Read
		btfsc	I2C_Flags,0
		goto	_Error_Routine

        	call 	_I2C_Read		; fetch the byte
        	call 	_I2C_Send_Nak		; send no acknowledgement

        	call 	_I2C_Stop

		movfw 	I2C_InByte		; return the byte in W
        	return

;******************************************************************************
; Seq_Read_BMP
; reads data at location specified in I2C_Adr
; length specified by I2C_Len
; returns result in buf pointed by FSR
;
_Seq_Read_BMP
		call 	_I2C_Start

		call	_I2C_Set_Write
		btfsc	I2C_Flags, 0
		goto	_Error_Routine

		call	_I2C_Addr
		btfsc	I2C_Flags, 0
		goto	_Error_Routine

        	call 	_I2C_Start		; note there is no STOP

		call	_I2C_Set_Read
		btfsc	I2C_Flags,0
		goto	_Error_Routine

_Seq_Read_1
		call 	_I2C_Read		; fetch each byte
		movfw	I2C_InByte
		movwf 	INDF			; and save in data buffer
		incf 	FSR,f
		decfsz 	I2C_Len,f
		goto 	_Seq_Read_2		; not done
		goto 	_Seq_Read_3

_Seq_Read_2
		call 	_I2C_Ack		; if not done, send an ACK and continue
		goto 	_Seq_Read_1
_Seq_Read_3
        	call 	_I2C_Send_Nak		; send no acknowledgement

        	call 	_I2C_Stop
		movf 	I2C_InByte,w		; return the byte in W
        	return

_Error_Routine
        	call	_I2C_Stop
		return

;******************************************************************************
; The following routines are low level I2C routines applicable to most
; interfaces with I2C devices.
_I2C_Read					; read byte on i2c bus
		clrf 	I2C_InByte
		movlw 	0x08
		movwf 	I2C_Count		; set index to 8	
		call	_HIGH_SDA		; be sure SDA is configured as input
_In_Bit
		call 	_HIGH_SCL		; clock high
		btfss 	G_SDA			; test SDA bit
		goto	_In_Zero
		goto	_In_One

_In_Zero
		bcf 	STATUS,C		; clear carry
		rlf 	I2C_InByte,F		; i_byte = i_byte << 1 | 0
		goto 	_Cont_In

_In_One
		bsf	STATUS,C		; set carry
		rlf 	I2C_InByte,F

_Cont_In
		call	_LOW_SCL		; bring clock low
		decfsz	I2C_Count,F		; decrement index
		goto	_In_Bit
		return

_I2C_Out					; send w register on I2C bus
        	movwf	I2C_OutByte
		movlw	0x08
		movwf	I2C_Count
_Out_Bit
		bcf	STATUS,C		; clear carry
		rlf	I2C_OutByte,F		; left shift, most sig bit is now in carry
		btfss 	STATUS,C		; if one, send a one
		goto	_Out_Zero
		goto	_Out_One

_Out_Zero
		call	_LOW_SDA		; SDA at zero
		call	_Clock_Pulse	
		call	_HIGH_SDA
		goto	_Out_Cont

_Out_One
		call 	_HIGH_SDA		; SDA at logic one
		call 	_Clock_Pulse
_Out_Cont
		decfsz 	I2C_Count,F		; decrement index
		goto	_Out_Bit
		return	

_I2C_Nak					; bring SDA high and clock
		call 	_HIGH_SDA
		call 	_HIGH_SCL

		clrf	I2C_Count		; wait for ACK
_WaitForACK	incf	I2C_Count,f		; increase timeout counter each time ACK is not received
		btfsc	STATUS,Z
		goto	_No_ACK_Rec
		btfsc	G_SDA			; test pin. If clear, EEPROM is pulling SDA low for ACK
		goto	_WaitForACK		; ...otherwise, continue to wait
		bcf	I2C_Flags,0		; clear flag bit (ACK received)
		call 	_LOW_SCL
		return

_I2C_Send_Nak					; bring SDA high and clock
		call 	_HIGH_SDA
		call 	_Clock_Pulse
		return

_WaitForWrite					; poll ACK for write timing
		call 	_I2C_Start
		call	_I2C_Set_Write
		btfsc	I2C_Flags, 0
		goto	_WaitForWrite
		return

;------ No ACK received from slave (must use "return" from here)
;; Typically, set a flag bit to indicate failed write and check for it upon return.
_No_ACK_Rec
		bsf	I2C_Flags, 0		; set flag bit
		return

_I2C_Ack:
		call 	_LOW_SDA
		call 	_Clock_Pulse
		return

_I2C_Start				
		call 	_LOW_SCL
		call	_HIGH_SDA
		call 	_HIGH_SCL
		call 	_LOW_SDA		; bring SDA low while SCL is high
		call 	_LOW_SCL
		return

_I2C_Stop:
		call 	_LOW_SCL
		call 	_LOW_SDA
		call 	_HIGH_SCL
		call 	_HIGH_SDA		; bring SDA high while SCL is high
		; call 	_LOW_SCL
		return

_I2C_Set_Write
        	movlw 	BMP_ADDR		; address of the chip
        	call 	_I2C_Out
        	call 	_I2C_Nak
		return

_I2C_Set_Read
        	movlw 	BMP_ADDR		; address of the chip
		iorlw	I2C_READ		; add 1 for read
        	call 	_I2C_Out
        	call 	_I2C_Nak
		return

_I2C_Addr
		movfw	I2C_Addr		; send address
        	call 	_I2C_Out
        	call 	_I2C_Nak
		return

_Clock_Pulse					; SCL momentarily to logic one
		call 	_HIGH_SCL
		call 	_LOW_SCL
		return		

_HIGH_SDA					; high impedance by making SDA an input
		bsf 	STATUS,RP0		; bank 1
		bsf 	T_SDA			; make SDA pin an input
		bcf 	STATUS,RP0		; back to bank 0
		return

_LOW_SDA
		bcf 	G_SDA	
		bsf 	STATUS,RP0		; bank 1
		bcf 	T_SDA			; make SDA pin an output
		bcf 	STATUS,RP0		; back to bank 0
		return

_HIGH_SCL
		bsf 	STATUS,RP0		; bank 1
		bsf 	T_SCL			; make SCL pin an input
		bcf 	STATUS,RP0		; back to bank 0
		return

_LOW_SCL
		bcf	G_SCL
		bsf 	STATUS,RP0		; bank 1
		bcf 	T_SCL			; make SCL pin an output
		bcf 	STATUS,RP0		; back to bank 0
		return

;******************************************************************************
;
; End of module
;
	END
