/* 
 * File:   keybrd_main.c
 * Author: CHAS
 *
 * Created on May 5, 2014, 8:37 PM
 */

#include <stdio.h>
#include <stdlib.h>

/*
 * 
 */



int count;                      //pin number called
int nokey;                      //1 = no keypressed
char key;                       //ASCII char pressed
char noncharkey;                //no chacter key selected
                                //see table


int main(int argc, char** argv) {

    nokey = 1;                  //start with nokey pressed is true

   //first key in last row scan wins the keypress on double keys 
    
   //Row 1 Output On
   switch(count);
   {
       case 1:  key = ('A'); nokey = 0; break;
       case 2:  key = ('A'); nokey = 0; break;
       case 3:  key = ('A'); nokey = 0; break;
       case 4:  key = ('A'); nokey = 0; break;
       case 5:  key = ('A'); nokey = 0; break;
       case 6:  key = ('A'); nokey = 0; break;
       case 7:  key = ('A'); nokey = 0; break;
       case 8:  key = ('A'); nokey = 0; break;
   }
   //Row 1 Output Off
   
    return (EXIT_SUCCESS);
}

