/*
POSSIVEIS TCCs
--- CONTROLE
incluir acelerometro no loop de controle
controle de posicao(x,y,z) com gps e magnetometro
controle de altitude com barometro e yaw com magnet
controle de posicao indoor, com sonar e infrared
colocar uma camera mirando pro chao para controle de posicao
--- SOFT
usar CI de wifi para que o quad seja um servidor wifi
depois que o quad for um servidor de wifi, fazer um controle remoto no android para controlar
fazer um fpv com osd

falar com o julio pra ver se sai a nova estrutura (tirar fotos dela desmontada, porem pintada)
fazer o analog por DMA
colocar os filtros passas baixas no menu de ajuste dos ganhos
fazer um esquema pra setinha comecar a ir mais rapido se segurar o stick
fazer um buzzer process
fazer uma telinha de entrada (swasthya yoga man)
fazer um doc "user manual" do miniquad
adicionar integrador nos 3 eixos
adaptar acelerometro (juliano: previsao para chegar dia 18 de DEZ)
gravar e ler da flash
gravar parametros
carregar parametros
fazer rotinas I2C com timeout
fazer tudo pro itg3200
*/
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "legacymsp430.h"
#include "msp430f2618.h"

#include "mytypes.h"
#include "misc.h"

#include "color.h"
#include "lcd6100.h"
//#include "itg3200.h" fazer a funcao i2c antes de descomentar
#include "eeprom.h"

#include "usefullibs.h"
#include "menu.h"

#include "miniquad.h"
#include "resources.h"

#include "delay.h"
#include "system.h"
#include "peripherals.h"

//variaveis controle
volatile uchar ControlSample = 0;       // sample time flag - indica a hora de fazer um sample e um controle
int ThrottleFiltered = 0;
// variaveis controle realimentacao
int AccelValue[3] = {0,0,0};
int AccelOffset[3] = {0,0,0};
int GyroValue[3] = {0,0,0};
int GyroOffset[3] = {0,0,0};
// variaveis controle ganhos
int ControlProportionalMul[3];
int ControlProportionalDiv[3];
int ControlIntegralMul[3];
int ControlIntegralDiv[3];
int ControlReferenceMul[3];
int ControlReferenceDiv[3];
// variaveis controle sinais de controle
int ControlRadioResult[3] = {0, 0, 0};
int ControlResult[3] = {0, 0, 0};
int ControlIntegralResult[3] = {0, 0, 0};
// variaveis controle atuadores
int MotorOutput[6] = {MIN_MOTOR, MIN_MOTOR, MIN_MOTOR, MIN_MOTOR, MIN_MOTOR, MIN_MOTOR};
int MotorOutputOld[6] = {MIN_MOTOR, MIN_MOTOR, MIN_MOTOR, MIN_MOTOR, MIN_MOTOR, MIN_MOTOR};
// general purpose
uint16 FlightTime = 0;
// analog graph
byte AnalogChecked;
unsigned char AnalogColours[8] = {BLUE, LIME, RED, YELLOW, CYAN, MAGENTA, SILVER, WHITE}; 
unsigned char AnalogGraph[8][GRAPH_LENGHT];   
// menu
MENU* MainMenu;
MENU* AnalogMenu;
MENU* MotorMenu;
MENU* ControlMenu;
MENU* RadioMenu;
MENU* SensorMenu;
MENU* OptionMenu;
// variavel que guarda o estado da state-machine principal
PROGRAM_STEP ProgStep;

