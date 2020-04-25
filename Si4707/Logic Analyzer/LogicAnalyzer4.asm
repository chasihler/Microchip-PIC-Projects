; ============================================================================
; Logic Analyzer USB Interface
;
; Professor M. Csele, Niagara College, Canada
; http://technology.niagarac.on.ca/staff/mcsele
; Version 1.000 (2008/11/09): First test version
;         1.100 (2009/09/13): Fixed read bug (idle before reading)
;         1.200 (2011/06/22): Added Vcc select (3.3/5V) and Pull-Up select
; Uses version 4.xx Win-32 Front-End on the PC, 3.xx compatible
;
; USB Peripheral Framework based on Nick Christodoulou's interrupt-driven version of 
; Bradley Minch's (Franklin Olin College) USB framework in assembly code.
; Uses the Win-32 "libusb" driver from sourceforge.net (ver 0.1.12.0)
;
; Device enumerates as a vendor-specific device using the 'libusb.sys' driver
; and supports two types of transfers with the PC:
; #1: Transfer of two 16-bit words from the PC via wValue and wIndex
; #2: Transfer of eight bytes of data to the PC via EP0 buffer (in VendorRequests)
;
; The firmware is configured to use an external 20-MHz crystal, to operate as a
; full-speed USB device, and to use the internal pull-up resistor and 3V3regulator.
; A 470nF capacitor must be installed between the Vusb pin and ground.
;
; USB Status LED lights for 'enumerated' state, blinks for 'configured' state
; ============================================================================
;
; Hardware Defines:
;
; Port A (6 lines)
VCC_RELAY		equ		0		; RA0 - Vcc Select Relay
PULLUP_RELAY	equ		1		; RA1 - Pull Up Relay

; Port B
FCLK_0		equ		0		; RB0 - FCLK Sample Rate D0
FCLK_1		equ		1		; RB1 - FCLK Sample Rate D1
FCLK_2		equ		2		; RB2 - FCLK Sample Rate D2
CLK_EN		equ		3		; RB3 - Clock Enable (active low)
FIFO_RST	equ		4		; RB4 - Reset (active low)
READ_CLK	equ		5		; RB5 - Read Clock (active low)
; RB6/7 - ICD-2 Programming/Debugging Interface
;
TRIG_0		equ		0		; RC0 - Trig Source (C0)
TRIG_1		equ		1		; RC1 - Trig Source (C1)
TRIG_2		equ		2		; RC2 - Trig Source (C2)
; RC4/5 - USB D-/D+ Interface
TRIG_EDGE	equ		6		; RC6 - Trig Edge (C3)
TRIG_EN		equ		7		; RC7 - Trig Enable (D7)(high=trig,low=free)
;
; Port D (8 lines) - Data Input from FIFO
;
FIFO_FULL	equ		0		; RE0 - FIFO Full input (old S3)(active low)
; RE1 - spare
USB_LED		equ		2		; RE2 - USB Status LED output (READY) 


