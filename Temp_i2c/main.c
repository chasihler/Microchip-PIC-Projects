//    Send_I2C_Data(word_address);            // send word address
    Send_I2C_StartBit();                    // send start bit
    LATCbits.LATC0 = 0;
    Send_i2c_CByte_R();                  // send control byte with R/W bit set high
    LATCbits.LATC1 = 0;
    while (SSPCON2bits.ACKSTAT)
    {
    LATAbits.LATA0 = 0;                 //something
    }
    //incoming_data = Read_I2C_Data();        // now we read the data coming back from the eeprom
    Send_I2C_NAK();                         // send a the NAK to tell the eeprom we don't want any more data
    Send_I2C_StopBit();                     // and then send the stop bit
    LATAbits.LATA2 = 0;

    
