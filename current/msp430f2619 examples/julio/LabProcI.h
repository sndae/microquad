//
// ****************************************************************
// **                                                            **
// **  LABORATORIO DE PROCESSADORES I (2010/I)                   **
// **                                                            **
// **  Esta biblioteca implementa as rotinas de configura��o     **
// **  do MSP430.                                                **
// **                                                            **
// **                                    (C) by JCLima, 2010/1   **
// **                                                            **
// ****************************************************************
//
#include <msp430x261x.h>

//
// CONFIGURA��O DA PORTA P1 (SOMENTE I/O)
//  
//    0    0     0     0     0     0     0     0    <---- P1SEL: 0x00
//    1    1     1     1     0     0     0     0    <---- P1DIR: 0xf0
// +----+-----+-----+-----+-----+-----+-----+-----+
// | CS | CLK | DIN | RST | SW4 | SW3 | SW2 | SW1 | P1
// +----+-----+-----+-----+-----+-----+-----+-----+
// |                      |                       |
//  \---------v----------/ \----------v----------/
//      DISPLAY GRAFICO            TECLADO
//
//
#define p1_como_io      0x00            // Para P1SEL
#define p1_disp_teclado 0xf0            // Para P1DIR

#define sw1             port1.in.pin0
#define sw2             port1.in.pin1
#define sw3             port1.in.pin2
#define sw4             port1.in.pin3

// Display Gr�fico Monocrom�tico
//
#define lcd_reset       port1.out.pin4
#define lcd_din         port1.out.pin5
#define lcd_sck         port1.out.pin6
#define lcd_cs          port1.out.pin7

// Display Gr�fico Color
//
#define RESX            port1.out.pin4     
#define SDA             port1.out.pin5     
#define SCLK            port1.out.pin6     
#define CSX             port1.out.pin7     

//
// CONFIGURA��O DA PORTA P2
//
// Esta porta admite v�rias configura��es. Foram definidas tr�s configura��es
// b�sicas:
//           P2 s� como I/O e todos os pinos s�o sa�da (default)
//           P2 s� como I/O e todos os pinos s�o entrada
//           P2 acessando perif�ricos: ACLK
//  
//     0      0      0      0      0      0      0      0    <---- P2SEL: 0x00
//     1      1      1      1      1      1      1      1    <---- P2DIR: 0xff
// +------+------+------+------+------+------+------+------+
// | P2.7 | P2.6 | P2.5 | P2.4 | P2.3 | P2.2 | P2.1 | P2.0 | P2
// +------+------+------+------+------+------+------+------+
//                                                    ACLK
//
#define p2_ver_aclk     0x01    // Para P2SEL
#define p2_como_io      0x00    // Para P2SEL
#define p2_saidas       0xff    // Para P2DIR
#define p2_entradas     0x00    // Para P2DIR

//
// CONFIGURA��O DA PORTA P3
//  
// Esta porta admite v�rias configura��es. Foi definida a configura��o para
// acessar os perif�ricos de I2C e da UCSI configurada como UART.
//
//    0    0     1     1     0       1     1     0    <---- P3SEL: 0x36
//    1    1     0     1     0       1    1/0    1    <---- P3DIR: 0xd5 ou 0xd7
// +----+----+-----+-----+--------+-----+-----+-----+
// | EN | RS | RxD | TxD | INSERT | SCL | SDA | LED | P3
// +----+----+-----+-----+--------+-----+-----+-----+
// |         |           |   |    |           |  |
//  \---v---/ \----v----/    v    \-----v----/   v
//     LCD       UART      SMART      PORTA     LED
//                         CARD        I2C
//
#define p3_uart_i2c     0x36            // Para P3SEL
#define p3_uart         0x30            // Para P3SEL
#define p3_como_io      0x00            // Para P3SEL
#define i2c_entrada     0xd5            // Para P3DIR
#define i2c_saida       0xd7            // Para P3DIR

#define led             port3.out.pin0    
#define sda_in          port3.in.pin1
#define sda_out         port3.out.pin1
#define scl             port3.out.pin2
#define insert          port3.in.pin3
#define txd             port3.out.pin4
#define rxd             port3.in.pin5
#define lcd_rs          port3.out.pin6
#define lcd_en          port3.out.pin7

#define SDA_OUT()       P3DIR = 0xd7    // Fun��o que configura i2c como sa�da
#define SDA_IN()        P3DIR = 0xd5    // Fun��o que configura i2c como entrada

