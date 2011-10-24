#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <msp430x2619.h>
#include "lcd2112.h"
#include "uart0.h"


// Declara��o de pinos...
//
#define led         port3.out.pin0

#define sw1         port1.in.pin0
#define sw2         port1.in.pin1
#define sw3         port1.in.pin2
#define sw4         port1.in.pin3

#define sda_in      port3.in.pin1
#define sda_out     port3.out.pin1
#define scl         port3.out.pin2

#define dataout()   P3DIR = 0xd7
#define datain()    P3DIR = 0xd5


// Defini��es...
//
#define             no_lcd      0
#define             na_serial   1

#define             DS1338      0xd0
#define             TMP275      0x90
#define             MEM_24LC    0xa2
#define             SMART_CARD  0xa0

#define             i2c_TX      1
#define             i2c_RX      0
#define             speed_400   40
#define             speed_100   160

#define             ACK         0


// Constantes e Vari�veis...
//
const unsigned char hexadecimal[16] = {"0123456789ABCDEF"};
unsigned char aonde = no_lcd;


// Prot�tipos...
//
int putchar(int c);

void cpu_config(void);
void delayus(unsigned int tempo);
void delayms(unsigned int tempo);

unsigned char readkeyboard(void);
unsigned char waitpress(void);
void waitrelease(void);
void waitkey(unsigned char key);
unsigned char waitanykey(void);

void i2c_config(unsigned char slave_add, unsigned char speed);
void i2c_setmode(unsigned char mode);
void i2c_setslave(unsigned char slave_add);
unsigned char i2c_write(unsigned char *dado, unsigned char length);
unsigned char i2c_read(unsigned char *dado, unsigned char length);

unsigned char find_device(unsigned char address);


// M A I N
//
int main (void)
{
  unsigned char i, buff[80], buff_rx[80];

  WDTCTL = WDTPW + WDTHOLD; 
  cpu_config();
  lcd_initmodule(nao_virado);
  config_serial(baud_115200, mod_115200);
  i2c_config(SMART_CARD, speed_100);

  printf("<Primitivas i2c>\r\n");  
  if(find_device(SMART_CARD) == ACK)
  {
    printf("SMART ok...\r\n"); 
    strcpy(buff, " JULIO");
    // Inicializa Endere�o...
    buff[0] = 0;
    if(i2c_write(buff, 6) == ACK) printf("Escrita ok\r\n"); else printf("Escrita nok\r\n");  
    // Aguarda processo de grava��o!!!
    delayms(10);

//    strcpy(buff, " CESAR");
    // Inicializa Endere�o...
//    buff[0] = 0x10;
//    if(i2c_write(buff, 6) == ACK) printf("Escrita ok\r\n"); else printf("Escrita nok\r\n");  
    // Aguarda processo de grava��o!!!
//    delayms(10);

    // Inicializa Endere�o...
    buff[0] = 0x10;
    i2c_write(buff, 1);

    // Muda modo de opera��o...
    i2c_setmode(i2c_RX);    
    // Le o smart card...
    if(i2c_read(buff_rx, 5) == ACK) 
    {
      printf("Leitura ok\r\n");
      for(i=0; i<5; i++) printf("%c", buff_rx[i]);
    } else printf("Falha na leitura\r\n");
  } else printf("SMART nao ok");

  while(1)
  {
  }    

  return 0;
}   

// Implementa��o do Printf...
//
int putchar(int c)
{
  if(aonde == no_lcd) lcd_putchar(c); else send_serial(c);
  return 0;
}

// Configura��o da CPU...
//
void cpu_config(void)
{
  // configura o clock para 16MHz...
  DCOCTL = 0x77;
  BCSCTL1 = 0x0f;
  BCSCTL2 = 0x08;
  BCSCTL3 = 0x8c;

  // Configura as portas...
  P1DIR = 0xf0;
  P2DIR = 0x00;
  P3DIR = 0xd5;  
  P3SEL = 0x36;
  P4DIR = 0xff;
  P5DIR = 0x70;
  P5SEL = 0x70;

  // Apaga o led...
  led = 1;
}

// T = 0.2923us * tempo (aproximadamente...)
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
    
    for(i = 0; i < tempo; i++) delayus(3420);
}

