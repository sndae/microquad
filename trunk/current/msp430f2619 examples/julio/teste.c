#include <msp430x20x3.h>
#include <signal.h>
#include <string.h>

#define verm            port1.out.pin5
#define verde           port1.out.pin4
#define sda_in          port1.in.pin0 
#define sda_out         port1.out.pin0
#define scl             port1.in.pin1

#define datain()        P1DIR = 0x32;      
#define dataout()       P1DIR = 0x33;  


// ****************************************************************
// ** P R O T O T I P O S                                        **
// ****************************************************************
//
void set_DCO(void);
void delayus(unsigned int tempo);
void delayms(unsigned int tempo);


int main(void) 
{	
	set_DCO();	
    dataout();
	verm=1;
    verde=1;
						
	while(1) 
	{
      verm=1;
      verde=1;
      delayms(1000);
      verm=0;
      verde=0;
      delayms(1000);
	}
    return 0;
}


// Ajusta DCO em 8MHz...
//
void set_DCO(void)
{
  DCOCTL = DCO0+DCO1;                        // DCOCTL  = 0x60;
  BCSCTL1 = XT2OFF + RSEL3 + RSEL2 + RSEL0;  // BCSCTL1 = 0x8d;
  BCSCTL2 = 0x00;                            
  BCSCTL3 = 0x00;
}

// T = 3u * tempo (aproximadamente)
//
void delayus(unsigned int tempo)
{
  	int i;
	
	for(i = 0; i < tempo; i++) nop();
}

// Aproximadamente 1ms...
//
void delayms(unsigned int tempo)
{
  	int i;
	
	for(i = 0; i < tempo; i++) delayus(300);
}

