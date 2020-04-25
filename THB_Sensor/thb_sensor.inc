;******************************************************************
;*								  *
;*	Filename: thb_sensor.inc					  *
;*								  *
;*	J-Paul ROUBELAT - F6FBB - 5 March 2010			  *
;*								  *
;******************************************************************

	list p=12f683 ; list directive to define processor
	errorlevel	-302
	#include	<p12f683.inc>

;******************************************************************
;	Macros
;
BANK0 MACRO
	bcf	STATUS, RP0
	ENDM

BANK1	MACRO
	bsf	STATUS, RP0
	ENDM


;******************************************************************
;	GPIO ports
;
SCK	EQU	0	; serial bus clock
SDT	EQU	1	; serial bus DATA
TX	EQU	2	; transmitter data
SCL	EQU	4	; I2C clock
SDA	EQU	5	; I2C data

#define	T_SDT	TRISIO,SDT
#define	T_SCK	TRISIO,SCK
#define	T_TX	TRISIO,TX
#define	T_SCL	TRISIO,SCL
#define	T_SDA	TRISIO,SDA

#define	G_SDT	GPIO,SDT
#define	G_SCK	GPIO,SCK
#define	G_TX	GPIO,TX
#define	G_SCL	GPIO,SCL
#define	G_SDA	GPIO,SDA

;******************************************************************
;	GPIO ports
;
#define	I2C_READ	0x1	; I2C read request

;******************************************************************
; BMP085 chip specs
;
#define	BMP_ADDR	0xee	; BMP085 chip address
#define	BMP_REGT	0xf4	; register address
#define	BMP_PARM	0xaa	; base address for calibration values
#define	BMP_TEMP	0x2e	; temperature request
#define	BMP_PRES	0x34	; pressure request
#define	BMP_MEAS	0xf6	; base address for measures
#define	BMP_NPAR	.22	; nb of calib parameters

;******************************************************************
; BMP085 calibration parameters
;
#define	CALIB	0x20		; params at E2PROM @0x20
#define	AC1	CALIB+.0
#define	AC2	CALIB+.2
#define	AC3	CALIB+.4
#define	AC4	CALIB+.6
#define	AC5	CALIB+.8
#define	AC6	CALIB+.10
#define	B1	CALIB+.12
#define	B2	CALIB+.14
#define	MB	CALIB+.16
#define	MC	CALIB+.18
#define	MD	CALIB+.20
