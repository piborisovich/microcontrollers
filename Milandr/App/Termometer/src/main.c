#include <MDR32FxQI_port.h>
#include <MDR32FxQI_rst_clk.h>
#include <MDR32FxQI_eeprom.h>
#include <MDR32FxQI_bkp.h>
#include <MDR32FxQI_timer.h>

#include "lcd.h"
#include "timeout.h"

#define LIGHT_PIN PORT_Pin_9
#define PWR_PIN   PORT_Pin_10

#define  PWR_ON()      PORT_SetBits(MDR_PORTB, PWR_PIN)
#define  PWR_OFF()     PORT_ResetBits(MDR_PORTB, PWR_PIN)
#define  LIGHT_ON()    PORT_SetBits(MDR_PORTB, PORT_Pin_9)
#define  LIGHT_OFF()   PORT_ResetBits(MDR_PORTB, PORT_Pin_9)

#define PWR_SWITCH()   MDR_PORTB->RXTX ^= PORT_Pin_10

volatile uint32_t global_ticks = 0;

enum TIMER_DELAY
{
    DELAY_500MS  = 500000,
    DELAY_1000MS = 1000000
};

typedef struct _Timers {
    uint32_t pwrLedTimer;
} TIMERS;

TIMERS timers = { 0 };
static Timeout_t ring_timeout;

/*!
* \brief Cpu clock initialization
*/
void InitCpuClock(void);

/*!
* \brief GPIO initialization
*/
void InitGpio(void);

/*!
* \brief 
*/
void InitTimers(void);

void LedMain(void);

uint32_t GetMicros(void);
void Delay_us(uint32_t us);

int main()
{
    SystemInit();
    InitCpuClock();
    int startIndex = 0;

    InitGpio();
    InitTimers();
  
    PWR_ON();
    LIGHT_ON();
    
    LCD_Init();
    Timeout_Start(&ring_timeout, 400000);
  
  while (1)
  {
    LedMain();
    LCD_Process();
    
    if ( Timeout_IsExpired(&ring_timeout) ) {
        LCD_ShowRowRing("Welcome!", 0, startIndex++);
        Timeout_Start(&ring_timeout, 400000);
    } 
  }
}

void InitCpuClock(void)
{
    //Resetting clock settings to defaults
    RST_CLK_DeInit();
  
    // Enabling the HSE clock (external crystal, if present on the board)
    RST_CLK_HSEconfig(RST_CLK_HSE_ON);
    
    while (RST_CLK_HSEstatus() != SUCCESS);
        
    //16MHz crystal * 9 = 144MHz
    RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, RST_CLK_CPU_PLLmul9);
    
    //Enable PLL
    RST_CLK_CPU_PLLcmd(ENABLE);
    
    while (RST_CLK_CPU_PLLstatus() != SUCCESS);
    
    //(CPU_C2_SEL = PLLCPUo = 144 MHz)
    RST_CLK_CPU_PLLuse(ENABLE); 
    
    //(CPU_C3_SEL = CPU_C2) 
    RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);
    
    // Сбрасываем делитель HCLK в 1 (без деления частоты)
    MDR_RST_CLK->ETH_CLOCK = (MDR_RST_CLK->ETH_CLOCK & ~0x03) | 0x01;
    
    //Enable EEPROM clock
    RST_CLK_PCLKcmd (RST_CLK_PCLK_EEPROM, ENABLE);
    //EEPROM set latency is 3 because freq<=100 MHz
    EEPROM_SetLatency(EEPROM_Latency_5);
    RST_CLK_PCLKcmd (RST_CLK_PCLK_EEPROM, DISABLE);

    //Set BKP parameters (SelectRI, LOW)
    RST_CLK_PCLKcmd(RST_CLK_PCLK_BKP, ENABLE);
    BKP_DUccMode(BKP_DUcc_over_80MHz);
  
    //set CPU_C3 as CPU clock source (HCLK signal)
    RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);                       
  
    SystemCoreClockUpdate();
}


void InitGpio(void)
{
  PORT_InitTypeDef GPIOInitStruct = {0};
  
  RST_CLK_PCLKcmd (RST_CLK_PCLK_PORTA, ENABLE);
  RST_CLK_PCLKcmd (RST_CLK_PCLK_PORTB, ENABLE);
  
  PORT_DeInit(MDR_PORTB);
  
  PORT_StructInit(&GPIOInitStruct);
  
  GPIOInitStruct.PORT_Pin    = PWR_PIN | LIGHT_PIN;
  GPIOInitStruct.PORT_FUNC   = PORT_FUNC_PORT;
  GPIOInitStruct.PORT_OE     = PORT_OE_OUT;
  GPIOInitStruct.PORT_SPEED  = PORT_SPEED_FAST;
  GPIOInitStruct.PORT_MODE   = PORT_MODE_DIGITAL;
  PORT_Init(MDR_PORTB, &GPIOInitStruct);
}

void InitTimers(void)
{
    TIMER_CntInitTypeDef sTIM_CntInit = { 0 };
  
    /* Enable TIMER1 clock */
    RST_CLK_PCLKcmd( RST_CLK_PCLK_TIMER1, ENABLE );

    //TIMER1
    TIMER_DeInit( MDR_TIMER1 );
  
    /* Set TIM1 divider */
    TIMER_BRGInit(MDR_TIMER1, TIMER_HCLKdiv2); //144 MHz / 2 = 72 MHz
  
    //Init TIM struct
    TIMER_CntStructInit(&sTIM_CntInit);
    
    /* Initializes the TIMERx Counter */
    sTIM_CntInit.TIMER_Prescaler   = 71;         //CLK = TIMER_CLK/(TIMER_Prescaler + 1) = 72 MHz/ 71 + 1  = 1  MHz (1 мкс)
    sTIM_CntInit.TIMER_Period      = 0xFFFFFFFF; //ARR register
    sTIM_CntInit.TIMER_CounterMode = TIMER_CntMode_ClkFixedDir;
    sTIM_CntInit.TIMER_CounterDirection = TIMER_CntDir_Up;
    sTIM_CntInit.TIMER_IniCounter = 0; //Initial counter value

    TIMER_CntInit(MDR_TIMER1,&sTIM_CntInit);
    
    TIMER_Cmd(MDR_TIMER1, ENABLE);
}

void LedMain(void)
{
    if ( GetMicros() - timers.pwrLedTimer >= DELAY_1000MS ) {
        timers.pwrLedTimer = GetMicros();
        PWR_SWITCH();
    } 
}

uint32_t GetMicros(void)
{
    return MDR_TIMER1->CNT;
}

// Задержка в микросекундах
void Delay_us(uint32_t us)
{
    uint32_t start = GetMicros();
    while ( ( GetMicros() - start ) < us ); // Ожидание
}