int main(void){
    
    uchar MenuDraw = 0;
    uint16 BatteryColor = LIME;
    uint16 GPCounter = 0;

    ACTION act = ACTION_NONE;
    MENU_RESPONSE resp = RESP_NONE;
    ProgStep = PROCESS_MAIN_MENU;
    
    setup();
    
    menu_draw(MainMenu, 1);
    
    /* iluminacao progressiva do LCD
    while(get_lcd_bright() < 100){
        set_lcd_bright(get_bright_timer_cout() + 1);
        delayus(1500);
    }*/
    
    while(1){
        
        analog_refresh_all();
        adjust_readings();
           
        switch(ProgStep){
            case PROCESS_MAIN_MENU:
                menu_draw(MainMenu, MenuDraw);
                if(delay_poll(DELAY_SECONDS_INDEX)){
                    if(AnalogValue[BATTERY_ACH] < BATTERY_RED){
                        BatteryColor = RED;
                    }      
                    if(BatteryColor != RED){
                        if(AnalogValue[BATTERY_ACH] < BATTERY_YELLOW){
                            BatteryColor = YELLOW;
                        }
                    }
                    //draw batterybar
                    lcd_drawprogressbar(108, 2, 20, 4, BatteryColor, BLACK, ((AnalogValue[BATTERY_ACH]-MIN_BATTERY)/((MAX_BATTERY-MIN_BATTERY)/100)));                                
                    delay_set(DELAY_BATTERY_CHECK, BATTERY_CHECK_TIME);
                    
                    //print flighttime
                    lcd_goto(11,0);
                    printf("%ds", FlightTime);
                }

                MenuDraw = 0;                
                act = get_radio_action();
                resp = menu_process(MainMenu, act);
                switch(resp){                    
                    case RESP_SUBMENU_IN:
                        ProgStep = (PROGRAM_STEP)(MainMenu->SelectedItem);
                        MenuDraw = 1;
                    break;
                    case RESP_EMERGENCY:
                    case RESP_SUBMENU_OUT:
                    case RESP_NONE:          
                    case RESP_BUSY:
                    case RESP_SEL_MIN_LIMIT:
                    case RESP_SEL_MAX_LIMIT:
                    case RESP_MAX_VALUE:
                    case RESP_MIN_VALUE:
                    case RESP_CHECKED:
                    case RESP_UNCHECKED:
                    case RESP_DONE:
                    break;
                }
            break;
                       
            case PROCESS_ANALOG_MENU:
                menu_draw(AnalogMenu, MenuDraw);

                if(MenuDraw == 1){                  
                    
                    lcd_fillrect(10,GRAPH_START-3,107,GRAPH_HEIGHT+6, BLACK);
                    lcd_fillrect(30,GRAPH_START,107-42,GRAPH_HEIGHT,GRAY);
                    
                    LCDBackColor = BLACK;
                    
                    for(GPCounter = 0; GPCounter < 4; GPCounter++){
                        lcd_goto(2,9+GPCounter);
                        LCDForeColor = AnalogColours[GPCounter];
                        printf("%d", GPCounter);
                    }
                    for(GPCounter = 4; GPCounter < 8; GPCounter++){
                        lcd_goto(16,5 + GPCounter);
                        LCDForeColor = AnalogColours[GPCounter];
                        printf("%d", GPCounter);
                    }
                    
                    LCDBackColor = LCD_DEFAULT_BACKCOLOR;
                    LCDForeColor = LCD_DEFAULT_FORECOLOR;
                }
                process_analog_graph();
                MenuDraw = 0;
                                
                menu_refresh(AnalogMenu);
                act = get_radio_action();
                resp = menu_process(AnalogMenu, act);
                switch(resp){
                    case RESP_SUBMENU_OUT:
                        switch(AnalogMenu->SelectedItem){
                            case RETURN_INDEX:
                                ProgStep = PROCESS_MAIN_MENU;
                                MenuDraw = 1;
                            break;
                            
                            case ANALOG_CH0_INDEX:
                                AnalogChecked.BITS.bit0 = 0;
                                analog_graph_clear(0);
                            break;
                            
                            case ANALOG_CH1_INDEX:
                                AnalogChecked.BITS.bit1 = 0;
                                analog_graph_clear(1);
                            break;
                            
                            case ANALOG_CH2_INDEX:
                                AnalogChecked.BITS.bit2 = 0;
                                analog_graph_clear(2);
                            break;
                            
                            case ANALOG_CH3_INDEX:
                                AnalogChecked.BITS.bit3 = 0;
                                analog_graph_clear(3);
                            break;
                            
                            case ANALOG_CH4_INDEX:
                                AnalogChecked.BITS.bit4 = 0;
                                analog_graph_clear(4);
                            break;
                            
                            case ANALOG_CH5_INDEX:
                                AnalogChecked.BITS.bit5 = 0;
                                analog_graph_clear(5);
                            break;
                            
                            case ANALOG_CH6_INDEX:
                                AnalogChecked.BITS.bit6 = 0;
                                analog_graph_clear(6);
                            break;
                            
                            case ANALOG_CH7_INDEX:
                                AnalogChecked.BITS.bit7 = 0;
                                analog_graph_clear(7);
                            break;
                        }
                    break;
                    
                    case RESP_SUBMENU_IN:
                        switch(AnalogMenu->SelectedItem){
                            case CALIBR_INDEX:
                                analog_calibrate_channel(0);
                                analog_calibrate_channel(1);
                                analog_calibrate_channel(2);
                                analog_calibrate_channel(3);
                                analog_calibrate_channel(4);
                                analog_calibrate_channel(5);
                                analog_calibrate_channel(6);
                                analog_calibrate_channel(7);
                            break;
                            
                            case ANALOG_CH0_INDEX:
                                AnalogChecked.BITS.bit0 = 1;
                            break;
                            
                            case ANALOG_CH1_INDEX:
                                AnalogChecked.BITS.bit1 = 1;
                            break;
                            
                            case ANALOG_CH2_INDEX:
                                AnalogChecked.BITS.bit2 = 1;
                            break;
                            
                            case ANALOG_CH3_INDEX:
                                AnalogChecked.BITS.bit3 = 1;
                            break;
                            
                            case ANALOG_CH4_INDEX:
                                AnalogChecked.BITS.bit4 = 1;
                            break;
                            
                            case ANALOG_CH5_INDEX:
                                AnalogChecked.BITS.bit5 = 1;
                            break;
                            
                            case ANALOG_CH6_INDEX:
                                AnalogChecked.BITS.bit6 = 1;
                            break;
                            
                            case ANALOG_CH7_INDEX:
                                AnalogChecked.BITS.bit7 = 1;
                            break;
                        }
                   
                    break;                    
                    
                    case RESP_EMERGENCY:
                    case RESP_NONE:          
                    case RESP_BUSY:
                    case RESP_SEL_MIN_LIMIT:
                    case RESP_SEL_MAX_LIMIT:
                    case RESP_MAX_VALUE:
                    case RESP_MIN_VALUE:
                    case RESP_CHECKED:
                    case RESP_UNCHECKED:
                    case RESP_DONE:
                    break;
                }
            break;
            
            case PROCESS_MOTOR_MENU:
                menu_draw(MotorMenu, MenuDraw);
                if(MenuDraw == 1){
                    set_array(MotorOutput, MIN_MOTOR, 6);
                    ppm_set_all_outputs(MIN_MOTOR);
                }
                MenuDraw = 0;
                ppm_set_outputs(MotorOutput);
                menu_refresh(MotorMenu);
                act = get_radio_action();
                resp = menu_process(MotorMenu, act);
                
                switch(resp){
                    case RESP_SUBMENU_OUT:
                        if(MotorMenu->SelectedItem == RETURN_INDEX){
                            set_array(MotorOutput, MIN_MOTOR, 6);
                            ppm_set_all_outputs(MIN_MOTOR);
                            ProgStep = PROCESS_MAIN_MENU;
                            MenuDraw = 1;
                        }
                        else{ // calibrate index
                            
                        }
                    break;
                    
                    case RESP_EMERGENCY:
                        set_array(MotorOutput, MIN_MOTOR, 6);
                        ppm_set_all_outputs(MIN_MOTOR);
                        screen_flash(RED, 200, 2);
                        ProgStep = PROCESS_MAIN_MENU;
                        MenuDraw = 1;
                    break;
                        
                    case RESP_SUBMENU_IN:
                    case RESP_NONE:          
                    case RESP_BUSY:
                    case RESP_SEL_MIN_LIMIT:
                    case RESP_SEL_MAX_LIMIT:
                    case RESP_MAX_VALUE:
                    case RESP_MIN_VALUE:
                    case RESP_CHECKED:
                    case RESP_UNCHECKED:
                    case RESP_DONE:
                    break;
                }
            break;
            
            case PROCESS_SENSOR_MENU:
                menu_draw(SensorMenu, MenuDraw);
                MenuDraw = 0;
                menu_refresh(SensorMenu);
                act = get_radio_action();
                resp = menu_process(SensorMenu, act);
                switch(resp){
                    case RESP_SUBMENU_OUT:
                        if(SensorMenu->SelectedItem == RETURN_INDEX){
                            ProgStep = PROCESS_MAIN_MENU;
                            MenuDraw = 1;
                        }
                    break;
                    
                    case RESP_EMERGENCY:
                    case RESP_SUBMENU_IN:
                    case RESP_NONE:          
                    case RESP_BUSY:
                    case RESP_SEL_MIN_LIMIT:
                    case RESP_SEL_MAX_LIMIT:
                    case RESP_MAX_VALUE:
                    case RESP_MIN_VALUE:
                    case RESP_CHECKED:
                    case RESP_UNCHECKED:
                    case RESP_DONE:
                    break;
                }
            break;
            
            case PROCESS_RADIO_MENU:
                menu_draw(RadioMenu, MenuDraw);
                MenuDraw = 0;
                menu_refresh(RadioMenu);
                act = get_radio_action();
                resp = menu_process(RadioMenu, act);
                switch(resp){
                    case RESP_SUBMENU_OUT:
                        if(RadioMenu->SelectedItem == RETURN_INDEX){
                            ProgStep = PROCESS_MAIN_MENU;
                            MenuDraw = 1;
                        }
                    break;
                    
                    case RESP_SUBMENU_IN:
                        if(RadioMenu->SelectedItem == CALIBR_INDEX){
                            ppm_calibrate();
                        }                    
                    break;                    
                    
                    case RESP_EMERGENCY:
                    case RESP_NONE:          
                    case RESP_BUSY:
                    case RESP_SEL_MIN_LIMIT:
                    case RESP_SEL_MAX_LIMIT:
                    case RESP_MAX_VALUE:
                    case RESP_MIN_VALUE:
                    case RESP_CHECKED:
                    case RESP_UNCHECKED:
                    case RESP_DONE:
                    break;
                }
            break;
            
            case PROCESS_CONTROL_MENU:
                menu_draw(ControlMenu, MenuDraw);
                MenuDraw = 0;
                menu_refresh(ControlMenu);
                act = get_radio_action();
                resp = menu_process(ControlMenu, act);
                switch(resp){
                    case RESP_SUBMENU_OUT:
                        if(ControlMenu->SelectedItem == RETURN_INDEX){
                            ProgStep = PROCESS_MAIN_MENU;
                            MenuDraw = 1;
                        }
                    break;
                    
                    case RESP_EMERGENCY:
                    case RESP_SUBMENU_IN:
                    case RESP_NONE:          
                    case RESP_BUSY:
                    case RESP_SEL_MIN_LIMIT:
                    case RESP_SEL_MAX_LIMIT:
                    case RESP_MAX_VALUE:
                    case RESP_MIN_VALUE:
                    case RESP_CHECKED:
                    case RESP_UNCHECKED:
                    case RESP_DONE:
                    break;
                }
            break;
            
            case PROCESS_OPTION_MENU:
                menu_draw(OptionMenu, MenuDraw);
                MenuDraw = 0;
                act = get_radio_action();
                resp = menu_process(OptionMenu, act);
                switch(resp){
                    case RESP_SUBMENU_OUT:
                        ProgStep = PROCESS_MAIN_MENU;
                        MenuDraw = 1;
                    break;
                    case RESP_SUBMENU_IN:
                        switch(OptionMenu->SelectedItem){
                            case SAVE_INDEX:
                                if(save_params()){
                                    printf("FAILLED");
                                    delayms(2000);
                                }
                            break;
                            
                            case RESET_INDEX:
                                //reset_defaults();
                            break;
                        }
                        
                    break;
                    case RESP_EMERGENCY:
                    case RESP_NONE:          
                    case RESP_BUSY:
                    case RESP_SEL_MIN_LIMIT:
                    case RESP_SEL_MAX_LIMIT:
                    case RESP_MAX_VALUE:
                    case RESP_MIN_VALUE:
                    case RESP_CHECKED:
                    case RESP_UNCHECKED:
                    case RESP_DONE:
                    break;
                }
            break;
            
            case PROCESS_CONTROL:
                if(TACCR1 > 0){ //apaga tela
                    TACCR1 = TACCR1 - 10;
                    delayus(10000);
                }
                else{
                    if(ControlSample == 1){
                        ControlSample = 0;
                        control_loop();
                    }
                }
                    
                act = get_radio_action();
                switch(act){
                    case ACTION_EMERGENCY:
                        set_array(MotorOutput, MIN_MOTOR, 6);
                        ppm_set_all_outputs(MIN_MOTOR);
                        screen_flash(BLACK,500,2);
                        ProgStep = PROCESS_MAIN_MENU;
                        MenuDraw = 1;
                    break;
                    case ACTION_NONE:
                    case ACTION_UP:      
                    case ACTION_DOWN:     
                    case ACTION_LEFT:     
                    case ACTION_RIGHT:     
                    break;
                }
            break;
        }
    }
}

