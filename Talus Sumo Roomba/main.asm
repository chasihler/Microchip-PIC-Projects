title "Roomba System 1"
#define _version="0.01"
;-------------------------------------------
;
; Roomba Sumo Bot System 1
; by Charles M Douvier
;
;	Software History
;	v 0.01 	LED ON, board check
;	v 0.02	pulse counter
;
;--------------------------------------------

list p=16F628
#include p16F628.inc
;Macro
	__CONFIG _CP_OFF & _WDT_OFF & _XT_OSC & _BODEN_OFF
;
org	0

Mainline: 	
	goto MainCode:

org 4
Int:

MainCode:
;goodies go here.

end