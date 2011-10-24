#include <msp430x2619.h>


void config_serial(unsigned char baud, unsigned char modul)
{
  UCA0CTL1 = 0x81;
  UCA0CTL0 = 0x08;
  UCA0BR0 = baud;
  UCA0BR1 = 0;
  UCA0MCTL = modul;
  UCA0CTL1 = 0x80;
}

unsigned char receive_serial(void)
{
  while(!(IFG2 & UCA0RXIFG));
  return UCA0RXBUF;
}

void send_serial(unsigned char c)
{
  while(!(IFG2 & UCA0TXIFG));
  UCA0TXBUF = c;                    // TX -> RXed character
}