void setup(void){   
    int i = 100;
    
    vars_init();   

    timer_init(); // inicia com 0 ppm no ESC (ESC fica esperando sinal valido beep --- beep --- beep)
    
    port_init_p1(0xFF, 0xFF, 0x00, 0x00);
    port_init_p2(0x18, 0x00, 0x18, 0x18);
    port_init_p3(0x00, 0x00, 0x00);
    port_init_p4(0x00, 0x7E, 0x7E);
    port_init_p5(0x00, 0x0F, 0x00);
    port_init_p6(0x00, 0x00, 0xFF);   // analog
    
    timer_set_interrupt_A(&delay_process);              // 500us
    timer_set_interrupt_B(&interrupt_400Hz);            // 2500us

    eint(); // habilita interrupt
    
    delayms(400);
        
    // calibra esc
    if(PPMValue[RADIO_THROTTLE_CH] > STICK_LOWER_THRESHOLD){
        while(i--){
            set_array(MotorOutput, PPMValue[RADIO_THROTTLE_CH], 6);
            ppm_set_all_outputs(PPMValue[RADIO_THROTTLE_CH]);
            delayms(80);
        }       
    }
    else{
        set_array(MotorOutput, MIN_MOTOR, 6);
        ppm_set_all_outputs(MIN_MOTOR);
    }
    
    analog_init(); // config registradores        
    analog_calibrate_channel(0);
    analog_calibrate_channel(1);
    analog_calibrate_channel(2);
    analog_calibrate_channel(3);
    analog_calibrate_channel(4);
    analog_calibrate_channel(5);
    analog_calibrate_channel(6);
    analog_calibrate_channel(7);
        
    ppm_calibrate();
        
    menu_init();
    ENABLE_BUZZER();
    BUZZER_ON();
    
    lcd_init(LCDBackColor);
    
    BUZZER_OFF();
}

