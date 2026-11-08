#ifndef _MODBUS_H
#define _MODBUS_H

#include <stdint.h>

// Структура, описывающая одно устройство/запрос
typedef struct {
    uint8_t slave_address;     // Modbus адрес устройства (1-247)
    uint16_t start_register;   // Начальный адрес регистра для функции 0x03
    uint16_t register_count;   // Сколько регистров нужно прочитать
    uint16_t *dest_buffer;     // Указатель на массив, куда сохранить результат
    uint8_t is_connected;      // Флаг успешного опроса (0 - тайм-аут/ошибка, 1 - ОК)
} ModbusTarget_t;

// Интерфейс библиотеки

//Запрос (функция 0x03)
void Modbus_CreateRequest(uint8_t *packet, ModbusTarget_t *target);
//Проверка ответа
int8_t Modbus_ParseResponse(uint8_t *packet, uint16_t packet_len, ModbusTarget_t *target);
//Вычисление CRC16 (полином 0xA001)
uint16_t Modbus_CRC16(uint8_t *buffer, uint16_t length);

#endif //_MODBUS_H