;USB Data transfer and command modes
#define CMD_IDLE        0x00        ; Idle command
#define	CMD_RESET		0x01		; Reset command (reset FIFO counters&trigger) and Idle
#define CMD_START		0x02		; Start sampling with parameters in wValue
									; D0-D2=Sample Rate (0=ext, 1=20M ... 7=4.9K)
									; D3-D6=Trig channel (0=Free, 1-8=Channel#, 9=Pattern)
									; D7=Trig Edge (0=L->H)
									; D8=Vcc Select (0=3.3V, 1=5V)
									; D9=Pull Dn/Up (0=Gnd, 1=Vcc)
									; D8-D15=(Was Pattern Option)

#define GET_FIFO_STATUS 0x03		; Get FIFO status
									; Return D7 Set (Full) in Byte 0 (return value to PC via buffer)
#define GET_FIFO_DATA   0x04		; Get eight bytes of data from USB device to PC via buffer
									; Return eight bytes in EP0 to PC

;#define	FIFO_Data		0x0200		; Target location for data from the FIFO (DEBUG/VERIFY)

;Include files
#include <p18F4550.inc>
#include "usb_defs.inc"
#include "ENGR2210.inc"				; Macros from Minch's original code

;CONFIG: Uses a PIC18F4550 with a 20MHz XTAL
			config PLLDIV = 5			; PLL Prescaler Divide by 5 (20 MHz oscillator input)
			config CPUDIV = OSC1_PLL2	; CPU System Clock Postscaler (CPU Clock:80MHz/2=40MHz)
			config USBDIV = 2
			config FOSC = HSPLL_HS      ; HS oscillator, PLL enabled, HS used by USB
			config IESO = OFF
			config PWRT = OFF
			config BOR = ON
			config BORV = 1
			config VREGEN = ON
			config WDT = OFF
			config WDTPS = 32768
			config MCLRE = ON
			config LPT1OSC = OFF
			config PBADEN = OFF
			config CCP2MX = ON
			config STVREN = ON
			config LVP = OFF
			config ICPRT = OFF
			config XINST = OFF
			config DEBUG = OFF
			config CP0 = OFF
			config CP1 = OFF
			config CP2 = OFF
			config CP3 = OFF
			config CPB = OFF
			config CPD = OFF
			config WRT0 = OFF
			config WRT1 = OFF
			config WRT2 = OFF
			config WRT3 = OFF
			config WRTB = OFF
			config WRTC = OFF
			config WRTD = OFF
			config EBTR0 = OFF
			config EBTR1 = OFF
			config EBTR2 = OFF
			config EBTR3 = OFF
			config EBTRB = OFF

;Registers
bank0		udata  
;USB Registers
USB_buffer_desc		res	4
USB_buffer_data		res	8
USB_error_flags		res 1
USB_curr_config		res	1
USB_device_status	res	1
USB_dev_req			res	1
USB_address_pending	res	1
USB_desc_ptr		res	1
USB_bytes_left		res	1
USB_loop_index		res	1
USB_packet_length	res	1
USB_USTAT			res	1
USB_USWSTAT			res	1

Counter				res 1			;Data counter for generating test data
ByteCounter			res	1			;# bytes to send via EP0 back to host PC
DelayRegisterA 		res	1			;Used for blinking the status LED in main
DelayRegisterB		res	1
DelayRegisterC      res 1
ParmwValue1			res 1			;Copy of wValue
ParmwValue2			res 1

DataReadPointerHi	res	1			;Pointer to Fifo data already read into bank 2 registers
DataReadPointerLo	res	1			;Reset when FIFO is full (and status read)


STARTUP		code		0x0000
			goto		Main					; Reset vector
			nop
			nop
			call        ServiceUSB, FAST  		; High-priority interrupt vector trap
			retfie     	FAST 					; <<<
			nop
			nop
			nop
			nop
			nop
			nop
			goto		$						; Low-priority interrupt vector trap


USBSTUFF	code
Descriptor
			movlw		upper Descriptor_begin
			movwf		TBLPTRU, ACCESS
			movlw		high Descriptor_begin
			movwf		TBLPTRH, ACCESS
			movlw		low Descriptor_begin
			banksel		USB_desc_ptr
			addwf		USB_desc_ptr, W, BANKED
			ifset STATUS, C, ACCESS
				incf		TBLPTRH, F, ACCESS
				ifset STATUS, Z, ACCESS
					incf		TBLPTRU, F, ACCESS
				endi
			endi
			movwf		TBLPTRL, ACCESS
			tblrd*
			movf		TABLAT, W
			return

Descriptor_begin
Device
			db			0x12, DEVICE		; bLength, bDescriptorType
			db			0x10, 0x01			; bcdUSB (low byte), bcdUSB (high byte)
			db			0x00, 0x00			; bDeviceClass, bDeviceSubClass
			db			0x00, MAX_PACKET_SIZE	; bDeviceProtocol, bMaxPacketSize
			db			0xD8, 0x04			; idVendor (low byte), idVendor (high byte)
			db			0x04, 0x00			; idProduct (low byte), idProduct (high byte)
			db			0x00, 0x00			; bcdDevice (low byte), bcdDevice (high byte)
			db			0x01, 0x02			; iManufacturer, iProduct
			db			0x00, NUM_CONFIGURATIONS	; iSerialNumber (none), bNumConfigurations

Configuration1
			db			0x09, CONFIGURATION	; bLength, bDescriptorType
			db			0x12, 0x00			; wTotalLength (low byte), wTotalLength (high byte)
			db			NUM_INTERFACES, 0x01	; bNumInterfaces, bConfigurationValue
			db			0x00, 0xA0			; iConfiguration (none), bmAttributes
			db			0x32, 0x09			; bMaxPower (100 mA), bLength (Interface1 descriptor starts here)
			db			INTERFACE, 0x00		; bDescriptorType, bInterfaceNumber
			db			0x00, 0x00			; bAlternateSetting, bNumEndpoints (excluding EP0)
			db			0xFF, 0x00			; bInterfaceClass (vendor specific class code), bInterfaceSubClass
			db			0xFF, 0x00			; bInterfaceProtocol (vendor specific protocol used), iInterface (none)

String0
			db			String1-String0, STRING		; bLength, bDescriptorType
			db			0x09, 0x04					; wLANGID[0] (low byte), wLANGID[0] (high byte)
String1
			db			String2-String1, STRING		; bLength, bDescriptorType
			db			'M', 0x00					; bString
			db			'i', 0x00
			db			'c', 0x00
			db			'r', 0x00
			db			'o', 0x00
			db			'c', 0x00
			db			'h', 0x00
			db			'i', 0x00
			db			'p', 0x00
			db			' ', 0x00
			db			'T', 0x00
			db			'e', 0x00
			db			'c', 0x00
			db			'h', 0x00
			db			'n', 0x00
			db			'o', 0x00
			db			'l', 0x00
			db			'o', 0x00
			db			'g', 0x00
			db			'y', 0x00
			db			',', 0x00
			db			' ', 0x00
			db			'I', 0x00
			db			'n', 0x00
			db			'c', 0x00
			db			'.', 0x00
String2
			db			Descriptor_end-String2, STRING	; bLength, bDescriptorType
			db			'L', 0x00						; bString
			db			'o', 0x00
			db			'g', 0x00
			db			'i', 0x00
			db			'c', 0x00
			db			' ', 0x00
			db			'A', 0x00
			db			'n', 0x00
			db			'a', 0x00
			db			'l', 0x00
			db			'y', 0x00
			db			'z', 0x00
			db			'e', 0x00
			db			'r', 0x00
			db			' ', 0x00
			db			'3', 0x00
			db			'.', 0x00
			db			'X', 0x00
			db			' ', 0x00
			db			'U', 0x00
			db			'S', 0x00
			db			'B', 0x00
Descriptor_end

InitUSB
			clrf		UIE, ACCESS				; mask all USB interrupts
			clrf		UIR, ACCESS				; clear all USB interrupt flags
			movlw		0x14
			movwf		UCFG, ACCESS			; configure USB for full-speed transfers and to use the on-chip transciever and pull-up resistor
			movlw		0x08
			movwf		UCON, ACCESS			; enable the USB module and its supporting circuitry
			banksel		USB_curr_config
			clrf		USB_curr_config, BANKED
			clrf		USB_USWSTAT, BANKED		; default to powered state
			movlw		0x01
			movwf		USB_device_status, BANKED
			movlw		NO_REQUEST
			movwf		USB_dev_req, BANKED		; No device requests in process
			repeat								; do nothing...
			untilclr UCON, SE0, ACCESS			; ...until initial SE0 condition clears 
			return

ServiceUSB
			bcf PIR2,USBIF
			select										; UIR: USB INTERRUPT STATUS REGISTER
				caseset	UIR, UERRIF, ACCESS				; UERRIF: USB Error Condition Interrupt bit
					clrf		UEIR, ACCESS			; UEIR: USB ERROR INTERRUPT STATUS REGISTER
					break
				caseset UIR, SOFIF, ACCESS				; START-OF-FRAME Token Interrupt bit
					bcf			UIR, SOFIF, ACCESS		 
					break
				caseset	UIR, IDLEIF, ACCESS				; Idle condition detected (constant Idle state of 3 ms or more)
					bcf			UIR, IDLEIF, ACCESS
					bsf			UCON, SUSPND, ACCESS	; UCON: USB CONTROL REGISTER, SUSPND: USB module and supporting circuitry in Power Conserve mode, SIE clock inactive
					break
				caseset UIR, ACTVIF, ACCESS				; Bus Activity Detect Interrupt bit (Activity on the D+/D- lines was detected)
					bcf			UIR, ACTVIF, ACCESS
					bcf			UCON, SUSPND, ACCESS
					break
				caseset	UIR, STALLIF, ACCESS			; A STALL Handshake Interrupt bit (STALL handshake was sent by the SIE)
					bcf			UIR, STALLIF, ACCESS
					break
				caseset	UIR, URSTIF, ACCESS				; USB Reset Interrupt bit (Valid USB Reset occurred; 00h is loaded into UADDR register)
					banksel		USB_curr_config
					clrf		USB_curr_config, BANKED
					bcf			UIR, TRNIF, ACCESS		; Clear TRNIF four times to clear out the USTAT FIFO
					bcf 		UIR, TRNIF, ACCESS		; Transaction Complete Interrupt bit (Clearing this bit will cause the USTAT FIFO to advance)
					bcf			UIR, TRNIF, ACCESS		
					bcf			UIR, TRNIF, ACCESS
					clrf		UEP0, ACCESS			; clear all EP control registers to disable all endpoints
					clrf		UEP1, ACCESS
					clrf		UEP2, ACCESS
					clrf		UEP3, ACCESS
					clrf		UEP4, ACCESS
					clrf		UEP5, ACCESS
					clrf		UEP6, ACCESS
					clrf		UEP7, ACCESS
					clrf		UEP8, ACCESS
					clrf		UEP9, ACCESS
					clrf		UEP10, ACCESS
					clrf		UEP11, ACCESS
					clrf		UEP12, ACCESS
					clrf		UEP13, ACCESS
					clrf		UEP14, ACCESS
					clrf		UEP15, ACCESS
					banksel		BD0OBC
					movlw		MAX_PACKET_SIZE
					movwf		BD0OBC, BANKED
					movlw		low USB_Buffer			; EP0 OUT gets a buffer...
					movwf		BD0OAL, BANKED
					movlw		high USB_Buffer
					movwf		BD0OAH, BANKED			; ...set up its address
					movlw		0x88					; set UOWN bit (USB can write)
					movwf		BD0OST, BANKED
					movlw		low (USB_Buffer+MAX_PACKET_SIZE)	; EP0 IN gets a buffer...
					movwf		BD0IAL, BANKED
					movlw		high (USB_Buffer+MAX_PACKET_SIZE)
					movwf		BD0IAH, BANKED			; ...set up its address
					movlw		0x08					; clear UOWN bit (MCU can write)
					movwf		BD0IST, BANKED

					clrf		UADDR, ACCESS			; set USB Address to 0
					clrf		UIR, ACCESS				; clear all the USB interrupt flags
					movlw		ENDPT_CONTROL
					movwf		UEP0, ACCESS			; EP0 is a control pipe and requires an ACK

					movlw		0xFF					; enable all error interrupts
					movwf		UEIE, ACCESS
					banksel		USB_USWSTAT
					movlw		DEFAULT_STATE
					movwf		USB_USWSTAT, BANKED
					movlw		0x01
					movwf		USB_device_status, BANKED	; self powered, remote wakeup disabled
					break
				caseset	UIR, TRNIF, ACCESS				; Transaction Complete Interrupt bit (read USTAT register for endpoint information)
					movlw		0x04
					movwf		FSR0H, ACCESS
					movf		USTAT, W, ACCESS
					andlw		0x7C					; mask out bits 0, 1, and 7 of USTAT
					movwf		FSR0L, ACCESS
					banksel		USB_buffer_desc
					movf		POSTINC0, W
					movwf		USB_buffer_desc, BANKED
					movf		POSTINC0, W
					movwf		USB_buffer_desc+1, BANKED
					movf		POSTINC0, W
					movwf		USB_buffer_desc+2, BANKED
					movf		POSTINC0, W
					movwf		USB_buffer_desc+3, BANKED
					movf		USTAT, W, ACCESS
					movwf		USB_USTAT, BANKED		; save the USB status register
					bcf			UIR, TRNIF, ACCESS		; clear TRNIF interrupt flag
					clrf		USB_error_flags, BANKED	; clear USB error flags
					movf		USB_buffer_desc, W, BANKED
					andlw		0x3C					; extract PID bits
					select
						case TOKEN_SETUP
							call		ProcessSetupToken
							break
						case TOKEN_IN
							call		ProcessInToken
							break
						case TOKEN_OUT
							call		ProcessOutToken
							break
					ends
					banksel USB_error_flags
					ifset USB_error_flags, 0, BANKED	; if there was a Request Error...
						banksel		BD0OBC
						movlw		MAX_PACKET_SIZE
						movwf		BD0OBC				; ...get ready to receive the next Setup token...
						movlw		0x84
						movwf		BD0IST
						movwf		BD0OST				; ...and issue a protocol stall on EP0
					endi
					break
			ends
			return

ProcessSetupToken
			banksel		USB_buffer_data
			movf		USB_buffer_desc+ADDRESSH, W, BANKED
			movwf		FSR0H, ACCESS
			movf		USB_buffer_desc+ADDRESSL, W, BANKED
			movwf		FSR0L, ACCESS
			movf		POSTINC0, W
			movwf		USB_buffer_data, BANKED
			movf		POSTINC0, W
			movwf		USB_buffer_data+1, BANKED
			movf		POSTINC0, W
			movwf		USB_buffer_data+2, BANKED
			movf		POSTINC0, W
			movwf		USB_buffer_data+3, BANKED
			movf		POSTINC0, W
			movwf		USB_buffer_data+4, BANKED
			movf		POSTINC0, W
			movwf		USB_buffer_data+5, BANKED
			movf		POSTINC0, W
			movwf		USB_buffer_data+6, BANKED
			movf		POSTINC0, W
			movwf		USB_buffer_data+7, BANKED
			banksel		BD0OBC
			movlw		MAX_PACKET_SIZE
			movwf		BD0OBC, BANKED					; reset the byte count
			movwf		BD0IST, BANKED					; return the in buffer to us (dequeue any pending requests)
			banksel		USB_buffer_data+bmRequestType
			ifclr USB_buffer_data+bmRequestType, 7, BANKED
				ifl USB_buffer_data+wLength, !=, 0
            	orif USB_buffer_data+wLengthHigh, !=, 0
					movlw		0xC8
				otherwise
					movlw		0x88
				endi
			otherwise
				movlw		0x88
			endi
			banksel		BD0OST
			movwf		BD0OST, BANKED					; set EP0 OUT UOWN back to USB and DATA0/DATA1 packet according to request type
			bcf			UCON, PKTDIS, ACCESS			; assuming there is nothing to dequeue, clear the packet disable bit
			banksel		USB_dev_req
			movlw		NO_REQUEST
			movwf		USB_dev_req, BANKED				; clear the device request in process
			movf		USB_buffer_data+bmRequestType, W, BANKED
			andlw		0x60							; extract request type bits
			select
				case STANDARD
					call		StandardRequests
					break
				case CLASS
					call		ClassRequests
					break
				case VENDOR
					call		VendorRequests
					break
				default
					bsf			USB_error_flags, 0, BANKED	; set Request Error flag
			ends
			return

StandardRequests
			movf		USB_buffer_data+bRequest, W, BANKED
			select
				case GET_STATUS
					movf		USB_buffer_data+bmRequestType, W, BANKED
					andlw		0x1F					; extract request recipient bits
					select
						case RECIPIENT_DEVICE
							banksel		BD0IAH
							movf		BD0IAH, W, BANKED
							movwf		FSR0H, ACCESS
							movf		BD0IAL, W, BANKED				; get buffer pointer
							movwf		FSR0L, ACCESS
							banksel		USB_device_status
							movf		USB_device_status, W, BANKED	; copy device status byte to EP0 buffer
							movwf		POSTINC0
							clrf		INDF0
							banksel		BD0IBC
							movlw		0x02
							movwf		BD0IBC, BANKED					; set byte count to 2
							movlw		0xC8
							movwf		BD0IST, BANKED					; send packet as DATA1, set UOWN bit
							break
						case RECIPIENT_INTERFACE
							movf		USB_USWSTAT, W, BANKED
							select
								case ADDRESS_STATE
									bsf			USB_error_flags, 0, BANKED		; set Request Error flag
									break
								case CONFIG_STATE
									ifl USB_buffer_data+wIndex, <, NUM_INTERFACES
										banksel		BD0IAH
										movf		BD0IAH, W, BANKED
										movwf		FSR0H, ACCESS
										movf		BD0IAL, W, BANKED				; get buffer pointer
										movwf		FSR0L, ACCESS
										clrf		POSTINC0
										clrf		INDF0
										movlw		0x02
										movwf		BD0IBC, BANKED					; set byte count to 2
										movlw		0xC8
										movwf		BD0IST, BANKED					; send packet as DATA1, set UOWN bit																								
									otherwise
										bsf			USB_error_flags, 0, BANKED		; set Request Error flag
									endi
									break
							ends
							break
						case RECIPIENT_ENDPOINT
							movf		USB_USWSTAT, W, BANKED
							select
								case ADDRESS_STATE
									movf		USB_buffer_data+wIndex, W, BANKED	; get EP
									andlw		0x0F								; strip off direction bit
									ifset STATUS, Z, ACCESS							; see if it is EP0
										banksel		BD0IAH
										movf		BD0IAH, W, BANKED				; put EP0 IN buffer pointer...
										movwf		FSR0H, ACCESS
										movf		BD0IAL, W, BANKED
										movwf		FSR0L, ACCESS					; ...into FSR0
										banksel		USB_buffer_data+wIndex
										ifset USB_buffer_data+wIndex, 7, BANKED		; if the specified direction is IN...
											banksel		BD0IST
											movf		BD0IST, W, BANKED
										otherwise
											banksel		BD0OST
											movf		BD0OST, W, BANKED
										endi
										andlw		0x04							; extract the BSTALL bit
										movwf		INDF0
										rrncf		INDF0, F
										rrncf		INDF0, F						; shift BSTALL bit into the lsb position
										clrf		PREINC0
										movlw		0x02
										movwf		BD0IBC, BANKED					; set byte count to 2
										movlw		0xC8
										movwf		BD0IST, BANKED					; send packet as DATA1, set UOWN bit
									otherwise
										bsf			USB_error_flags, 0, BANKED		; set Request Error flag
									endi
									break
								case CONFIG_STATE
									banksel		BD0IAH
									movf		BD0IAH, W, BANKED					; put EP0 IN buffer pointer...
									movwf		FSR0H, ACCESS
									movf		BD0IAL, W, BANKED
									movwf		FSR0L, ACCESS						; ...into FSR0
									movlw		high UEP0							; put UEP0 address...
									movwf		FSR1H, ACCESS
									movlw		low UEP0
									movwf		FSR1L, ACCESS						; ...into FSR1
									movlw		high BD0OST							; put BDndST address...
									movwf		FSR2H, ACCESS
									banksel		USB_buffer_data+wIndex
									movf		USB_buffer_data+wIndex, W, BANKED
									andlw		0x8F								; mask out all but the direction bit and EP number
									movwf		FSR2L, ACCESS
									rlncf		FSR2L, F, ACCESS
									rlncf		FSR2L, F, ACCESS
									rlncf		FSR2L, F, ACCESS					; FSR2L now contains the proper offset into the BD table for the specified EP
									movlw		low BD0OST
									addwf		FSR2L, F, ACCESS					; ...into FSR2
									ifset STATUS, C, ACCESS
										incf		FSR2H, F, ACCESS
									endi
									movf		USB_buffer_data+wIndex, W, BANKED	; get EP and...
									andlw		0x0F								; ...strip off direction bit
									ifset USB_buffer_data+wIndex, 7, BANKED			; if the specified EP direction is IN...
									andifclr PLUSW1, EPINEN, ACCESS					; ...and the specified EP is not enabled for IN transfers...
										bsf			USB_error_flags, 0, BANKED		; ...set Request Error flag
									elsifclr USB_buffer_data+wIndex, 7, BANKED		; otherwise, if the specified EP direction is OUT...
									andifclr PLUSW1, EPOUTEN, ACCESS				; ...and the specified EP is not enabled for OUT transfers...
										bsf			USB_error_flags, 0, BANKED		; ...set Request Error flag
									otherwise
										movf		INDF2, W						; move the contents of the specified BDndST register into WREG
										andlw		0x04							; extract the BSTALL bit
										movwf		INDF0
										rrncf		INDF0, F
										rrncf		INDF0, F						; shift BSTALL bit into the lsb position
										clrf		PREINC0										
										banksel		BD0IBC
										movlw		0x02
										movwf		BD0IBC, BANKED					; set byte count to 2
										movlw		0xC8
										movwf		BD0IST, BANKED					; send packet as DATA1, set UOWN bit
									endi
									break
								default
									bsf			USB_error_flags, 0, BANKED	; set Request Error flag
							ends
							break
						default
							bsf			USB_error_flags, 0, BANKED	; set Request Error flag
					ends
					break
				case CLEAR_FEATURE
				case SET_FEATURE
					movf		USB_buffer_data+bmRequestType, W, BANKED
					andlw		0x1F					; extract request recipient bits
					select
						case RECIPIENT_DEVICE
							movf		USB_buffer_data+wValue, W, BANKED
							select
								case DEVICE_REMOTE_WAKEUP
									ifl USB_buffer_data+bRequest, ==, CLEAR_FEATURE
										bcf			USB_device_status, 1, BANKED
									otherwise
										bsf			USB_device_status, 1, BANKED
									endi
									banksel		BD0IBC
									clrf		BD0IBC, BANKED					; set byte count to 0
									movlw		0xC8
									movwf		BD0IST, BANKED					; send packet as DATA1, set UOWN bit
									break
								default
									bsf			USB_error_flags, 0, BANKED		; set Request Error flag
							ends
							break
						case RECIPIENT_ENDPOINT
							movf		USB_USWSTAT, W, BANKED
							select
								case ADDRESS_STATE
									movf		USB_buffer_data+wIndex, W, BANKED	; get EP
									andlw		0x0F								; strip off direction bit
									ifset STATUS, Z, ACCESS							; see if it is EP0
										banksel		BD0IBC
										clrf		BD0IBC, BANKED					; set byte count to 0
										movlw		0xC8
										movwf		BD0IST, BANKED					; send packet as DATA1, set UOWN bit
									otherwise
										bsf			USB_error_flags, 0, BANKED		; set Request Error flag
									endi
									break
								case CONFIG_STATE
									movlw		high UEP0							; put UEP0 address...
									movwf		FSR0H, ACCESS
									movlw		low UEP0
									movwf		FSR0L, ACCESS						; ...into FSR0
									movlw		high BD0OST							; put BD0OST address...
									movwf		FSR1H, ACCESS
									movlw		low BD0OST
									movwf		FSR1L, ACCESS						; ...into FSR1
									movf		USB_buffer_data+wIndex, W, BANKED	; get EP
									andlw		0x0F								; strip off direction bit
									ifclr STATUS, Z, ACCESS							; if it was not EP0...
										addwf		FSR0L, F, ACCESS					; add EP number to FSR0
										ifset		STATUS, C, ACCESS
											incf		FSR0H, F, ACCESS
										endi
										rlncf		USB_buffer_data+wIndex, F, BANKED
										rlncf		USB_buffer_data+wIndex, F, BANKED
										rlncf		USB_buffer_data+wIndex, W, BANKED	; WREG now contains the proper offset into the BD table for the specified EP
										andlw		0x7C								; mask out all but the direction bit and EP number (after three left rotates)
										addwf		FSR1L, F, ACCESS					; add BD table offset to FSR1
										ifset		STATUS, C, ACCESS
											incf		FSR1H, F, ACCESS
										endi
										ifset USB_buffer_data+wIndex, 1, BANKED			; if the specified EP direction (now bit 1) is IN...
											ifset INDF0, EPINEN, ACCESS						; if the specified EP is enabled for IN transfers...
												ifl USB_buffer_data+bRequest, ==, CLEAR_FEATURE
													clrf		INDF1					; clear the stall on the specified EP
												otherwise
													movlw		0x84
													movwf		INDF1					; stall the specified EP
												endi
											otherwise
												bsf			USB_error_flags, 0, BANKED		; set Request Error flag
											endi
										otherwise										; ...otherwise the specified EP direction is OUT, so...
											ifset INDF0, EPOUTEN, ACCESS					; if the specified EP is enabled for OUT transfers...
												ifl USB_buffer_data+bRequest, ==, CLEAR_FEATURE
													movlw		0x88
													movwf		INDF1					; clear the stall on the specified EP													
												otherwise
													movlw		0x84
													movwf		INDF1					; stall the specified EP
												endi
											otherwise
												bsf			USB_error_flags, 0, BANKED		; set Request Error flag
											endi
										endi
									endi
									ifclr USB_error_flags, 0, BANKED	; if there was no Request Error...
										banksel		BD0IBC
										clrf		BD0IBC, BANKED			; set byte count to 0
										movlw		0xC8
										movwf		BD0IST, BANKED			; send packet as DATA1, set UOWN bit
									endi
									break
								default
									bsf			USB_error_flags, 0, BANKED	; set Request Error flag
							ends
							break
						default
							bsf			USB_error_flags, 0, BANKED	; set Request Error flag
					ends
					break
				case SET_ADDRESS
					ifset USB_buffer_data+wValue, 7, BANKED		; if new device address is illegal, send Request Error
						bsf			USB_error_flags, 0, BANKED	; set Request Error flag
					otherwise
						movlw		SET_ADDRESS
						movwf		USB_dev_req, BANKED			; processing a SET_ADDRESS request
						movf		USB_buffer_data+wValue, W, BANKED
						movwf		USB_address_pending, BANKED	; save new address
						banksel		BD0IBC
						clrf		BD0IBC, BANKED				; set byte count to 0
						movlw		0xC8
						movwf		BD0IST, BANKED				; send packet as DATA1, set UOWN bit
					endi
					break
				case GET_DESCRIPTOR
					movwf		USB_dev_req, BANKED				; processing a GET_DESCRIPTOR request
					movf		USB_buffer_data+(wValue+1), W, BANKED
					select
						case DEVICE
							movlw		low (Device-Descriptor_begin)
							movwf		USB_desc_ptr, BANKED
							call		Descriptor				; get descriptor length
							movwf		USB_bytes_left, BANKED
							ifl USB_buffer_data+(wLength+1), ==, 0
							andiff USB_buffer_data+wLength, <, USB_bytes_left
								movf		USB_buffer_data+wLength, W, BANKED
								movwf		USB_bytes_left, BANKED
							endi
							call		SendDescriptorPacket
							break
						case CONFIGURATION
							movf		USB_buffer_data+wValue, W, BANKED
							select
								case 0
									movlw		low (Configuration1-Descriptor_begin)
									break
								default
									bsf			USB_error_flags, 0, BANKED	; set Request Error flag
							ends
							ifclr USB_error_flags, 0, BANKED
								addlw		0x02				; add offset for wTotalLength
								movwf		USB_desc_ptr, BANKED
								call		Descriptor			; get total descriptor length
								movwf		USB_bytes_left, BANKED
								movlw		0x02
								subwf		USB_desc_ptr, F, BANKED	; subtract offset for wTotalLength
								ifl USB_buffer_data+(wLength+1), ==, 0
								andiff USB_buffer_data+wLength, <, USB_bytes_left
									movf		USB_buffer_data+wLength, W, BANKED
									movwf		USB_bytes_left, BANKED
								endi
								call		SendDescriptorPacket
							endi
							break
						case STRING
							movf		USB_buffer_data+wValue, W, BANKED
							select
								case 0
									movlw		low (String0-Descriptor_begin)
									break
								case 1
									movlw		low (String1-Descriptor_begin)
									break
								case 2
									movlw		low (String2-Descriptor_begin)
									break
								default
									bsf			USB_error_flags, 0, BANKED	; Set Request Error flag
							ends
							ifclr USB_error_flags, 0, BANKED
								movwf		USB_desc_ptr, BANKED
								call		Descriptor		; get descriptor length
								movwf		USB_bytes_left, BANKED
								ifl USB_buffer_data+(wLength+1), ==, 0
								andiff USB_buffer_data+wLength, <, USB_bytes_left
									movf		USB_buffer_data+wLength, W, BANKED
									movwf		USB_bytes_left, BANKED
								endi
								call		SendDescriptorPacket
							endi
							break
						default
							bsf			USB_error_flags, 0, BANKED	; set Request Error flag
					ends
					break
				case GET_CONFIGURATION
					banksel		BD0IAH
					movf		BD0IAH, W, BANKED
					movwf		FSR0H, ACCESS
					movf		BD0IAL, W, BANKED
					movwf		FSR0L, ACCESS
					banksel		USB_curr_config
					movf		USB_curr_config, W, BANKED
					movwf		INDF0					; copy current device configuration to EP0 IN buffer
					banksel		BD0IBC
					movlw		0x01
					movwf		BD0IBC, BANKED			; set EP0 IN byte count to 1
					movlw		0xC8
					movwf		BD0IST, BANKED			; send packet as DATA1, set UOWN bit
					break
				case SET_CONFIGURATION
					ifl USB_buffer_data+wValue, <=, NUM_CONFIGURATIONS
						clrf		UEP1, ACCESS		; clear all EP control registers except for EP0 to disable EP1-EP15 prior to setting configuration
						clrf		UEP2, ACCESS
						clrf		UEP3, ACCESS
						clrf		UEP4, ACCESS
						clrf		UEP5, ACCESS
						clrf		UEP6, ACCESS
						clrf		UEP7, ACCESS
						clrf		UEP8, ACCESS
						clrf		UEP9, ACCESS
						clrf		UEP10, ACCESS
						clrf		UEP11, ACCESS
						clrf		UEP12, ACCESS
						clrf		UEP13, ACCESS
						clrf		UEP14, ACCESS
						clrf		UEP15, ACCESS
						movf		USB_buffer_data+wValue, W, BANKED
						movwf		USB_curr_config, BANKED
						select
							case 0
								movlw		ADDRESS_STATE
								movwf		USB_USWSTAT, BANKED
								break
							default									; Place code to initialise the buffer descriptors of enpoints, 
								movlw		CONFIG_STATE			; that are defined with endpoints descriptors at the beginning  
								movwf		USB_USWSTAT, BANKED

								banksel		BD1OBC
								movlw		0x08
								movwf		BD1OBC, BANKED			; Set EP1 OUT Byte Count register set to 8
								movlw		low (USB_Buffer+0x10)	; EP1 OUT gets a buffer...(the previous 16 bytes contain...
								movwf		BD1OAL, BANKED			;...EP0 OUT/IN Byte Count registers)
								movlw		high (USB_Buffer+0x10)
								movwf		BD1OAH, BANKED			; ...set its address
								movlw		0xC8
								movwf		BD1OST, BANKED			; set UOWN bit (USB can write EP1 OUT buffer)
								movlw		0x08
								movwf		BD1IBC, BANKED			; set EP1 IN byte count to 8 
								movlw		low (USB_Buffer+0x18)	; EP1 IN gets a buffer...
								movwf		BD1IAL, BANKED
								movlw		high (USB_Buffer+0x18)
								movwf		BD1IAH, BANKED			; ...set its address
								movlw		0x48
								movwf		BD1IST, BANKED			; clear UOWN bit (PIC can write EP1 IN buffer)
								movlw		ENDPT_NON_CONTROL
								movwf		UEP1, ACCESS			; Enable EP1 for for in and out tranfers

								movlw		0x40
								movwf		BD2OBC, BANKED			; set EP2 OUT byte count to 64 
								movlw		low (USB_Buffer+0x58)	; EP2 OUT gets a buffer...
								movwf		BD2OAL, BANKED
								movlw		high (USB_Buffer+0x58)
								movwf		BD2OAH, BANKED			; ...set its address
								movlw		0xC8
								movwf		BD2OST, BANKED			; clear UOWN bit (PIC can write EP1 IN buffer)
								movlw		ENDPT_OUT_ONLY
								movwf		UEP2, ACCESS			; Enable EP2 for for in and out tranfers
						ends
						banksel		BD0IBC
						clrf		BD0IBC, BANKED			; set byte count to 0
						movlw		0xC8
						movwf		BD0IST, BANKED			; send packet as DATA1, set UOWN bit
					otherwise
						bsf			USB_error_flags, 0, BANKED	; set Request Error flag
					endi
					break
				case GET_INTERFACE
					movf		USB_USWSTAT, W, BANKED
					select
						case CONFIG_STATE
							ifl USB_buffer_data+wIndex, <, NUM_INTERFACES
								banksel		BD0IAH
								movf		BD0IAH, W, BANKED
								movwf		FSR0H, ACCESS
								movf		BD0IAL, W, BANKED		; get buffer pointer
								movwf		FSR0L, ACCESS
								clrf		INDF0					; always send back 0 for bAlternateSetting
								movlw		0x01
								movwf		BD0IBC, BANKED			; set byte count to 1
								movlw		0xC8
								movwf		BD0IST, BANKED			; send packet as DATA1, set UOWN bit
							otherwise
								bsf			USB_error_flags, 0, BANKED	; set Request Error flag
							endi
							break
						default
							bsf			USB_error_flags, 0, BANKED	; set Request Error flag
					ends
					break
				case SET_INTERFACE
					movf		USB_USWSTAT, W, BANKED
					select
						case CONFIG_STATE
							ifl USB_buffer_data+wIndex, <, NUM_INTERFACES
								movf		USB_buffer_data+wValue, W, BANKED
								select
									case 0									; currently support only bAlternateSetting of 0
										banksel		BD0IBC
										clrf		BD0IBC, BANKED			; set byte count to 0
										movlw		0xC8
										movwf		BD0IST, BANKED			; send packet as DATA1, set UOWN bit
										break
									default
										bsf			USB_error_flags, 0, BANKED	; set Request Error flag
								ends
							otherwise
								bsf			USB_error_flags, 0, BANKED	; set Request Error flag
							endi
							break
						default
							bsf			USB_error_flags, 0, BANKED	; set Request Error flag
					ends
					break
				case SET_DESCRIPTOR
				case SYNCH_FRAME
				default
					bsf			USB_error_flags, 0, BANKED	; set Request Error flag
					break
			ends
			return

ClassRequests
			movf		USB_buffer_data+bRequest, W, BANKED
			select
				default
					bsf			USB_error_flags, 0, BANKED	; set Request Error flag
			ends
			return

;**********************************************
; All Logic Analyzer commands are handled here
;**********************************************
VendorRequests
			movf		USB_buffer_data+bRequest, W, BANKED
			select
				case CMD_IDLE		; Do nothing
					banksel		BD0IBC
					clrf		BD0IBC, BANKED			; set byte count to 0
					movlw		0xC8
					movwf		BD0IST, BANKED			; send packet as DATA1, set UOWN bit
					break
				case CMD_RESET		; Reset command (reset FIFO counters and trigger)
					movlw		b'00111000'				; Idle: Clock disabled, Reset high, ReadClock high
					movwf		PORTB
					bcf			PORTB,FIFO_RST			; Toggle FIFO reset line low-then-high
					nop
					bsf			PORTB,FIFO_RST
					banksel		BD0IBC
					clrf		BD0IBC, BANKED			; set byte count to 0
					movlw		0xC8
					movwf		BD0IST, BANKED			; send packet as DATA1, set UOWN bit
					break
				case CMD_START		; Enable channel-triggered sampling
									; D0-D15 in wValue is argument (Sample Rate, Trig)
					;Copy wValue from incoming packet to local PIC variable
					;FSixxteen bit argument, grab both halves
					movf		USB_buffer_data+(wValue+1), W, BANKED
					movwf		ParmwValue2,BANKED
					movf		USB_buffer_data+wValue, W, BANKED
					movwf		ParmwValue1,BANKED

					;Set Sample Rate (Bits D0-D2 in argument)
					andlw		b'00000111'
					iorlw		b'00111000'				; Ensure Clk/Rst/Enable lines stay high
					movwf		PORTB
					;Set Trigger Source on Port C (Bits D3-D6 in argument)
					movf		ParmwValue1,w,BANKED
					andlw		b'01111000'
					btfsc		STATUS,Z				;Check if argument was zero (free run)
					goto		FreeRunStart
TriggeredStart
					rrncf		ParmwValue1,f,BANKED	;Align Trigger source to LSB position			
					rrncf		ParmwValue1,f,BANKED
					rrncf		ParmwValue1,f,BANKED
					decf		ParmwValue1,w,BANKED	;Decrement argument by one
					andlw		b'00000111'				
					movwf		PORTC					;Set trigger channel on '251 chip
					btfsc		ParmwValue1,4,BANKED	
					bsf			PORTC,TRIG_EDGE			;Set edge (h-->l or l-->h)
  					bsf			PORTC,TRIG_EN 			;Enable trigger
					goto		AnalyzerRunning
FreeRunStart	
					bcf			PORTC,TRIG_EN			;Trigger inhibit - free running
AnalyzerRunning
					btfsc		ParmwValue2,0,BANKED	
					bsf			PORTA,VCC_RELAY			;Set Vcc relay for 5V
					btfss		ParmwValue2,0,BANKED	
					bcf			PORTA,VCC_RELAY			;Set Vcc relay for 3.3V
					btfsc		ParmwValue2,1,BANKED	
					bsf			PORTA,PULLUP_RELAY		;Set PullUp relay for Vcc
					btfss		ParmwValue2,1,BANKED	
					bcf			PORTA,PULLUP_RELAY		;Set PullUp relay for Ground
					
					bcf			PORTB,FIFO_RST			;Toggle FIFO reset line low-then-high
					nop									;Resets trigger FF just prior to start
					bsf			PORTB,FIFO_RST	
					bcf			PORTB,CLK_EN			;Enable clock
				
					banksel		BD0IBC
					clrf		BD0IBC, BANKED			; set byte count to 0
					movlw		0xC8
					movwf		BD0IST, BANKED			; send packet as DATA1, set UOWN bit
					break
				case GET_FIFO_STATUS	; Get FIFO status and return in EP0
										; Return D7 Set (Full) in Byte 0 (return value to PC via buffer)
					banksel		BD0IAH
					movf		BD0IAH, W, BANKED		; put EP0 IN buffer pointer...
					movwf		FSR0H, ACCESS
					movf		BD0IAL, W, BANKED
					movwf		FSR0L, ACCESS			; ...into FSR0
					btfss		PORTE,FIFO_FULL
					bra			FifoFull

					movlw		0x00					; "Fifo empty" default
					movwf		POSTINC0
					banksel		BD0IBC
					movlw		0x08
					movwf		BD0IBC,BANKED					; set byte count to 8
					movlw		0xC8
					movwf		BD0IST,BANKED					; send packet as DATA1, set UOWN bit
					break

FifoFull
;***** DEBUG/VERIFY Code: can view data directly from FIFO here *****
;					movlw		high (FIFO_Data)		;Copy all fifo data to memory to inspect
;					movwf		FSR1H
;					movlw		low (FIFO_Data)
;					movwf		FSR1L

;					movlw		b'00111000'				; Idle: Clock disabled, Reset high, ReadClock high
;					movwf		PORTB
					
;					banksel		Counter					;Copy Data to Register memory
;					movlw		.0						;Do 256 bytes
;					movwf		ByteCounter,BANKED
;CopyNextDataByte
;					nop
;					bsf			PORTB,READ_CLK			;Generate a clock pulse for read pointer on FIFO
;					nop
;					nop		
;					bcf			PORTB,READ_CLK
;					nop
;					movf		PORTD,w					;Get data from fifo ...
;					movwf		POSTINC1				; ... and put into the EP0 IN buffer
;					decfsz		ByteCounter,f,BANKED
;					goto		CopyNextDataByte

;					banksel		DataReadPointerHi
;					movlw		high (FIFO_Data)		;RESET Pointer for READ via USB
;					movwf		DataReadPointerHi
;					movlw		low (FIFO_Data)
;					movwf		DataReadPointerLo

;********** BREAK HERE to see data in bank 2 ***************
;					nop
;					nop									;All FIFO data should now be in Bank 2 registers (255 bytes)

					movlw		0x80					; "Fifo full" status (if /FF is low)
					movwf		POSTINC0
					banksel		BD0IBC
					movlw		0x08
					movwf		BD0IBC,BANKED					; set byte count to 8
					movlw		0xC8
					movwf		BD0IST,BANKED					; send packet as DATA1, set UOWN bit
					break
				case GET_FIFO_DATA		; Get eight bytes of data from USB device to PC via buffer
					banksel		BD0IAH
					movf		BD0IAH, W, BANKED		; put EP0 IN buffer pointer...
					movwf		FSR0H, ACCESS
					movf		BD0IAL, W, BANKED
					movwf		FSR0L, ACCESS			; ...into FSR0

					;Put into idle before reading to avoid stomping on data already in the fifo
					movlw		b'00111000'				; Idle: Clock disabled, Reset high, ReadClock high
					movwf		PORTB
					
					banksel		Counter
					movlw		.8
					movwf		ByteCounter,BANKED
NextDataByte
					bcf			PORTB,READ_CLK			;Generate a clock pulse for read pointer on FIFO
					nop	
					bsf			PORTB,READ_CLK
					movf		PORTD,w					;Get data from fifo ...
					movwf		POSTINC0				; ... and put into the EP0 IN buffer
					decfsz		ByteCounter,f,BANKED
					goto		NextDataByte

					banksel		BD0IBC
					movlw		0x08
					movwf		BD0IBC,BANKED			; set byte count to 8
					movlw		0xC8
					movwf		BD0IST, BANKED			; send packet as DATA1, set UOWN bit
					break
				default
					bsf			USB_error_flags, 0, BANKED	; set Request Error flag
			ends
			return

ProcessInToken
			banksel		USB_USTAT
			movf		USB_USTAT, W, BANKED
			andlw		0x18		; extract the EP bits
			select
				case EP0
					movf		USB_dev_req, W, BANKED
					select
						case SET_ADDRESS
							movf		USB_address_pending, W, BANKED
							movwf		UADDR, ACCESS
							select
								case 0
									movlw		DEFAULT_STATE
									movwf		USB_USWSTAT, BANKED
									break
								default
									movlw		ADDRESS_STATE
									movwf		USB_USWSTAT, BANKED
							ends
							break
						case GET_DESCRIPTOR
							call		SendDescriptorPacket
							break
					ends
					break
				case EP1
					break
				case EP2
					break
			ends
			return

ProcessOutToken
			banksel		USB_USTAT
			movf		USB_USTAT, W, BANKED
			andlw		0x18		; extract the EP bits
			select
				case EP0
					movf		USB_dev_req, W, BANKED
					;SET_BUFFER unimplemented (buffer transfer from PC to this device)
					;Not required for this device, but can be used in other projects
					banksel		BD0OBC
					movlw		MAX_PACKET_SIZE
					movwf		BD0OBC, BANKED
					movlw		0x88
					movwf		BD0OST, BANKED
					clrf		BD0IBC, BANKED		; set byte count to 0
					movlw		0xC8
					movwf		BD0IST, BANKED		; send packet as DATA1, set UOWN bit
					break
				case EP1
					movlw		NO_REQUEST
					break
				case EP2
					break
			ends
			return

SendDescriptorPacket
			banksel		USB_bytes_left
			ifl USB_bytes_left, <, MAX_PACKET_SIZE
				movlw		NO_REQUEST
				movwf		USB_dev_req, BANKED		; sending a short packet, so clear device request
				movf		USB_bytes_left, W, BANKED
			otherwise
				movlw		MAX_PACKET_SIZE
			endi
			subwf		USB_bytes_left, F, BANKED
			movwf		USB_packet_length, BANKED
			banksel		BD0IBC
			movwf		BD0IBC, BANKED			; set EP0 IN byte count with packet size
			movf		BD0IAH, W, BANKED		; put EP0 IN buffer pointer...
			movwf		FSR0H, ACCESS
			movf		BD0IAL, W, BANKED
			movwf		FSR0L, ACCESS			; ...into FSR0
			banksel		USB_loop_index
			forlf USB_loop_index, 1, USB_packet_length
				call		Descriptor			; get next byte of descriptor being sent
				movwf		POSTINC0			; copy to EP0 IN buffer, and increment FSR0
				incf		USB_desc_ptr, F, BANKED	; increment the descriptor pointer
			next USB_loop_index
			banksel		BD0IST
			movlw		0x40
			xorwf		BD0IST, W, BANKED		; toggle the DATA01 bit
			andlw		0x40					; clear the PIDs bits
			iorlw		0x88					; set UOWN and DTS bits
			movwf		BD0IST, BANKED
			return

APPLICATION	code

;Main Code - initialize ports, start USB, and indicate status on the LED
Main
			call		InitPorts
			call		InitUSB					; initialize the USB registers and serial interface engine
								
			movlw   	b'00100000'
			movwf 		PIE2, ACCESS 			; Enable USB interrupt			
			movlw		b'01111111'
			movwf		UIE						; Enable all USB interrupts
			movlw		b'10011111'		
			movwf		UEIE					; Enable all USB error interrupts
			movlw 		b'11000000'
			movwf		INTCON, ACCESS			; Enable all interrupts and all unmasked peripheral interrupts

            repeat  
      			nop 
      			banksel		USB_USWSTAT
			until USB_USWSTAT, ==, ADDRESS_STATE	; ...until the host enumerates the peripheral
			bsf			PORTE,USB_LED				; Indicate enumerated status with constant-on LED

            repeat  
      			nop 
       			banksel		USB_USWSTAT
			until USB_USWSTAT, ==, CONFIG_STATE		; ...until the host configures the peripheral

Busy
			call		Delay					; Blink Status LED Forever to show CONFIGURED status
			bsf			PORTE,USB_LED
			call		Delay
			bcf			PORTE,USB_LED
			goto		Busy

;***** Delay Routine for Blinking LED *****
Delay
			movlw		0x08
			movwf		DelayRegisterC
BigLoop
			movlw		0xFF
			movwf		DelayRegisterA
OuterLoop
			movlw		0xFF
			movwf		DelayRegisterB
InnerLoop
			decfsz		DelayRegisterB,f
			goto		InnerLoop
			decfsz		DelayRegisterA,f
			goto		OuterLoop
			decfsz		DelayRegisterC,f
			goto		BigLoop
			return			

;***** Initialize Ports *****
InitPorts
			clrf		PORTB,A
			clrf		PORTC,A
			clrf		PORTE,A
			movlw		0x0F
			movwf		ADCON1, ACCESS			; set up all Analog I/O pins (Ports A/E) as digital I/O
			movlw		b'11111100'
			movwf		TRISA,A					; RA0-RA1 outputs (Vcc, Pull Up relays)
			movlw		b'11000000'
			movwf		TRISB,A					; RB0-RB5 all outputs (FIFO control lines)
			movlw		b'00111000'
			movwf		TRISC,A					; RC0-RC2, RC6, RC7 all outputs (Trigger control lines)
			movlw		b'11111111'
			movwf		TRISD,A					; RD0-RD7 all inputs (FIFO data)
			movlw		b'11111011'
			movwf		TRISE,A					; RE2 is an output (Status LED)
			return

			end

