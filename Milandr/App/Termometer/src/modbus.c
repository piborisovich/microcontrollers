#include "modbus.h"

void Modbus_CreateRequest(uint8_t *packet, ModbusTarget_t *target)
{
    packet[0] = target->slave_address;
    packet[1] = 0x03; // Код функции
    packet[2] = (target->start_register >> 8) & 0xFF;
    packet[3] = target->start_register & 0xFF;
    packet[4] = (target->register_count >> 8) & 0xFF;
    packet[5] = target->register_count & 0xFF;
    
    uint16_t crc = Modbus_CRC16(packet, 6);
    packet[6] = crc & 0xFF;         // Low byte
    packet[7] = (crc >> 8) & 0xFF;  // High bytes
}

int8_t Modbus_ParseResponse(uint8_t *packet, uint16_t packet_len, ModbusTarget_t *target)
{
    // Проверка минимальной длины (Адрес + Функция + Кол-во байт + Данные(минимум 2) + CRC(2))
    if ( packet_len < 7 ) return -1; 
    
    // Проверка CRC
    uint16_t crc_calc = Modbus_CRC16(packet, packet_len - 2);
    uint16_t crc_received = packet[packet_len - 2] | (packet[packet_len - 1] << 8);
    if (crc_calc != crc_received) return -2; // Ошибка CRC
    
    // Проверка адреса и функции
    if (packet[0] != target->slave_address || packet[1] != 0x03) return -3;
    
    uint8_t byte_count = packet[2];
    if (byte_count != (target->register_count * 2)) return -4; // Несовпадение данных
    
    // Запись данных в буфер устройства (перевод из Big-Endian в Little-Endian МК)
    for (uint16_t i = 0; i < target->register_count; ++i) {
        target->dest_buffer[i] = (packet[3 + i*2] << 8) | packet[4 + i*2];
    }
    
    return 0; // Успешно
}

// Табличный или алгоритмический CRC16 (Modbus)
uint16_t Modbus_CRC16(uint8_t *buffer, uint16_t length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++) {
        crc ^= buffer[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}