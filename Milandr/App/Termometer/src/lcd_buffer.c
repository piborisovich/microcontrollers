#include "lcd_buffer.h"
#include "stddef.h"

void LCD_Buffer_Init(lcd_fifo_t *f)
{
    if (f == NULL) return;
    f->head = 0;
    f->tail = 0;
    f->count = 0;
}

bool LCD_Buffer_Put(lcd_fifo_t *f, lcd_packet_t packet)
{
    if (f == NULL) return false;
    
    // Проверяем, не переполнен ли буфер
    if (f->count >= LCD_BUFFER_SIZE) {
        return false; 
    }

    // Записываем пакет в позицию head
    f->data[f->tail] = packet;
    
    // Сдвигаем head вперед с учетом кольцевой структуры
    f->tail = (f->tail + 1) % LCD_BUFFER_SIZE;
    f->count++;

    return true;
}

bool LCD_Buffer_Get(lcd_fifo_t *f, lcd_packet_t *packet)
{
    if (f == NULL || packet == NULL) return false;
    
    // Проверяем, есть ли данные в буфере
    if (f->count == 0) {
        return false; 
    }

    // Читаем пакет из позиции tail
    *packet = f->data[f->head];
    
    // Сдвигаем tail вперед с учетом кольцевой структуры
    f->head = (f->head + 1) % LCD_BUFFER_SIZE;
    f->count--;

    return true;
}

bool LCD_Buffer_IsEmpty(lcd_fifo_t *f)
{
    if (f == NULL) return true;
    
    return (f->count == 0);
}