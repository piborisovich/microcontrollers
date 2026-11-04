
#include "lcd.h"
#include "lcd_buffer.h"
#include "timeout.h"
#include "string.h"

#include <MDR32FxQI_port.h>

#define DISPLAY_WIDTH 16

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

//Автомат состояний
typedef enum {
    LCD_STATE_INIT,           //Инициализации железа
    LCD_STATE_CHECK_BUFFER,   // Проверка, работаем ли с буфером
    LCD_STATE_SET_PINS,       // Установка RS, RW и шины данных
    LCD_STATE_PULSE_E_HIGH,   // Поднимаем пин E
    LCD_STATE_PULSE_E_WAIT,   // Ждем длительность импульса E
    LCD_STATE_EXEC_WAIT       // Ждем, пока дисплей выполнит команду (40 мкс, 2 мс...)
} lcd_driver_state_t;

static lcd_driver_state_t driver_state = LCD_STATE_INIT;
static lcd_fifo_t lcd_queue;
static Timeout_t lcd_timeout;
static lcd_packet_t current_packet;

void Delay_us(uint32_t us);

void InitLCD_GPIO(void);
void LCD_PulseEnable(void);
void LCD_Port_Write(uint8_t data); 
void LCD_Command_8Bit(uint8_t data);
void LCD_Char(unsigned char data);

void LCD_Init(void)
{
    LCD_Buffer_Init(&lcd_queue);
    InitLCD_GPIO();
    
    //1. Ожидание стабилизации питания после включения (>15 мс)
    Delay_us(20000);
   
    //2. Трижды отправляем команду 0x30 с четко регламентированными паузами
    //  -------------------------------- ---------------------------
    // | RS  | RW  | DB7 | DB6 | DB5 | DB4 | DB3 | DB2 | DB1 | DB0 |
    //  -----------------------------------------------------------
    // |  0  |  0  |  0  |  0  |  1  |  1  |  0  |  0  |  0  |  0  |
    //  -----------------------------------------------------------
    LCD_Command_8Bit(0x30);
    Delay_us(5000); //Пауза более 4.1 мс
      
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
    Delay_us(2000); // Критично! Очистка выполняется долго, ждем не менее 2 мс (>1,64 мс)

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
    
    driver_state = LCD_STATE_CHECK_BUFFER;
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

void LCD_Process(void)
{
    switch (driver_state) {
    case LCD_STATE_CHECK_BUFFER:
        // Если в буфере что-то есть — забираем элемент на обработку
        if ( LCD_Buffer_Get(&lcd_queue, &current_packet) ) {
          driver_state = LCD_STATE_SET_PINS;
        }
        break;
        
    case LCD_STATE_SET_PINS:
        //Управляем линией RS (команда или данные)
        if ( current_packet.msg_type == LCD_TYPE_COMMAND ) {
            LCD_COMMAND_MODE(); 
        } else {
            LCD_DATA_MODE();
        }
        
        LCD_Port_Write(current_packet.payload); // Выставляем байт на шину данных (DB0..DB7)
        driver_state = LCD_STATE_PULSE_E_HIGH;
        break;
        
    case LCD_STATE_PULSE_E_HIGH:
        LCD_E_SET(); // Дисплей начинает считывать шину
        Timeout_Start(&lcd_timeout, 2); // Строб на 2 микросекунды
        driver_state = LCD_STATE_PULSE_E_WAIT;  
        break;
        
        
    case LCD_STATE_PULSE_E_WAIT:
        if (Timeout_IsExpired(&lcd_timeout)) {
            LCD_E_RESET(); // Зафиксировали данные на стороне дисплея
                
            // Рассчитываем, сколько дисплей будет выполнять эту команду внутри себя
            uint32_t wait_time_us = 40; // Стандартное время для символа (40 мкс)
                
            // Если это команда очистки экрана (0x01) или Home (0x02), ей нужно ~1.64 мс
            if ( current_packet.msg_type == LCD_TYPE_COMMAND && 
                ( current_packet.payload == 0x01 || current_packet.payload == 0x02) ) {
                wait_time_us = 2000; // Ждем 2 миллисекунды (2000 мкс)
            }
                
            Timeout_Start(&lcd_timeout, wait_time_us);
            driver_state = LCD_STATE_EXEC_WAIT;
        }
        break;
    case LCD_STATE_EXEC_WAIT:
        // Ждем, пока медленный чип дисплея выполнит команду
        if ( Timeout_IsExpired(&lcd_timeout) ) {
            // Все готово, возвращаемся проверять, нет ли в буфере следующего символа
                driver_state = LCD_STATE_CHECK_BUFFER; 
        }
        break;
    default:;
    }
}

/*!
 * \brief Импульс E
 */
void LCD_PulseEnable(void)
{
    LCD_E_SET();
    Delay_us(2); // Длительность импульса должна быть не менее 450 нс
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

void LCD_SendCommand(uint8_t cmd)
{
      lcd_packet_t p;
      p.msg_type = LCD_TYPE_COMMAND;
      p.payload = cmd;
      LCD_Buffer_Put(&lcd_queue, p);
}

void LCD_Print(char *str)
{
    lcd_packet_t p;
    p.msg_type = LCD_TYPE_DATA;
    
    while (*str) {
        p.payload = (uint8_t)(*str++);
        LCD_Buffer_Put(&lcd_queue, p); // Добавляем символы в буфер, пока есть место
    }
}

void LCD_SetCursor(uint8_t column, uint8_t row)
{
    uint8_t address = 0;

    // Ограничиваем координаты физическими размерами Waveshare LCD1602
    if (column > 15) column = 15;
    if (row > 1) row = 1;

    // Вычисляем адрес DDRAM в зависимости от строки
    if (row == 0) {
        address = 0x00 + column; // Первая строка начинается с 0x00
    } else {
        address = 0x40 + column; // Вторая строка начинается с 0x40
    }

    // Формируем финальную команду для контроллера(устанавливаем Bit 7)
    uint8_t command = 0x80 | address;

    // Упаковываем в пакет и бросаем в наш неблокирующий кольцевой буфер
    lcd_packet_t p;
    p.msg_type = LCD_TYPE_COMMAND;
    p.payload = command;
    
    LCD_Buffer_Put(&lcd_queue, p);
}

void LCD_String(char *str)
{
    while (*str) {
        LCD_Char(*str++);
    }
}

void LCD_ShowRowRing(const char *text, int row, int startIndex)
{
    char row_buf[DISPLAY_WIDTH + 1];
    
    memset((void*)row_buf, ' ', DISPLAY_WIDTH);
    row_buf[DISPLAY_WIDTH] = 0;
    
    startIndex %= DISPLAY_WIDTH;
        
    while (*text) {
        row_buf[startIndex] = *text++;
        startIndex = (startIndex + 1) % DISPLAY_WIDTH;
    }

    lcd_packet_t p;
    p.msg_type = LCD_TYPE_COMMAND;
    p.payload = 0x01; //Cleae display
    
    LCD_Buffer_Put(&lcd_queue, p);
    
    LCD_Print(row_buf);
}