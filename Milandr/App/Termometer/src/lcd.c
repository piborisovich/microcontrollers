
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

#define LCD_PORT            MDR_PORTB

#define D0_PIN              PORT_Pin_0
#define D1_PIN              PORT_Pin_1
#define D2_PIN              PORT_Pin_2
#define D3_PIN              PORT_Pin_3
#define D4_PIN              PORT_Pin_4
#define D5_PIN              PORT_Pin_5
#define D6_PIN              PORT_Pin_6
#define D7_PIN              PORT_Pin_7

void Delay_ms(uint32_t ms);
void Delay_us(uint32_t us);


void InitLCD_GPIO(void);
void LCD_PulseEnable(void);
void LCD_Port_Write(uint8_t data); 
void LCD_Command_8Bit(uint8_t data);
void LCD_Char(unsigned char data);

void Init_LCD(void)
{
    InitLCD_GPIO();
    
    //1. Ожидание стабилизации питания после включения (>15 мс)
    Delay_ms(20);
   
    //2. Трижды отправляем команду 0x30 с четко регламентированными паузами
    //  -------------------------------- ---------------------------
    // | RS  | RW  | DB7 | DB6 | DB5 | DB4 | DB3 | DB2 | DB1 | DB0 |
    //  -----------------------------------------------------------
    // |  0  |  0  |  0  |  0  |  1  |  1  |  0  |  0  |  0  |  0  |
    //  -----------------------------------------------------------
    LCD_Command_8Bit(0x30);
    Delay_ms(5); //Пауза более 4.1 мс
      
    LCD_Command_8Bit(0x30);
    Delay_us(100); // Пауза более 100 мкс

    LCD_Command_8Bit(0x30); 
    Delay_us(100);
    
    // --- Базовый сброс завершен, дисплей готов принимать настройки ---
    
    //3. Function Set (Set DL, display line, font):
    //      DL = 1 (8-бит), N = 1 (2 строки), F = 0 ( шрифт 5x8 )
    // Байт: 0b00111000 = 0x38
    //  -------------------------------- ---------------------------
    // | RS  | RW  | DB7 | DB6 | DB5 | DB4 | DB3 | DB2 | DB1 | DB0 |
    //  -----------------------------------------------------------
    // |  0  |  0  |  0  |  0  |  1  | DL  |  N  |  F  |  *  |  *  |
    //  -----------------------------------------------------------
    LCD_Command_8Bit(0x38); 

    // 4. Display Switch (Set display on/off,cursor on/off,
    //    blink on/off): 
    //        D = 0 (выкл. экран, чтобы избежать артефактов при настройке)
    //        C = 0 (выкл. отображение курсора)
    //        B = 0 (выкл. мигание курсора)
    // Байт: 0b00001000 = 0x08
    //  -------------------------------- ---------------------------
    // | RS  | RW  | DB7 | DB6 | DB5 | DB4 | DB3 | DB2 | DB1 | DB0 |
    //  -----------------------------------------------------------
    // |  0  |  0  |  0  |  0  |  0  |  0  |  1  |  D  |  C  |  B  |
    //  -----------------------------------------------------------
    LCD_Command_8Bit(0x08); 

    // 5. Clear Display: Очистка экрана и сброс курсора в левый верхний угол
    // Байт: 0b00000001 = 0x01
    //  -------------------------------- ---------------------------
    // | RS  | RW  | DB7 | DB6 | DB5 | DB4 | DB3 | DB2 | DB1 | DB0 |
    //  -----------------------------------------------------------
    // |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  1  |
    //  -----------------------------------------------------------
    LCD_Command_8Bit(0x01); 
    Delay_ms(2); // Критично! Очистка выполняется долго, ждем не менее 2 мс (>1,64 мс)

    // 6. Input Set (Задать направление движения курсора и экрана (эффект бегущей строки)): 
    //    I/D = 1 (курсор вправо), S = 0 (экран неподвижен)
    // Байт: 0b00000110 = 0x06
    //  -------------------------------- ---------------------------
    // | RS  | RW  | DB7 | DB6 | DB5 | DB4 | DB3 | DB2 | DB1 | DB0 |
    //  -----------------------------------------------------------
    // |  0  |  0  |  0  |  0  |  0  |  0  |  0  |  1  | I/D |  S  |
    //  -----------------------------------------------------------
    LCD_Command_8Bit(0x06); 

    // 7. Финал: Вкл. дисплей (Display Switch):
    //        D = 1 (вкл. экран)
    //        C = 0 (выкл. отображение курсора)
    //        B = 0 (выкл. мигание курсора)
    // Байт: 0b00001100 = 0x0C
    LCD_Command_8Bit(0x0C); 
}