void process_analog_graph(void){
    int i, k, j, y;
    for(i = 0; i < 8; i++){
        if((1 << i) & AnalogChecked.BYTE){
            // apaga
            for(k = 0; k < (GRAPH_LENGHT - 1); k++){
                lcd_drawpoint(k+GRAPH_OFFSETX, AnalogGraph[i][k] ,GRAY);
                j = AnalogGraph[i][k]; // posicao atual
                y = AnalogGraph[i][k+1]; // proxima posicao
                if(j != y){ // se agora ta mais alto q depois
                    if(j < y){
                        y--; 
                        while(j++ < y){
                            lcd_drawpoint(k+GRAPH_OFFSETX, j ,GRAY);
                        }
                    }
                    else{
                        y++;
                        while(j-- > y){
                            lcd_drawpoint(k+GRAPH_OFFSETX, j ,GRAY);
                        }
                    }
                }
                AnalogGraph[i][k] = AnalogGraph[i][k+1];
            } 
            // apaga ultimo
            lcd_drawpoint(GRAPH_LENGHT - 1 + GRAPH_OFFSETX, AnalogGraph[i][GRAPH_LENGHT - 1] , GRAY);                       
            // insere mais um valor
            AnalogGraph[i][GRAPH_LENGHT - 1] = constrain(GRAPH_OFFSET - ((AnalogValue[i] >> 6) & 0xFF), GRAPH_START, GRAPH_START+GRAPH_HEIGHT);            
            // redesenha
            for(k = 0; k < (GRAPH_LENGHT - 1); k++){
                lcd_drawpoint(k+GRAPH_OFFSETX, AnalogGraph[i][k] ,AnalogColours[i]);
                j = AnalogGraph[i][k]; // posicao atual
                y = AnalogGraph[i][k+1]; // proxima posicao
                if(j != y){ // se agora ta mais alto q depois
                    if(j < y){
                        y--; 
                        while(j++ < y){
                            lcd_drawpoint(k+GRAPH_OFFSETX, j ,AnalogColours[i]);
                        }
                    }
                    else{
                        y++;
                        while(j-- > y){
                            lcd_drawpoint(k+GRAPH_OFFSETX, j ,AnalogColours[i]);
                        }
                    }
                }
            } 
            // desenha ultimo
            lcd_drawpoint(GRAPH_LENGHT - 1 + GRAPH_OFFSETX, AnalogGraph[i][GRAPH_LENGHT - 1] ,AnalogColours[i]);            
        }
    }    
}

void analog_graph_clear(int i){
    int k, j, y;
    // apaga o grafico
    for(k = 0; k < (GRAPH_LENGHT - 1); k++){
        lcd_drawpoint(k+GRAPH_OFFSETX, AnalogGraph[i][k] , GRAY);
        j = AnalogGraph[i][k]; // posicao atual
        y = AnalogGraph[i][k+1]; // proxima posicao
        if(j != y){ // se agora ta mais alto q depois
            if(j < y){
                y--; 
                while(j++ < y){
                    lcd_drawpoint(k+GRAPH_OFFSETX, j , GRAY);
                }
            }
            else{
                y++;
                while(j-- > y){
                    lcd_drawpoint(k+GRAPH_OFFSETX, j , GRAY);
                }
            }
        }
        //AnalogGraph[i][k] = AnalogGraph[i][k+1];
    } 
    // apaga ultimo
    lcd_drawpoint(GRAPH_LENGHT - 1 + GRAPH_OFFSETX, AnalogGraph[i][GRAPH_LENGHT - 1] , GRAY);
}

