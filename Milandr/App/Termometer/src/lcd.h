#ifndef LCD_H
#define LCD_H

#include <stdint.h>

/*!
 */
void LCD_Init(void); //Блокирующий вызов
void LCD_Process(void);
void LCD_SendCommand(uint8_t cmd);
void LCD_Print(char *str); 
void LCD_SetCursor(uint8_t column, uint8_t row);
void LCD_String(char *str); //Блокирующий вызов
void LCD_ShowRowRing(const char *text, int row, int startIndex);

#endif //LCD_H