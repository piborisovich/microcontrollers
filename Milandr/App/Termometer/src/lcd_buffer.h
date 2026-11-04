#ifndef LCD_BUFFER_H
#define LCD_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_BUFFER_SIZE 64 // Размер буфера

/*
 * \brief Тип элемента в буфере: команда или данные (символ)
 */
typedef enum {
    LCD_TYPE_COMMAND,
    LCD_TYPE_DATA
} lcd_msg_type_t;

typedef struct {
    uint8_t payload;         // Сам байт данных (код символа или команды)
    lcd_msg_type_t msg_type; // Тип: команда или данные
} lcd_packet_t;

typedef struct {
    lcd_packet_t data[LCD_BUFFER_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} lcd_fifo_t;

// Функции работы с буфером
void LCD_Buffer_Init(lcd_fifo_t *f);
bool LCD_Buffer_Put(lcd_fifo_t *f, lcd_packet_t packet);
bool LCD_Buffer_Get(lcd_fifo_t *f, lcd_packet_t *packet);
bool LCD_Buffer_IsEmpty(lcd_fifo_t *f);

#endif //LCD_BUFFER_H