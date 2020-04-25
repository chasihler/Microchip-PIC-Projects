
START I2C
Send Address 0x90
Send 0x01	conf pointer
Send 0x225	"one shot temp"
STOP I2C

START I2C
Send Address 0x90
Send 0x00
RESTART I2C //?
Send Address 0x91
CLR NACK
READ I2C //HIGH
incf ACK_NAK?
READ I2C //LOW
STOP I2C


getTemp1 ; request TEMP conversion
call start_I2C
movlw b'10010000' ; address byte
movwf dataIO
call write_I2C ; send address byte
movlw 1
movwf dataIO ; config pointer
call write_I2C
movlw b'11100001' ; request one-shot temp conv
movwf dataIO ; temp resolution
call write_I2C
call stop_I2C
return

getTemp2 ; read TEMP from sensor
call start_I2C
movlw b'10010000' ; address byte/W
movwf dataIO
call write_I2C ; send address byte
clrf dataIO ; TEMP register pointer
call write_I2C

call start_I2C
movlw b'10010001' ; address byte/R
movwf dataIO
call write_I2C ; send address byte
clrf ACK_NAK
call read_I2C ; get TEMP high order byte
movf dataIO, w
movwf temp+1
incf ACK_NAK, f ; setup to send NAK signal
call read_I2C ; get TEMP low order byte
movf dataIO, w
movwf temp
call stop_I2C

movlw 4 ; shift temp 4 bits to the right
movwf cnt ; multiply the sign bit
bcf STATUS, C
btfsc temp+1, 7
bsf STATUS, C
rrf temp+1, f
rrf temp, f
decfsz cnt, f
goto $-6
return
tcn75.txt

9 of 10
Displaying tcn75.txt.