void menu_init(void){
    MainMenu = menu_create(str_mainmenu, create_item(str_analogmenu, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL), &val_janela_size_large, NULL, NULL, &val_arrow_center, NULL);
    menu_add_item(MainMenu, create_item(str_radiomenu, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));
    menu_add_item(MainMenu, create_item(str_motormenu, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));
    menu_add_item(MainMenu, create_item(str_controlmenu, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));
    menu_add_item(MainMenu, create_item(str_sensormenu, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));
    menu_add_item(MainMenu, create_item(str_optionmenu, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));    
    menu_add_item(MainMenu, create_item(str_letsfly, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));
    // analog menu
    AnalogMenu = menu_create(str_analogmenu, create_item(str_return, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL), &val_janela_size_small, &val_bar_position_center, &val_value_position_right, &val_arrow_center, &val_bar_lenght_medium);
    menu_add_item(AnalogMenu, create_item(str_calibrate, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));
    menu_add_item(AnalogMenu, create_item(str_analogch0, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogValue[0]));
    menu_add_item(AnalogMenu, create_item(str_analogch1, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogValue[1]));
    menu_add_item(AnalogMenu, create_item(str_analogch2, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogValue[2]));
    menu_add_item(AnalogMenu, create_item(str_analogch3, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogValue[3]));
    menu_add_item(AnalogMenu, create_item(str_analogch4, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogValue[4]));
    menu_add_item(AnalogMenu, create_item(str_analogch5, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogValue[5]));    
    menu_add_item(AnalogMenu, create_item(str_analogch6, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogValue[6]));    
    menu_add_item(AnalogMenu, create_item(str_analogch7, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogValue[7]));    
    
    menu_add_item(AnalogMenu, create_item(str_analogof0, ITEMTYPE_VALUE_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogOffset[0]));
    menu_add_item(AnalogMenu, create_item(str_analogof1, ITEMTYPE_VALUE_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogOffset[1]));
    menu_add_item(AnalogMenu, create_item(str_analogof2, ITEMTYPE_VALUE_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogOffset[2]));
    menu_add_item(AnalogMenu, create_item(str_analogof3, ITEMTYPE_VALUE_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogOffset[3]));
    menu_add_item(AnalogMenu, create_item(str_analogof4, ITEMTYPE_VALUE_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogOffset[4]));
    menu_add_item(AnalogMenu, create_item(str_analogof5, ITEMTYPE_VALUE_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogOffset[5]));    
    menu_add_item(AnalogMenu, create_item(str_analogof6, ITEMTYPE_VALUE_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogOffset[6]));    
    menu_add_item(AnalogMenu, create_item(str_analogof7, ITEMTYPE_VALUE_R, &val_zero, &val_max_analog, NULL, (int*)&AnalogOffset[7]));    
    // control menu
    ControlMenu = menu_create(str_controlmenu, create_item(str_return, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL), &val_janela_size_medium, &val_bar_position_center, &val_value_position_right, &val_arrow_center, &val_bar_lenght_medium);
    menu_add_item(ControlMenu, create_item(str_yawpm, ITEMTYPE_VALUE_RW, &val_one, &val_max_control, &val_one, (int*)&ControlProportionalMul[YAW_INDEX]));
    menu_add_item(ControlMenu, create_item(str_pitchpm, ITEMTYPE_VALUE_RW, &val_one, &val_max_control, &val_one, (int*)&ControlProportionalMul[PITCH_INDEX]));
    menu_add_item(ControlMenu, create_item(str_rollpm, ITEMTYPE_VALUE_RW, &val_one, &val_max_control, &val_one, (int*)&ControlProportionalMul[ROLL_INDEX]));        
    menu_add_item(ControlMenu, create_item(str_yawpd, ITEMTYPE_VALUE_RW, &val_zero, &val_max_control, &val_one, (int*)&ControlProportionalDiv[YAW_INDEX]));
    menu_add_item(ControlMenu, create_item(str_pitchpd, ITEMTYPE_VALUE_RW, &val_zero, &val_max_control, &val_one, (int*)&ControlProportionalDiv[PITCH_INDEX]));
    menu_add_item(ControlMenu, create_item(str_rollpd, ITEMTYPE_VALUE_RW, &val_zero, &val_max_control, &val_one, (int*)&ControlProportionalDiv[ROLL_INDEX]));        

    menu_add_item(ControlMenu, create_item(str_yawrefmul, ITEMTYPE_VALUE_RW, &val_zero, &val_max_control, &val_one, (int*)&ControlReferenceMul[YAW_INDEX]));        
    menu_add_item(ControlMenu, create_item(str_pitchrefmul, ITEMTYPE_VALUE_RW, &val_zero, &val_max_control, &val_one, (int*)&ControlReferenceMul[PITCH_INDEX]));        
    menu_add_item(ControlMenu, create_item(str_rollrefmul, ITEMTYPE_VALUE_RW, &val_zero, &val_max_control, &val_one, (int*)&ControlReferenceMul[ROLL_INDEX]));            
    menu_add_item(ControlMenu, create_item(str_yawrefdiv, ITEMTYPE_VALUE_RW, &val_zero, &val_max_control, &val_one, (int*)&ControlReferenceDiv[YAW_INDEX]));        
    menu_add_item(ControlMenu, create_item(str_pitchrefdiv, ITEMTYPE_VALUE_RW, &val_zero, &val_max_control, &val_one, (int*)&ControlReferenceDiv[PITCH_INDEX]));        
    menu_add_item(ControlMenu, create_item(str_rollrefdiv, ITEMTYPE_VALUE_RW, &val_zero, &val_max_control, &val_one, (int*)&ControlReferenceDiv[ROLL_INDEX]));        
    
    // radio menu
    RadioMenu = menu_create(str_radiomenu, create_item(str_return, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL), &val_janela_size_medium, &val_bar_position_center, &val_value_position_right, &val_arrow_center, &val_bar_lenght_medium);
    menu_add_item(RadioMenu, create_item(str_calibrate, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));            
        
    menu_add_item(RadioMenu, create_item(str_radioch0, ITEMTYPE_VALUE_BAR_R, &val_min_radio, &val_max_radio, NULL, (int*)&PPMValue[0]));
    menu_add_item(RadioMenu, create_item(str_radioch1, ITEMTYPE_VALUE_BAR_R, &val_min_radio, &val_max_radio, NULL, (int*)&PPMValue[1]));
    menu_add_item(RadioMenu, create_item(str_radioch2, ITEMTYPE_VALUE_BAR_R, &val_min_radio, &val_max_radio, NULL, (int*)&PPMValue[2]));
    menu_add_item(RadioMenu, create_item(str_radioch3, ITEMTYPE_VALUE_BAR_R, &val_min_radio, &val_max_radio, NULL, (int*)&PPMValue[3]));
    menu_add_item(RadioMenu, create_item(str_radioch4, ITEMTYPE_VALUE_BAR_R, &val_min_radio, &val_max_radio, NULL, (int*)&PPMValue[4]));
    menu_add_item(RadioMenu, create_item(str_radioch5, ITEMTYPE_VALUE_BAR_R, &val_min_radio, &val_max_radio, NULL, (int*)&PPMValue[5]));    
    menu_add_item(RadioMenu, create_item(str_radioch6, ITEMTYPE_VALUE_BAR_R, &val_min_radio, &val_max_radio, NULL, (int*)&PPMValue[6]));    
    menu_add_item(RadioMenu, create_item(str_radioch7, ITEMTYPE_VALUE_BAR_R, &val_min_radio, &val_max_radio, NULL, (int*)&PPMValue[7]));
    
    menu_add_item(RadioMenu, create_item(str_radioof0, ITEMTYPE_VALUE_R, &val_zero, &val_max_radio, NULL, (int*)&PPMOffset[0]));
    menu_add_item(RadioMenu, create_item(str_radioof1, ITEMTYPE_VALUE_R, &val_zero, &val_max_radio, NULL, (int*)&PPMOffset[1]));
    menu_add_item(RadioMenu, create_item(str_radioof2, ITEMTYPE_VALUE_R, &val_zero, &val_max_radio, NULL, (int*)&PPMOffset[2]));
    menu_add_item(RadioMenu, create_item(str_radioof3, ITEMTYPE_VALUE_R, &val_zero, &val_max_radio, NULL, (int*)&PPMOffset[3]));
    menu_add_item(RadioMenu, create_item(str_radioof4, ITEMTYPE_VALUE_R, &val_zero, &val_max_radio, NULL, (int*)&PPMOffset[4]));
    menu_add_item(RadioMenu, create_item(str_radioof5, ITEMTYPE_VALUE_R, &val_zero, &val_max_radio, NULL, (int*)&PPMOffset[5]));    
    menu_add_item(RadioMenu, create_item(str_radioof6, ITEMTYPE_VALUE_R, &val_zero, &val_max_radio, NULL, (int*)&PPMOffset[6]));    
    menu_add_item(RadioMenu, create_item(str_radioof7, ITEMTYPE_VALUE_R, &val_zero, &val_max_radio, NULL, (int*)&PPMOffset[7]));
    // motor menu    
    MotorMenu = menu_create(str_motormenu, create_item(str_return, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL), &val_janela_size_medium, &val_bar_position_center, &val_value_position_right, &val_arrow_center, &val_bar_lenght_medium);
    menu_add_item(MotorMenu, create_item(str_motor1, ITEMTYPE_VALUE_BAR_RW, &val_min_motor, &val_max_motor, &val_int_motor, (int*)&MotorOutput[0]));
    menu_add_item(MotorMenu, create_item(str_motor2, ITEMTYPE_VALUE_BAR_RW, &val_min_motor, &val_max_motor, &val_int_motor, (int*)&MotorOutput[1]));
    menu_add_item(MotorMenu, create_item(str_motor3, ITEMTYPE_VALUE_BAR_RW, &val_min_motor, &val_max_motor, &val_int_motor, (int*)&MotorOutput[2]));
    menu_add_item(MotorMenu, create_item(str_motor4, ITEMTYPE_VALUE_BAR_RW, &val_min_motor, &val_max_motor, &val_int_motor, (int*)&MotorOutput[3]));
    menu_add_item(MotorMenu, create_item(str_motor5, ITEMTYPE_VALUE_BAR_RW, &val_min_motor, &val_max_motor, &val_int_motor, (int*)&MotorOutput[4]));
    menu_add_item(MotorMenu, create_item(str_motor6, ITEMTYPE_VALUE_BAR_RW, &val_min_motor, &val_max_motor, &val_int_motor, (int*)&MotorOutput[5]));    
    menu_add_item(MotorMenu, create_item(str_calibrate, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));    
    // sensor menu
    SensorMenu = menu_create(str_sensormenu, create_item(str_return, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL), &val_janela_size_medium, &val_bar_position_center, &val_value_position_right, &val_arrow_center, &val_bar_lenght_medium);
    menu_add_item(SensorMenu, create_item(str_accelx, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AccelValue[ACCEL_X_INDEX]));
    menu_add_item(SensorMenu, create_item(str_accely, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AccelValue[ACCEL_Y_INDEX]));
    menu_add_item(SensorMenu, create_item(str_accelz, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&AccelValue[ACCEL_Z_INDEX]));
    menu_add_item(SensorMenu, create_item(str_gyrox, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&GyroValue[GYRO_X_INDEX]));
    menu_add_item(SensorMenu, create_item(str_gyroy, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&GyroValue[GYRO_Y_INDEX]));
    menu_add_item(SensorMenu, create_item(str_gyroz, ITEMTYPE_VALUE_BAR_R, &val_zero, &val_max_analog, NULL, (int*)&GyroValue[GYRO_Z_INDEX]));    
    // option menu
    OptionMenu = menu_create(str_optionmenu, create_item(str_return, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL), &val_janela_size_medium, &val_bar_position_center, &val_value_position_right, &val_arrow_center, &val_bar_lenght_medium);
    menu_add_item(OptionMenu, create_item(str_grava, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));    
    menu_add_item(OptionMenu, create_item(str_rstdefault, ITEMTYPE_SUBMENU, NULL, NULL, NULL, NULL));    
}

