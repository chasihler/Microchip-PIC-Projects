
#include "p16f916.inc"

    __CONFIG _FCMEN_ON & _IESO_OFF & _CP_OFF & _WDT_OFF & _BOD_OFF & _MCLRE_ON & _PWRTE_ON & _HS_OSC


    org 0
    NOP
    NOP
    goto START

START
    nop
    END