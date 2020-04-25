#include <stdio.h>
#include <stdlib.h>
#include <xc.h>

//config bits
#pragma config OSC=INTOSC, WDTEN=OFF, CP0=OFF  //Internal OSC, No WDT and No code protect
#pragma config STVREN=ON, IESO=OFF, FCMEN=OFF
#pragma config XINST = OFF

unsigned char a=1;
unsigned char b=2;

void main(void) {
    while(1){
        a ++;
        b=b+1;
    }
}