ACTION get_radio_action(void){
    // highest priority
    if(PPMValue[5] > STICK_UPPER_THRESHOLD){
        return ACTION_EMERGENCY;
    }
        
    if(PPMValue[RADIO_ROLL_CH] > STICK_UPPER_THRESHOLD){
        return ACTION_RIGHT;
    }
    if(PPMValue[RADIO_ROLL_CH] < STICK_LOWER_THRESHOLD){
        return ACTION_LEFT;
    }
    if(PPMValue[RADIO_PITCH_CH] < STICK_LOWER_THRESHOLD){
        return ACTION_UP;
    }
    if(PPMValue[RADIO_PITCH_CH] > STICK_UPPER_THRESHOLD){
        return ACTION_DOWN;
    }
    return ACTION_NONE;
}

void screen_flash(int Color, int interval, int times){
    // iluminacao progressiva do LCD
    while(times--){    
        while(TACCR1 > 0){
            TACCR1 = TACCR1 - 1;
            delayus(interval);
        }
        if(Color != 256){
            lcd_clear(Color);
        }
        while(TACCR1 < LCD_MAX_BRIGHT){
            TACCR1 = TACCR1 + 1;
            delayus(interval);
        }
    }
    while(TACCR1 < LCD_MAX_BRIGHT){
        TACCR1 = TACCR1 + 1;
        delayus(interval);
    }
}


#ifdef TEST_LOOP_PERIOD
uint16 tempoloop = 0;
uint16 tempoloop2 = 0;
#endif // TEST_LOOP_PERIOD

