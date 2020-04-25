Fcy=FOSC/2

U1BRG = (Fcy/(16*Baud)-1
=79 / 31.25K
=29 / 9600

BRGH = 0

PDSEL (U1MODE) Parity
STSEL (U1MODE) Stop Bits

Set UARTEN (U1MODE)
Set UTXEN  (U1STA)

if != UTXBFTXREG = newchar;


#define FCY 29100000
#define BAUDRATE 9600
#define BRGVAL ((FCY/BAUDRATE)/16)-1
int main(void)
{
U1MODEbits.STSEL = 0; // 1-stop bit
U1MODEbits.PDSEL = 0; // No Parity, 8-data bits
U1MODEbits.ABAUD = 0; // Autobaud Disabled
U1MODEbits.BRGH = 0; // Low Speed mode
U1BRG = BRGVAL; // BAUD Rate Setting for 9600
U1STAbits.UTXISEL = 0; // Interrupt for every data transfer
IEC0bits.U1TXIE = 1; // Enable UART Transmit interrupt
U1MODEbits.UARTEN = 1; // Enable UART
U1STAbits.UTXEN = 1; // Enable UART Tx
/* wait at least 104 usec (1/9600) before sending first char */
for(i = 0; i < 4160; i++){
Nop();
}
U1TXREG = 'a'; // Transmit one character


© 2008 Microchip Technology Inc.
DS70276C-page 34-21
Section 34. UART (Part II)
UART (Part II)
34
Example 34-3 provides sample code t
hat sets up the UART for reception.
Example 34-3: UART Receiv
ing with Interrupt
#define FCY 29100000
#define BAUDRATE 9600
#define BRGVAL ((FCY/BAUDRATE)/16)-1
int main(void)
{
U1MODEbits.STSEL = 0; // 1-stop bit
U1MODEbits.PDSEL = 0; // No Parity, 8-data bits
U1MODEbits.ABAUD = 0; // Autobaud Disabled
U1MODEbits.BRGH = 0; // Low Speed mode
U1BRG = BRGVAL; // BAUD Rate Setting for 9600
U1STAbits.URXISEL = 0; // Interrupt after a character is received
IEC0bits.U1RXIE = 1; // Enable UART Receive interrupt
U1MODbits.UARTEN = 1; // Enable UART
while(1)
{
}
}
void __attribute__((interrupt, no_auto_psv)) _UxTXInterrupt(void)
{
if(U1STAbits.OERR == 1) {
U1STAbits.OERR = 0; // Clear Overrun Error to receive data
} elseif ((U1STAbits.FERR ==0) && (U1STAbits.PERR ==0) ) {
ReceivedChar = U1RXREG; // Read Data if there is no parity or
framing
// error
}
IFS0bits.U1RXIF = 0; // clear TX interrupt flag
}