//
// CONFIGURA��O DA PORTA P4 (SOMENTE I/O)
//
// Esta porta admite v�rias configura��es, por�m ser� usada apenas como
// sa�da para controle do barramento de dados do LCD alfanum�rico.
//
//     0      0      0      0      0      0      0      0    <---- P4SEL: 0x00
//     1      1      1      1      1      1      1      1    <---- P4DIR: 0xff
// +------+------+------+------+------+------+------+------+
// | P4.7 | P4.6 | P4.5 | P4.4 | P4.3 | P4.2 | P4.1 | P4.0 | P4
// +------+------+------+------+------+------+------+------+
//
//
#define p4_como_io      0x00    // Para P4SEL
#define p4_saidas       0xff    // Para P4DIR

#define lcd_data        P4OUT


//
// CONFIGURA��O DA PORTA P5
//
// Esta porta admite v�rias configura��es. Foram definidas tr�s configura��es
// b�sicas:
//           P5 s� como I/O e todos os pinos s�o sa�da (default)
//           P5 s� como I/O e todos os pinos s�o entrada
//           P5 acessando perif�ricos: ACLK,SMCLK e MCLK
//
// Obs.: A configura��o "ver clocks" n�o funciona na vers�o atual do PROTEUS!!
//  
//     0      0      0      0      0      0      0      0    <---- P5SEL: 0x00
//     1      1      1      1      1      1      1      1    <---- P5DIR: 0xff
// +------+------+------+------+------+------+------+------+
// | P5.7 | P5.6 | P5.5 | P5.4 | P5.3 | P5.2 | P5.1 | P5.0 | P5
// +------+------+------+------+------+------+------+------+
//          ACLK  SMCLK   MCLK                               <---- CLOCK DO MSP
//    CE    CLK2    CS    CLK1   DATA    DR1   DOUT2   DR2   <---- TRW-24G
//
//
#define p5_ver_clocks   0x70    // Para P5SEL
#define p5_como_io      0x00    // Para P5SEL
#define p5_saidas       0xff    // Para P5DIR
#define p5_entradas     0x00    // Para P5DIR
#define p5_radio        0xb8    // Para P5DIR

#define TRW_dr1         port5.in.pin2
#define TRW_data_in     port5.in.pin3
#define TRW_data_out    port5.out.pin3
#define TRW_clk1        port5.out.pin4
#define TRW_cs          port5.out.pin5
#define TRW_ce          port5.out.pin7

#define TRW_OUT()       P5DIR = 0xb8;
#define TRW_IN()        P5DIR = 0xb0;


//
// CONFIGURA��O DA PORTA P6
//
// Esta porta admite v�rias  configura��es entre  I/Os e sa�das e entradas 
// anal�gicas. A configura��o padr�o vai ajustar a porta como sa�da de IO.
// Nas fun��es de acesso ao A/D e D/A ser� modificado o estado da porta
// para permitir acesso as fun��es anal�gicas.
//
//     0      0      0      0      0      0      0      0    <---- P6SEL: 0x00
//     1      1      1      1      1      1      1      1    <---- P6DIR: 0xff
// +------+------+------+------+------+------+------+------+
// | P6.7 | P6.6 | P6.5 | P6.4 | P6.3 | P6.2 | P6.1 | P6.0 | P6
// +------+------+------+------+------+------+------+------+
//    A7     A6     A5     A4     A3     A2     A1     A0    <---- Perif�rico Principal
//   DAC1   DAC0   DAC1                                      <--+- Perif�rico Secund�rio 
//  SVSIN                                                    <--+
//
//
#define p6_como_io      0x00    // Para P6SEL
#define p6_saidas       0xff    // Para P6DIR
#define p6_entradas     0x00    // Para P6DIR


// Configura��o da CPU... Clocks e IOs
//
void configura_cpu(void)
{
  // Desliga o watch Dog...
  //
  WDTCTL = WDTPW + WDTHOLD; 

  // configura o clock do MSP430F2619
  //
  //  MCLK: 16MHz
  // SMCLK: 16MHz
  //  ACLK: 32KHz
  //
  DCOCTL = 0x77;
  BCSCTL1 = 0x0f;
  BCSCTL2 = 0x08;
  BCSCTL3 = 0x8c;

  // Configura as portas conforme a Placa de Laborat�rio 2010/I
  //
  //
  P1SEL = p1_como_io;
  P1DIR = p1_disp_teclado;
  P2SEL = p2_como_io;
  P2DIR = p2_saidas;
  P3SEL = p3_uart_i2c;
  P3DIR = i2c_saida;
  P4SEL = p4_como_io;
  P4DIR = p4_saidas;
  P5SEL = p5_como_io;
  P5DIR = p5_radio;
  P6SEL = p6_como_io;
  P6DIR = p6_saidas;

  // Apaga o LED...
  //
  led = 1;
}