inline void control_loop(void){
    if(PPMValue[RADIO_THROTTLE_CH] > STICK_LOWER_THRESHOLD){
        
        ThrottleFiltered = (ThrottleFiltered * 7 + PPMValue[RADIO_THROTTLE_CH]) >> 3;
        
        // calcula a proporcao do gyro
        ControlResult[ROLL_INDEX] = ((GyroValue[ROLL_INDEX] - AnalogOffset[GYRO_ROLL_ACH]) * ControlProportionalMul[ROLL_INDEX]) >> ControlProportionalDiv[ROLL_INDEX];
        ControlResult[YAW_INDEX] = ((GyroValue[YAW_INDEX] - AnalogOffset[GYRO_YAW_ACH]) * ControlProportionalMul[YAW_INDEX]) >> ControlProportionalDiv[YAW_INDEX];
        ControlResult[PITCH_INDEX] = ((GyroValue[PITCH_INDEX] - AnalogOffset[GYRO_PITCH_ACH]) * ControlProportionalMul[PITCH_INDEX]) >> ControlProportionalDiv[PITCH_INDEX];
        // calcula a influencia do comando do radio
        ControlRadioResult[ROLL_INDEX] = ((PPMValue[RADIO_ROLL_CH] - PPMOffset[RADIO_ROLL_CH]) * ControlReferenceMul[ROLL_INDEX]) >> ControlReferenceDiv[ROLL_INDEX]; 
        ControlRadioResult[PITCH_INDEX] = ((PPMValue[RADIO_PITCH_CH] - PPMOffset[RADIO_PITCH_CH]) * ControlReferenceMul[PITCH_INDEX]) >> ControlReferenceDiv[PITCH_INDEX];
        ControlRadioResult[YAW_INDEX] = ((PPMValue[RADIO_YAW_CH] - PPMOffset[RADIO_YAW_CH]) * ControlReferenceMul[YAW_INDEX]) >> ControlReferenceDiv[YAW_INDEX];
        // controle yaw integral
        //ControlIntegralResult[YAW_INDEX] = constrain(ControlIntegralResult[YAW_INDEX] + ((GyroValue[YAW_INDEX] - AnalogOffset[GYRO_YAW_ACH]) >> 2), -16384, 16384);
        
        MotorOutput[MOTOR_RIGHT] = ThrottleFiltered;
        MotorOutput[MOTOR_RIGHT] -= ControlRadioResult[ROLL_INDEX];
        MotorOutput[MOTOR_RIGHT] += ControlResult[ROLL_INDEX];
        
        MotorOutput[MOTOR_LEFT] = ThrottleFiltered;
        MotorOutput[MOTOR_LEFT] += ControlRadioResult[ROLL_INDEX];
        MotorOutput[MOTOR_LEFT] -= ControlResult[ROLL_INDEX];
            
        MotorOutput[MOTOR_FRONT] = ThrottleFiltered;
        MotorOutput[MOTOR_FRONT] += ControlRadioResult[PITCH_INDEX];
        MotorOutput[MOTOR_FRONT] += ControlResult[PITCH_INDEX];
        
        MotorOutput[MOTOR_BACK] = ThrottleFiltered;
        MotorOutput[MOTOR_BACK] -= ControlRadioResult[PITCH_INDEX];
        MotorOutput[MOTOR_BACK] -= ControlResult[PITCH_INDEX];
                
        MotorOutput[MOTOR_RIGHT] -= ControlRadioResult[YAW_INDEX];
        MotorOutput[MOTOR_LEFT] -= ControlRadioResult[YAW_INDEX];
//        MotorOutput[MOTOR_RIGHT] -= ControlResult[YAW_INDEX] - (ControlIntegralResult[YAW_INDEX] >> 5);
//        MotorOutput[MOTOR_LEFT] -= ControlResult[YAW_INDEX] - (ControlIntegralResult[YAW_INDEX] >> 5);
        MotorOutput[MOTOR_RIGHT] -= ControlResult[YAW_INDEX];
        MotorOutput[MOTOR_LEFT] -= ControlResult[YAW_INDEX];

        MotorOutput[MOTOR_FRONT] += ControlRadioResult[YAW_INDEX];
        MotorOutput[MOTOR_BACK] += ControlRadioResult[YAW_INDEX];
//        MotorOutput[MOTOR_FRONT] += ControlResult[YAW_INDEX] + (ControlIntegralResult[YAW_INDEX] >> 5);
//        MotorOutput[MOTOR_BACK] += ControlResult[YAW_INDEX] + (ControlIntegralResult[YAW_INDEX] >> 5);
        MotorOutput[MOTOR_FRONT] += ControlResult[YAW_INDEX];
        MotorOutput[MOTOR_BACK] += ControlResult[YAW_INDEX];

        // low pass na saida
        MotorOutput[MOTOR_FRONT] = (MotorOutput[MOTOR_FRONT] + MotorOutputOld[MOTOR_FRONT] * 7) >> 3;
        MotorOutput[MOTOR_LEFT] =  (MotorOutput[MOTOR_LEFT] + MotorOutputOld[MOTOR_LEFT]) >> 1;
        MotorOutput[MOTOR_BACK] = (MotorOutput[MOTOR_BACK] + MotorOutputOld[MOTOR_BACK] * 7) >> 3;
        MotorOutput[MOTOR_RIGHT] = (MotorOutput[MOTOR_RIGHT] + MotorOutputOld[MOTOR_RIGHT]) >> 1; 
        
        ppm_set_outputs(MotorOutput);
        
        // coleta sinal atual pra depois fazer o lowpass
        MotorOutputOld[MOTOR_FRONT] = MotorOutput[MOTOR_FRONT];
        MotorOutputOld[MOTOR_LEFT] = MotorOutput[MOTOR_LEFT];
        MotorOutputOld[MOTOR_BACK] = MotorOutput[MOTOR_BACK];
        MotorOutputOld[MOTOR_RIGHT] = MotorOutput[MOTOR_RIGHT];

        if(delay_poll(DELAY_FLIGHT_TIME)){
            FlightTime++;
            delay_set(DELAY_FLIGHT_TIME, 1000);
        }
    }
    else{
        ControlIntegralResult[YAW_INDEX] = 0;
        set_array(MotorOutput, MIN_MOTOR, 6);
        ppm_set_all_outputs(MIN_MOTOR);
    }
}


