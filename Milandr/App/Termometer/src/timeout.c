#include "timeout.h"
#include "stddef.h"

uint32_t GetMicros(void);

void Timeout_Start(Timeout_t *t, uint32_t microseconds)
{
    if (t == NULL) return;
    
    t->start_time = GetMicros();
    t->duration = microseconds;
    t->is_running = true;
}

bool Timeout_IsExpired(Timeout_t *t)
{
    if ( t == NULL || !t->is_running ) {
        return false; 
    }
    
    uint32_t current_time = GetMicros();
    
    if ( current_time - t->start_time >= t->duration ) {
        t->is_running = false; // Таймер отработал, выключаем его
        return true;           // Готов!
    }
    
    return false; // Время еще не прошло
}

void Timeout_Reset(Timeout_t *t)
{
    if (t == NULL) return;
    t->is_running = false;
}