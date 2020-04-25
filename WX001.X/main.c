/*
 * File:   main.c
 * Author: Charles M Ihler  Contact at: http://iradan.com
 *
 * Created on April 5m 20020
 *
 * Target Device: 18F47Q10
 *
 * Project: WX 0.01
 *
 *
 * Version:
 * 0.01      Initial testing
 *
 */

#include "mcc_generated_files/mcc.h"

/*
                         Main application
 */
void main(void)
{
    
    volatile uint8_t rxData;
    volatile eusart1_status_t rxStatus;
    
    // Initialize the device
    SYSTEM_Initialize();

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global and Peripheral Interrupts
    // Use the following macros to:

        // Enable the Global Interrupts
        //INTERRUPT_GlobalInterruptEnable();

        // Enable the Peripheral Interrupts
        INTERRUPT_PeripheralInterruptEnable();

    while (1)
    {
        // Add your application code
                    // Logic to echo received data
            if(EUSART1_is_rx_ready())
            {
                rxData = EUSART1_Read();
                if(EUSART1_is_tx_ready())
                {
                    EUSART1_Write(rxData);
                }
            }
    }
}
/**
 End of File
*/