// result 1 = fail
// result 0 = success
uchar save_params(void){
    uchar result = 0;
    twobytes VarAux;
    
    i2c_config(EEPROM_I2C_ADDR);
    result = i2c_find_device();
    if(!result){
        
        VarAux.i = 0xAA;
        result |= i2c_write_multiples(ADDR_VALIDATION, VarAux.c, 2);
        delayms(100);
        
        VarAux.ui = FlightTime;
        result |= i2c_write_multiples(ADDR_FLIGHT_TIME, VarAux.c, 2);
        delayms(100);
        
        VarAux.i = ControlProportionalMul[PITCH_INDEX];
        result |= i2c_write_multiples(ADDR_P_PITCH_MUL, VarAux.c, 2);
        delayms(100);
        
        VarAux.i = ControlProportionalMul[ROLL_INDEX];
        result |= i2c_write_multiples(ADDR_P_ROLL_MUL, VarAux.c, 2);
        delayms(100);
        
        VarAux.i = ControlProportionalMul[YAW_INDEX];
        result |= i2c_write_multiples(ADDR_P_YAW_MUL, VarAux.c, 2);
        delayms(100);

        VarAux.i = ControlProportionalDiv[PITCH_INDEX];
        result |= i2c_write_multiples(ADDR_P_PITCH_DIV, VarAux.c, 2);
        delayms(100);

        VarAux.i = ControlProportionalDiv[ROLL_INDEX];
        result |= i2c_write_multiples(ADDR_P_ROLL_DIV, VarAux.c, 2);
        delayms(100);

        VarAux.i = ControlProportionalDiv[YAW_INDEX];
        result |= i2c_write_multiples(ADDR_P_YAW_DIV, VarAux.c, 2);
        delayms(100);
        
        screen_flash(256, 200, 2);
    }
    return result;
}

uchar load_params(void){
    uchar result = 0;
    twobytes VarAux;
    
    i2c_config(EEPROM_I2C_ADDR);
    result = i2c_find_device();
    if(!result){
        i2c_read_multiples(ADDR_VALIDATION, VarAux.c, 2);
        if(VarAux.i == 0xAA){
            delayms(50);
            result |= i2c_read_multiples(ADDR_FLIGHT_TIME, VarAux.c, 2);
            FlightTime = VarAux.i;
            
            delayms(50);
            result |= i2c_read_multiples(ADDR_P_PITCH_MUL, VarAux.c, 2);
            ControlProportionalMul[PITCH_INDEX] = VarAux.i;
            
            delayms(50);
            result |= i2c_read_multiples(ADDR_P_ROLL_MUL, VarAux.c, 2);
            ControlProportionalMul[ROLL_INDEX] = VarAux.i;
                        
            delayms(50);
            result |= i2c_read_multiples(ADDR_P_YAW_MUL, VarAux.c, 2);
            ControlProportionalMul[YAW_INDEX] = VarAux.i;
            
            delayms(50);
            result |= i2c_read_multiples(ADDR_P_PITCH_DIV, VarAux.c, 2);
            ControlProportionalDiv[PITCH_INDEX] = VarAux.i;
            
            delayms(50);
            result |= i2c_read_multiples(ADDR_P_ROLL_DIV, VarAux.c, 2);
            ControlProportionalDiv[ROLL_INDEX] = VarAux.i;
            
            delayms(50);
            result |= i2c_read_multiples(ADDR_P_YAW_DIV, VarAux.c, 2);
            ControlProportionalDiv[YAW_INDEX] = VarAux.i;
            
            screen_flash(256, 200, 5);
        }    
        else{
            result |= reset_defaults();
        }    
    }
    return result;
}
    
uchar reset_defaults(void){
    FlightTime = 0;
    
    vars_init();
    
    return (save_params());
    
}

inline void adjust_readings(void){
   // lowpass no gyro, importante
    GyroValue[ROLL_INDEX] = (GyroValue[ROLL_INDEX] + AnalogValue[GYRO_ROLL_ACH]) >> 1;
    GyroValue[PITCH_INDEX] = (GyroValue[PITCH_INDEX] + AnalogValue[GYRO_PITCH_ACH]) >> 1;
    GyroValue[YAW_INDEX] = (GyroValue[YAW_INDEX] + AnalogValue[GYRO_YAW_ACH]) >> 1;
    
    // fazer o lowpass do accel tbm
    AccelValue[ROLL_INDEX] = (AccelValue[ROLL_INDEX] + AnalogValue[ACCELX_ACH]) >> 1;
    AccelValue[PITCH_INDEX] = (AccelValue[PITCH_INDEX] + AnalogValue[ACCELY_ACH]) >> 1;
    AccelValue[YAW_INDEX] = (AccelValue[YAW_INDEX] + AnalogValue[ACCELY_ACH]) >> 1;
    
}

void vars_init(void){
    uint16 i, j;
    // proporcional
    ControlProportionalMul[YAW_INDEX] = YAW_PROPORTIONAL_MUL;
    ControlProportionalMul[PITCH_INDEX] = PITCH_PROPORTIONAL_MUL;
    ControlProportionalMul[ROLL_INDEX] = ROLL_PROPORTIONAL_MUL;
    ControlProportionalDiv[YAW_INDEX] = YAW_PROPORTIONAL_DIV;  
    ControlProportionalDiv[PITCH_INDEX] = PITCH_PROPORTIONAL_DIV;
    ControlProportionalDiv[ROLL_INDEX] = ROLL_PROPORTIONAL_DIV; 
    // integral
    ControlIntegralMul[YAW_INDEX] = YAW_INTEGRAL_MUL;  
    ControlIntegralMul[PITCH_INDEX] = PITCH_INTEGRAL_MUL;
    ControlIntegralMul[ROLL_INDEX] = ROLL_INTEGRAL_MUL;
    ControlIntegralDiv[YAW_INDEX] = YAW_INTEGRAL_MUL;  
    ControlIntegralDiv[PITCH_INDEX] = PITCH_INTEGRAL_MUL;
    ControlIntegralDiv[ROLL_INDEX] = ROLL_INTEGRAL_DIV; 
    // porportional radio
    ControlReferenceMul[YAW_INDEX] = YAW_REF_MUL;  
    ControlReferenceMul[PITCH_INDEX] = PITCH_REF_MUL;
    ControlReferenceMul[ROLL_INDEX] = ROLL_REF_MUL; 
    ControlReferenceDiv[YAW_INDEX] = YAW_REF_DIV;  
    ControlReferenceDiv[PITCH_INDEX] = PITCH_REF_DIV;
    ControlReferenceDiv[ROLL_INDEX] = ROLL_REF_DIV; 
    
    // outras
    AnalogChecked.BYTE = 0x00;
    for(i = 0; i < 8; i++){
        for(j = 0; j < GRAPH_LENGHT; j++){
            AnalogGraph[i][j] = 90;
        }
    }
}

void interrupt_400Hz(void){
    ControlSample = 1;  
}