void InitLCD_GPIO(void)
{
    PORT_InitTypeDef GPIOInitStruct = {0};
    PORT_StructInit(&GPIOInitStruct);
  
    //PORTA (PA15 - nOE_BUF)
    GPIOInitStruct.PORT_Pin    = PORT_Pin_15;
    GPIOInitStruct.PORT_FUNC   = PORT_FUNC_PORT;
    GPIOInitStruct.PORT_OE     = PORT_OE_OUT;
    GPIOInitStruct.PORT_SPEED  = PORT_SPEED_FAST;
    GPIOInitStruct.PORT_MODE   = PORT_MODE_DIGITAL;
    PORT_Init(MDR_PORTA, &GPIOInitStruct);
    
    //Выкл. буфер 74LVC4245APW
    BUF_OFF();
    Delay_us(10);
    
    //PORTA (PA5 - RS_LCD, PA6 - R/W_LCD)
    GPIOInitStruct.PORT_Pin    = PORT_Pin_5 | PORT_Pin_6;
    GPIOInitStruct.PORT_FUNC   = PORT_FUNC_PORT;
    GPIOInitStruct.PORT_OE     = PORT_OE_OUT;
    GPIOInitStruct.PORT_SPEED  = PORT_SPEED_FAST;
    GPIOInitStruct.PORT_MODE   = PORT_MODE_DIGITAL;
    PORT_Init(MDR_PORTA, &GPIOInitStruct);
    
    //PORTB (PB0 - D0, PB1 - D1, PB2 - D2, PB3 - D3, PB4 - D4,
    //       PB5 - D5, PB6 - D6, PB7 - D7, PB8 - E_LCD
    GPIOInitStruct.PORT_Pin = DATA_PINS_MASK | PORT_Pin_8;
    PORT_Init(MDR_PORTB, &GPIOInitStruct);
    
      //R/W  в 0
    LCD_WRITE_MODE();
    Delay_us(10);
    //Вкл. буфер 74LVC4245APW
    BUF_ON();
    
    //Биты данных сбрасываем в 0
    PORT_ResetBits(LCD_PORT, DATA_PINS_MASK);
    //E в 0
    LCD_E_RESET();
    //RS в 0
    LCD_COMMAND_MODE();
}

/*!
 * \brief Импульс E
 */
void LCD_PulseEnable(void)
{
    LCD_E_SET();
    Delay_us(1); // Длительность импульса должна быть не менее 450 нс
    LCD_E_RESET();
}

void LCD_Port_Write(uint8_t data)
{
    PORT_ResetBits(LCD_PORT, DATA_PINS_MASK);
    //Устанавливаем биты 1 в нужных позициях
    PORT_SetBits(LCD_PORT, data);
}

void LCD_Command_8Bit(uint8_t data)
{
    LCD_COMMAND_MODE();
    // Очищаем старые значения пинов данных
    PORT_ResetBits(LCD_PORT, DATA_PINS_MASK);
    //Устанавливаем биты 1 в нужных позициях
    PORT_SetBits(LCD_PORT, data);
    
    LCD_PulseEnable();
    
    Delay_us(50); // Время на выполнение большинства стандартных команд дисплеем (40 мкс) + запас
}

void LCD_Char(unsigned char data)
{
    LCD_DATA_MODE(); //RS = 1 для передачи данных (символов)
    
    // 1. Выставляем код символа (ASCII) на всю 8-битную шину DB0-DB7
    LCD_Port_Write(data); 
    
    // 2. Стробирующий импульс E (в точности как у команд)
    LCD_PulseEnable();// Пауза на запись символа в память дисплея
    
    Delay_us(50); // Время на выполнение большинства стандартных команд дисплеем (40 мкс) + запас
}

void LCD_String(char *str)
{
    while (*str) {
        LCD_Char(*str++); // Отправляем текущий символ и переходим к следующему
    }
}