// Primitivas de Teclado...
//
unsigned char readkeyboard(void)
{
  unsigned char tecla = 0;
  if(sw1 == 0) tecla = '1';
  if(sw2 == 0) tecla = '2';
  if(sw3 == 0) tecla = '3';
  if(sw4 == 0) tecla = '4';

  return tecla;
}

unsigned char waitpress(void)
{
  unsigned char v = 0;
  while(v == 0) 
  {
    v = readkeyboard();
    delayms(1);
  }

  return v;
}

void waitrelease(void)
{
  while(readkeyboard() != 0) delayms(1);
}

void waitkey(unsigned char key)
{
  while(readkeyboard() != key) delayms(1);
  waitrelease();
}

unsigned char waitanykey(void)
{
  unsigned char v;

  v = waitpress();
  waitrelease();

  return v;
}


// Primitivas do i2c
//
void i2c_config(unsigned char slave_add, unsigned char speed)
{
  // Configurando a porta I2C... (TX Mode)
  //
  UCB0CTL1 = 0xc1;
  UCB0CTL0 = 0x0f;

  // Ajustando velocidade...  
  UCB0BR0 = speed;
  UCB0BR1 = 0;

  // Configurando Slave Address...
  UCB0I2CSA = (slave_add >> 1);
  UCB0CTL1 = 0xc0;

  // For�ando Stop Condition...
  UCB0CTL1 |= UCTR+UCTXSTP;

  // Aguardando o stop condition... (IDLE Mode)
  //
  while(!(UCB0CTL1 & UCTXSTP));  
}

void i2c_setmode(unsigned char mode)
{
  if(mode == i2c_TX) UCB0CTL1 |= UCTR; else UCB0CTL1 &= ~UCTR;
}

void i2c_setslave(unsigned char slave_add)
{
  UCB0I2CSA = (slave_add >> 1);
}





unsigned char i2c_read(unsigned char *dado, unsigned char length)
{
  unsigned char i = 0, status = 0;

  // Start i2c...(em tx mode!!!)
  //
  UCB0CTL1 |= UCTXSTT;
  while(!(UCB0CTL1 & UCTXSTT));

  // Envia a sequencia de caracteres, se houver...
  //
  while((!status) && (i<length))
  {  
    // Testa a ocorrencia de ACK/NACK dos dados...
    //
    if(UCB0STAT & UCNACKIFG) status++;

    // Aguarda at� receber o caracter...
    //
    while(!(IFG2 & UCB0RXIFG)); 

    *(dado+i) = UCB0RXBUF;
    i++;
  }

  // Stop i2c...
  //
  UCB0CTL1 |= UCTXSTP;
  while(!(UCB0CTL1 & UCTXSTP));

  // Testa a ocorrencia de ACK/NACK dos dados...
  //
  while(UCB0STAT & UCBBUSY); 
  if(UCB0STAT & UCNACKIFG) status++;

  return status;
}




unsigned char i2c_write(unsigned char *dado, unsigned char length)
{
  unsigned char i = 0, status = 0;

  // Start i2c...(em tx mode!!!)
  //
  UCB0CTL1 |= UCTR + UCTXSTT;
  while(!(UCB0CTL1 & UCTXSTT));

  // Envia a sequencia de caracteres, se houver...
  //
  while((!status) && (i<length))
  {  
    UCB0TXBUF = *(dado+i);
    i++;

    // Aguarda at� enviar o caracter...
    //
    while(!(IFG2 & UCB0TXIFG)); 

    // Testa a ocorrencia de ACK/NACK dos dados...
    //
    if(UCB0STAT & UCNACKIFG) status++;
  }

  // Stop i2c...
  //
  UCB0CTL1 |= UCTR + UCTXSTP;
  while(!(UCB0CTL1 & UCTXSTP));

  // Testa a ocorrencia de ACK/NACK dos dados...
  //
  while(UCB0STAT & UCBBUSY); 
  if(UCB0STAT & UCNACKIFG) status++;

  return status;
}


// Primitivas do Dispositivo i2c
//
unsigned char find_device(unsigned char address)
{
  unsigned char v, buff[2];

  i2c_setslave(address);
  v = i2c_write(buff, 0);
  return v;
}


