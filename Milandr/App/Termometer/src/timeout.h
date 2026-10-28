#ifndef TIMEOUT_H
#define TIMEOUT_H

#include "stdint.h"
#include "stdbool.h"

/*!
 * \brief Структура неблокирующего таймера
 */
typedef struct {
    uint32_t start_time; //!< Время запуска (в тиках/микросекундах)
    uint32_t duration;   //!< Длительность ожидания
    bool is_running;     //!< Флаг: запущен ли таймер в данный момент
} Timeout_t;

/*!
 * \brief Инициализация/Запуск таймера (в микросекундах)
 */
void Timeout_Start(Timeout_t *t, uint32_t microseconds);

/*!
 * \brief Проверка: Истекло ли время? (Возвращает true, если таймер сработал)
 */
bool Timeout_IsExpired(Timeout_t *t);

/*!
 * \brief Принудительный сброс/остановка таймера
 */
void Timeout_Reset(Timeout_t *t);


#endif //TIMEOUT_H