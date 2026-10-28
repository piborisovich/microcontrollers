
#include "lcd.h"

#include <MDR32FxQI_port.h>

//RS_LCD
#define LCD_COMMAND_MODE()  PORT_ResetBits(MDR_PORTA, PORT_Pin_5)
#define LCD_DATA_MODE()     PORT_SetBits(MDR_PORTA, PORT_Pin_5)

//R/W_LCD
#define LCD_WRITE_MODE()    PORT_ResetBits(MDR_PORTA, PORT_Pin_6)
#define LCD_READ_MODE()     PORT_SetBits(MDR_PORTA, PORT_Pin_6)

//nOE_BUF
#define BUF_ON()            PORT_ResetBits(MDR_PORTA, PORT_Pin_15)
#define BUF_OFF()           PORT_SetBits(MDR_PORTA, PORT_Pin_15)

//E_LCD
#define LCD_E_SET()         PORT_SetBits(MDR_PORTB, PORT_Pin_8)
#define LCD_E_RESET()       PORT_ResetBits(MDR_PORTB, PORT_Pin_8)

#define DATA_PINS_MASK      0x00FF

#define D0_PIN              PORT_Pin_0
#define D1_PIN              PORT_Pin_1
#define D2_PIN              PORT_Pin_2
#define D3_PIN              PORT_Pin_3
#define D4_PIN              PORT_Pin_4
#define D5_PIN              PORT_Pin_5
#define D6_PIN              PORT_Pin_6
#define D7_PIN              PORT_Pin_7

void InitLCD_GPIO(void);

void InitLCD_GPIO(void)
{
    PORT_InitTypeDef GPIOInitStruct = {0};
    PORT_StructInit(&GPIOInitStruct);
  
    //PORTA (PA5 - RS_LCD, PA6 - R/W_LCD, PA15 - nOE_BUF)
    GPIOInitStruct.PORT_Pin    = PORT_Pin_5 | PORT_Pin_6 | PORT_Pin_15;
    GPIOInitStruct.PORT_FUNC   = PORT_FUNC_PORT;
    GPIOInitStruct.PORT_OE     = PORT_OE_OUT;
    GPIOInitStruct.PORT_SPEED  = PORT_SPEED_FAST;
    GPIOInitStruct.PORT_MODE   = PORT_MODE_DIGITAL;
    PORT_Init(MDR_PORTA, &GPIOInitStruct);
    
    //PORTB (PB0 - D0, PB1 - D1, PB2 - D2, PB3 - D3, PB4 - D4,
    //       PB5 - D5, PB6 - D6, PB7 - D7, PB8 - E_LCD
    GPIOInitStruct.PORT_Pin = DATA_PINS_MASK | PORT_Pin_8;
    PORT_Init(MDR_PORTB, &GPIOInitStruct);
}