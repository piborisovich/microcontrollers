#include <MDR32FxQI_port.h>
#include <MDR32FxQI_rst_clk.h>
#include <MDR32FxQI_timer.h>
#include <MDR32FxQI_eeprom.h>
#include <MDR32FxQI_bkp.h>
#include <MDR32FxQI_power.h>
#include <stdint.h>

#define RELAY_SET_PIN   PORT_Pin_0
#define RELAY_RESET_PIN PORT_Pin_1

#define LED_PORT_PIN_G    PORT_Pin_4
#define LED_PORT_PIN_R    PORT_Pin_3

#define RELAY_SET_PIN_ON()  PORT_SetBits(MDR_PORTA, RELAY_SET_PIN);
#define RELAY_SET_PIN_OFF() PORT_ResetBits(MDR_PORTA, RELAY_SET_PIN);

#define RELAY_RESET_PIN_ON()  PORT_SetBits(MDR_PORTA, RELAY_RESET_PIN);
#define RELAY_RESET_PIN_OFF() PORT_ResetBits(MDR_PORTA, RELAY_RESET_PIN);

#define RELAY_SET() RELAY_SET_PIN_ON(); \
                    TIMER_Cmd(MDR_TIMER1, ENABLE);

#define RELAY_RESET() RELAY_RESET_PIN_ON(); \
                      TIMER_Cmd(MDR_TIMER1, ENABLE);

#define SWITCH_TO_RED() PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_G); \
                        PORT_SetBits(MDR_PORTA, LED_PORT_PIN_R);

#define SWITCH_TO_GREEN() PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_R); \
                          PORT_SetBits(MDR_PORTA, LED_PORT_PIN_G);
                          
#define LED_OFF()         PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_R); \
                          PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_G);

enum LED { GREEN, RED };

volatile uint32_t global_ticks = 0;

void Delay(uint32_t wait_ms);

/*!
* \brief Cpu clock initialization
*/
void InitCpuClock(void);

void InitGpio(void);

void InitNVIC(void);

//Init SysTick
void InitTick(void);

void InitTimers(void);

int main()
{
  uint8_t current_led = GREEN;
  
  SystemInit();
  InitCpuClock();
  InitGpio();
  InitTick();
  InitNVIC();
  InitTimers();
  
  RELAY_SET();
  Delay(1000);
  RELAY_RESET();
  
  
    	
  while (1)
  {
    Delay(200);
    if (current_led == GREEN) {
        current_led = RED;
        SWITCH_TO_RED();
    } else {
       current_led = GREEN;
       SWITCH_TO_GREEN();
    }
  }      
}

void Delay(uint32_t wait_ms)
{
  uint32_t ticks = global_ticks;
  
  while ( global_ticks - ticks < wait_ms );
  /*int i = 0, ticks = SystemCoreClock / 1000 * wait_ms;
  for (i = 0; i < ticks; ++i);*/	
}

void InitCpuClock(void)
{
  RST_CLK_DeInit();
  
  RST_CLK_HSEconfig(RST_CLK_HSE_ON); //enable HSE
  
  while (RST_CLK_HSEstatus() != SUCCESS);
  
  // CPU_C1_SEL = HSE_CLK, PLLCPUo = HSE_CLK * 5 =16 MHz * 5 = 80 MHz 
  RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, RST_CLK_CPU_PLLmul5); //Set HSE as CPU_C1 clock source, CPU_PLL multiplication factor = 5 
  
  //Enable PLL
  RST_CLK_CPU_PLLcmd(ENABLE);

  while ( RST_CLK_CPU_PLLstatus() != SUCCESS );
  
   //(CPU_C2_SEL = PLLCPUo = 80 MHz)
  RST_CLK_CPU_PLLuse(ENABLE);                                           //Set PLLCPUo as clock source of CPU_C3_SEL
  
  //(CPU_C3_SEL = CPU_C2) 
  RST_CLK_CPUclkPrescaler(RST_CLK_CPUclkDIV1);
  
  //Enable EEPROM clock
  RST_CLK_PCLKcmd (RST_CLK_PCLK_EEPROM, ENABLE);
  //EEPROM set latency is 3 because freq<=100 MHz
  EEPROM_SetLatency(EEPROM_Latency_3);
  RST_CLK_PCLKcmd (RST_CLK_PCLK_EEPROM, DISABLE);

  //Set BKP parameters (SelectRI, LOW)
  RST_CLK_PCLKcmd(RST_CLK_PCLK_BKP, ENABLE);
  BKP_DUccMode(BKP_DUcc_upto_80MHz); //SelectRI=110 (400 Ohm), LOW=110 (up to 80 MHz)
  
  RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);                       //set CPU_C3 as CPU clock source (HCLK signal)
  
  SystemCoreClockUpdate();
}

