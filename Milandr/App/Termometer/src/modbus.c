#include "modbus.h"
#include "timeout.h"

#include <MDR32FxQI_port.h>
#include <MDR32FxQI_uart.h>

#include <stddef.h>

#define MODBUS_TIMEOUT_US  100000    // Таймаут ожидания ответа от датчика
#define DEV_POOL_INTERVAL_US  100000 // Как часто опрашивать датчики
#define DRIVER_ON_MAX_INTERVAL_US 6  //Время включения драйвера RS-485

#define RX_BUF_SIZE 256
#define TX_BUF_SIZE 8

// Локальные переменные менеджера
static ModbusState_t sm_state = MB_STATE_IDLE;
static uint16_t current_target_idx = 0;

typedef struct
{
    uint8_t tx_buf[TX_BUF_SIZE];
    uint8_t tx_cnt;
    uint8_t tx_len;
    uint8_t rx_buf[RX_BUF_SIZE]; // Вх. буфер UART
    uint8_t rx_len;
} UART_Buffers;

UART_Buffers UART_buf = { 0 };

static Timeout_t mb_timeout;

void ModbusManager_Init(void)
{
    current_target_idx = 0;
    Timeout_Start(&mb_timeout, DEV_POOL_INTERVAL_US);
}

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

void ModbusManager_Process(ModbusTarget_t *targets, uint16_t target_count)
{
    if (target_count == 0 || targets == NULL) return;

    ModbusTarget_t *current_device = &targets[current_target_idx];

    switch (sm_state) {
        
    case MB_STATE_IDLE:
        // Проверяем, пришло ли время опросить текущее устройство
        if ( Timeout_IsExpired(&mb_timeout) ) {
            // Формируем пакет
            Modbus_CreateRequest(UART_buf.tx_buf, current_device);
            DE_ON();
            sm_state = MB_STATE_WAIT_TRANSMITTER_ENABLE;
                
            Timeout_Start(&mb_timeout, DRIVER_ON_MAX_INTERVAL_US);
        }
        break;
    case MB_STATE_WAIT_TRANSMITTER_ENABLE:
        
        if ( Timeout_IsExpired(&mb_timeout) ) {
            //Запускаем неблокирующую отправку
            UART_buf.tx_cnt = 0;
            UART_buf.rx_len = 0;
            UART_buf.tx_len = 8;
            UART_SendData(MDR_UART2, UART_buf.tx_buf[UART_buf.tx_cnt++]);
            
            sm_state = MB_STATE_WAIT_REPLY;
            
            Timeout_Start(&mb_timeout, MODBUS_TIMEOUT_US);
        }
        break;
        
    case MB_STATE_WAIT_REPLY: 
            
        if ( Timeout_IsExpired(&mb_timeout) ) {
            if ( Modbus_ParseResponse(UART_buf.rx_buf, UART_buf.rx_len, current_device) == 0 ) {
                current_device->is_connected = 1;
            } else {
                current_device->is_connected = 0;
            }
                
            sm_state = MB_STATE_IDLE;
                
            current_target_idx = (current_target_idx + 1) % target_count;
                
            Timeout_Start(&mb_timeout, DEV_POOL_INTERVAL_US);
        }
            
        break;
    }
}

/**
  * @brief  UART2 interrupt handler.
  * @param  None
  * @retval None
  */
void UART2_IRQHandler(void)
{
    if ( UART_GetITStatusMasked(MDR_UART2, UART_IT_TX) == SET )
    {
        if ( UART_buf.tx_cnt < UART_buf.tx_len ) {
            UART_SendData(MDR_UART2, UART_buf.tx_buf[UART_buf.tx_cnt++]);
        } else {
            DE_OFF();
        }
        UART_ClearITPendingBit(MDR_UART2, UART_IT_TX);
    }
    
    if ( UART_GetITStatusMasked(MDR_UART2, UART_IT_RX) == SET )
    {
        UART_buf.rx_buf[UART_buf.rx_len++] = (uint8_t)UART_ReceiveData(MDR_UART2);
        
        UART_ClearITPendingBit(MDR_UART2, UART_IT_RX);
    }
}