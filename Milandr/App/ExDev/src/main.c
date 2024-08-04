#include <MDR32F9Qx_port.h>
#include <MDR32F9Qx_rst_clk.h>
#include <MDR32F9Qx_timer.h>

#define LED_ON_PERIOD       200000
#define LED_OFF_PERIOD      1000000

#define LED_PORT_CLK      RST_CLK_PCLK_PORTA
#define LED_PORT          MDR_PORTA
#define LED_PORT_PIN_G    PORT_Pin_4
#define LED_PORT_PIN_R    PORT_Pin_3

enum LED { GREEN, RED };
char current_led;

//void Delay(int waitTicks);
void Timer1_IRQHandler(void);

int main()
{
  PORT_InitTypeDef GPIOInitStruct;
  TIMER_CntInitTypeDef sTIM_CntInit;
  
  RST_CLK_DeInit();
  
  RST_CLK_HSEconfig(RST_CLK_HSE_ON); //enable HSE
  while (RST_CLK_HSEstatus() != SUCCESS);
  
  RST_CLK_CPU_PLLconfig(RST_CLK_CPU_PLLsrcHSEdiv1, RST_CLK_CPU_PLLmul4); //Set HSE as CPU_C1 clock source, CPU_PLL multiplication factor = 1 
 
  //RST_CLK_CPU_PLLcmd(ENABLE);
  //while (RST_CLK_CPU_PLLstatus != SUCCESS);
  
  //RST_CLK_CPU_PLLuse(ENABLE);                                           //Set CPU_C1 as clock source of CPU_C3_SEL (bypass CPU_PLL)
  
  RST_CLK_CPUclkSelection(RST_CLK_CPUclkCPU_C3);                         //set CPU_C3 as CPU clock source (HCLK signal)
  
  //RST_CLK_HSIcmd(DISABLE);
  
  RST_CLK_PCLKcmd (LED_PORT_CLK | RST_CLK_PCLK_TIMER1, ENABLE);
  
 
  //PORT
  PORT_DeInit(LED_PORT);
  PORT_StructInit(&GPIOInitStruct);

  GPIOInitStruct.PORT_Pin        = LED_PORT_PIN_G | LED_PORT_PIN_R;
  GPIOInitStruct.PORT_OE         = PORT_OE_OUT;
  GPIOInitStruct.PORT_SPEED      = PORT_SPEED_FAST;
  GPIOInitStruct.PORT_MODE       = PORT_MODE_DIGITAL;
  
  PORT_Init(LED_PORT, &GPIOInitStruct);
  
  //TIMER
  TIMER_DeInit(MDR_TIMER1);
  
  
  /* TIM1 Configuration:
       TIM1CLK = 4 MHz, Prescaler = 4, TIM1 counter clock = 4 MHz
       TIM1 frequency = TIM1CLK/(TIM1_Period + 1) = 80 Hz

  */
  
  /* Initializes the TIMERx Counter */
    sTIM_CntInit.TIMER_Prescaler        = 0x16;
    sTIM_CntInit.TIMER_Period           = 0xFFFF;
    sTIM_CntInit.TIMER_CounterMode      = TIMER_CntMode_ClkFixedDir;
    sTIM_CntInit.TIMER_CounterDirection = TIMER_CntDir_Up;
    sTIM_CntInit.TIMER_IniCounter = 0;
    //sTIM_CntInit.TIMER_EventSource      = TIMER_EvSrc_TIM_CLK;
    //sTIM_CntInit.TIMER_FilterSampling   = TIMER_FDTS_TIMER_CLK_div_1;
    //sTIM_CntInit.TIMER_ARR_UpdateMode   = TIMER_ARR_Update_Immediately;
    //sTIM_CntInit.TIMER_ETR_FilterConf   = TIMER_Filter_1FF_at_TIMER_CLK;
    //sTIM_CntInit.TIMER_ETR_Prescaler    = TIMER_ETR_Prescaler_None;
    //sTIM_CntInit.TIMER_ETR_Polarity     = TIMER_ETRPolarity_NonInverted;
    //sTIM_CntInit.TIMER_BRK_Polarity     = TIMER_BRKPolarity_NonInverted;*/
    TIMER_CntInit(MDR_TIMER1,&sTIM_CntInit);
    
    /* Enable TIMER1 clock */
    TIMER_BRGInit(MDR_TIMER1, TIMER_HCLKdiv1);
    
    current_led = GREEN;
    
    PORT_ResetBits(LED_PORT, LED_PORT_PIN_R);
    PORT_SetBits(LED_PORT, LED_PORT_PIN_G);
    
    TIMER_ITConfig(MDR_TIMER1, TIMER_STATUS_CNT_ARR, ENABLE);
    NVIC_EnableIRQ(Timer1_IRQn);
    
    /* Enable TIMER1 */
    TIMER_Cmd(MDR_TIMER1, ENABLE);
	
  while (1)
  {
  }      
}

/*void Delay(int waitTicks)
{
  int i;
  for (i = 0; i < waitTicks; i++)
  {
    __NOP();
  }	
}*/

/*******************************************************************************
* Function Name  : Timer1_IRQHandler
* Description    : This function handles Timer1 global interrupt request.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void Timer1_IRQHandler(void)
{
    if (TIMER_GetITStatus(MDR_TIMER1, TIMER_STATUS_CNT_ARR) == SET)
    {
      if (current_led == GREEN) {
        current_led = RED;
        PORT_ResetBits(LED_PORT, LED_PORT_PIN_G);
        PORT_SetBits(LED_PORT, LED_PORT_PIN_R);
      } else {
        current_led = GREEN;
        PORT_ResetBits(LED_PORT, LED_PORT_PIN_R);
        PORT_SetBits(LED_PORT, LED_PORT_PIN_G);
      }
      TIMER_ClearFlag(MDR_TIMER1, TIMER_STATUS_CNT_ARR);
    }
}