void InitNVIC(void)
{
  
}

void InitGpio(void)
{
  PORT_InitTypeDef GPIOInitStruct = {0};
  
  RST_CLK_PCLKcmd (RST_CLK_PCLK_PORTA, ENABLE);
  
  PORT_DeInit(MDR_PORTA);
  PORT_StructInit(&GPIOInitStruct);
  
  GPIOInitStruct.PORT_Pin        = LED_PORT_PIN_G | LED_PORT_PIN_R | RELAY_SET_PIN | RELAY_RESET_PIN;
  GPIOInitStruct.PORT_OE         = PORT_OE_OUT;
  GPIOInitStruct.PORT_SPEED      = PORT_SPEED_FAST;
  GPIOInitStruct.PORT_MODE       = PORT_MODE_DIGITAL;
  PORT_Init(MDR_PORTA, &GPIOInitStruct);
  
  SWITCH_TO_GREEN();
  
  RELAY_RESET_PIN_OFF();
  RELAY_SET_PIN_OFF();
}

void InitTick(void)
{
  SysTick_Config( SystemCoreClock / 1000U );
}

void InitTimers(void)
{
  TIMER_CntInitTypeDef sTIM_CntInit = { 0 };
  
  /* Enable TIMER1 clock */
  RST_CLK_PCLKcmd ( RST_CLK_PCLK_TIMER1, ENABLE);

  //TIMER1
  TIMER_DeInit( MDR_TIMER1 );
  
  /* Set TIM1 divider */
  TIMER_BRGInit(MDR_TIMER1, TIMER_HCLKdiv8); //80 MHz / 8 = 10 MHz
  
  //Init TIM struct
  TIMER_CntStructInit(&sTIM_CntInit);
    
  /* Initializes the TIMERx Counter */
  sTIM_CntInit.TIMER_Prescaler   = 0;   //CLK = TIMER_CLK/(TIMER_Prescaler + 1) = 10 MHz/ 1  = 10  MHz
  sTIM_CntInit.TIMER_Period      = 20000; //ARR register
  sTIM_CntInit.TIMER_CounterMode = TIMER_CntMode_ClkFixedDir;
  sTIM_CntInit.TIMER_CounterDirection = TIMER_CntDir_Up;
  sTIM_CntInit.TIMER_IniCounter = 0; //Initial counter value
  //sTIM_CntInit.TIMER_EventSource      = TIMER_EvSrc_TIM_CLK;
  //sTIM_CntInit.TIMER_FilterSampling   = TIMER_FDTS_TIMER_CLK_div_1;
  //sTIM_CntInit.TIMER_ARR_UpdateMode   = TIMER_ARR_Update_Immediately;
  //sTIM_CntInit.TIMER_ETR_FilterConf   = TIMER_Filter_1FF_at_TIMER_CLK;
  //sTIM_CntInit.TIMER_ETR_Prescaler    = TIMER_ETR_Prescaler_None;
  //sTIM_CntInit.TIMER_ETR_Polarity     = TIMER_ETRPolarity_NonInverted;
  //sTIM_CntInit.TIMER_BRK_Polarity     = TIMER_BRKPolarity_NonInverted;*/
  TIMER_CntInit(MDR_TIMER1,&sTIM_CntInit);
  
  NVIC_EnableIRQ(Timer1_IRQn);
  NVIC_SetPriority(Timer1_IRQn, 0);
  
  TIMER_ITConfig(MDR_TIMER1, TIMER_STATUS_CNT_ARR, ENABLE);
}

/*******************************************************************************
* Function Name  : Timer1_IRQHandler
* Description    : This function handles Timer1 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void Timer1_IRQHandler(void)
{
  if ( TIMER_GetITStatus(MDR_TIMER1, TIMER_STATUS_CNT_ARR) != RESET )
  {
    RELAY_SET_PIN_OFF();
    RELAY_RESET_PIN_OFF();
    TIMER_ClearITPendingBit(MDR_TIMER1, TIMER_STATUS_CNT_ARR);
    TIMER_Cmd(MDR_TIMER1, DISABLE);
  }
}

void SysTick_Handler()
{
  ++global_ticks;
}