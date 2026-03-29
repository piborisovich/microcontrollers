#include <MDR32FxQI_port.h>
#include <MDR32FxQI_rst_clk.h>
#include <MDR32FxQI_timer.h>
#include <MDR32FxQI_eeprom.h>
#include <MDR32FxQI_bkp.h>
#include <MDR32FxQI_uart.h>
#include <stdint.h>
#include <string.h>

#define RELAY_SET_PIN   PORT_Pin_0
#define RELAY_RESET_PIN PORT_Pin_1

#define LED_PORT_PIN_G    PORT_Pin_4
#define LED_PORT_PIN_R    PORT_Pin_3

#define DE_PIN            PORT_Pin_2

#define RELAY_SET_PIN_ON()  PORT_SetBits(MDR_PORTA, RELAY_SET_PIN)
#define RELAY_SET_PIN_OFF() PORT_ResetBits(MDR_PORTA, RELAY_SET_PIN)

#define RELAY_RESET_PIN_ON()  PORT_SetBits(MDR_PORTA, RELAY_RESET_PIN)
#define RELAY_RESET_PIN_OFF() PORT_ResetBits(MDR_PORTA, RELAY_RESET_PIN)

#define RELAY_SET() RELAY_SET_PIN_ON(); \
                    TIMER_Cmd(MDR_TIMER1, ENABLE)

#define RELAY_RESET() RELAY_RESET_PIN_ON(); \
                      TIMER_Cmd(MDR_TIMER1, ENABLE)

#define SWITCH_TO_RED() PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_G); \
                        PORT_SetBits(MDR_PORTA, LED_PORT_PIN_R)

#define SWITCH_TO_GREEN() PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_R); \
                          PORT_SetBits(MDR_PORTA, LED_PORT_PIN_G)
                          
#define LED_OFF()         PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_R); \
                          PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_G)
                          
#define RED_ON()  PORT_SetBits(MDR_PORTA, LED_PORT_PIN_R)
#define RED_OFF() PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_R)
                          
#define GREEN_ON()  PORT_SetBits(MDR_PORTA, LED_PORT_PIN_G)
#define GREEN_OFF() PORT_ResetBits(MDR_PORTA, LED_PORT_PIN_G)
                          
                          
#define DE_ON()  PORT_SetBits(MDR_PORTD, DE_PIN)
#define DE_OFF() PORT_ResetBits(MDR_PORTD, DE_PIN)

#define CONNECTION_TIMEOUT 500 //ms
                            
#define MAX_UART_RX_BUF_LEN 1024
#define MAX_UART_TX_COMMAND_LEN 255
#define MAX_UART_TX_COMMAND_COUNT 10

#define UART_RECEIVE_TIMEOUT 50
                            
enum LED_BLINK_TIMEOUTS
{ 
  LED_BLINK_ON_TIME_MS  = 100,
  LED_BLINK_OFF_TIME_MS = 100
};

enum LED_MODE  { LED_MODE_OFF, LED_MODE_CONTINUOUS, LED_MODE_SWITCH, LED_MODE_BLINK };
enum LED_STATE { STATE_OFF, STATE_ON };

const char *fullConverterInfoStr = "Z397-Guard converter S/N:00214 "
                                   "Pulsar Ltd. +7(812)703-77-65, +7(495)787-70-66 "
                                   "Copyright 2010 RF Enabled http://www.ironlogic.ru "
                                   "Version 3.3 build Oct 20 2011 17:22:41 "
                                   "---------------------------------- "
                                   "Current mode - Advanced\r";

const char *briefConverterInfoStr = "Z397-Guard S/N:00214,Mode:1\r";
                          
typedef struct _LED_Typedef
{
  uint32_t mode;
  uint32_t state;
  uint32_t timeout;
  
  MDR_PORT_TypeDef* MDR_PORTx;
  uint32_t PORT_Pin;
  
} LED_Typedef;

typedef struct _UART_Command_Typedef
{
  uint8_t data[MAX_UART_TX_COMMAND_LEN];
  uint8_t current;
  uint8_t len;
} UART_Command_Typedef;

typedef struct _UART_TX_RX_Typedef
{
  uint32_t RX_Head;
  uint32_t RX_Tail;
  uint8_t RX_CommandCode;
  uint32_t RX_CommandCnt;
  uint8_t  RX_Buffer[MAX_UART_RX_BUF_LEN];
  
  uint8_t TX_Busy;
  uint32_t TX_Head;
  uint32_t TX_Tail;
  UART_Command_Typedef TX_Command[MAX_UART_TX_COMMAND_COUNT];
} UART_TX_RX_Typedef;

typedef struct _TimersTypedef
{
  uint32_t connectionCounter;
} TimersTypedef;

volatile uint32_t global_ticks = 0;

static LED_Typedef red_led   = { 0, 0, 0, MDR_PORTA, LED_PORT_PIN_R };
static LED_Typedef green_led = { 0, 0, 0, MDR_PORTA, LED_PORT_PIN_G};

static UART_TX_RX_Typedef uartDataStr[2]; //0 - UART1, 1 - UART2

static TimersTypedef timersCounters = { 0 };

void Delay(uint32_t wait_ms);

/*!
* \brief Cpu clock initialization
*/
void InitCpuClock(void);

void InitGpio(void);

//Init SysTick
void InitTick(void);

void InitTimers(void);

//Init UART
void InitUART(void);

//Led function
void LED_main(void);

//UART data processing
void UART_main(void);

int main()
{
  SystemInit();
  InitCpuClock();

  InitGpio();
  InitTick();
  InitTimers();
  InitUART();
  
  Delay(100);
  
  RELAY_SET();
  RED_ON();
  Delay(1000);
  RELAY_RESET();
  SWITCH_TO_GREEN();
  Delay(1000);
  LED_OFF();
  
  DE_ON(); //Enable tranceiver RS-485
    	
  while (1)
  {
    LED_main();
    UART_main();
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

void InitGpio(void)
{
  PORT_InitTypeDef GPIOInitStruct = {0};
  
  RST_CLK_PCLKcmd (RST_CLK_PCLK_PORTA, ENABLE);
  RST_CLK_PCLKcmd (RST_CLK_PCLK_PORTB, ENABLE);
  RST_CLK_PCLKcmd (RST_CLK_PCLK_PORTD, ENABLE);
  
  PORT_DeInit(MDR_PORTA);
  PORT_DeInit(MDR_PORTB);
  PORT_DeInit(MDR_PORTD);
  
  PORT_StructInit(&GPIOInitStruct);
  
  //PA0 - RELAY SET, PA1 - RELAY RESERT; PA3 - RED LED, PA4 - GREEN LED
  GPIOInitStruct.PORT_Pin    = LED_PORT_PIN_G | LED_PORT_PIN_R | RELAY_SET_PIN | RELAY_RESET_PIN;
  GPIOInitStruct.PORT_FUNC   = PORT_FUNC_PORT;
  GPIOInitStruct.PORT_OE     = PORT_OE_OUT;
  GPIOInitStruct.PORT_SPEED  = PORT_SPEED_FAST;
  GPIOInitStruct.PORT_MODE   = PORT_MODE_DIGITAL;
  PORT_Init(MDR_PORTA, &GPIOInitStruct);
  
  //PB5 - UART1_TXD (to RS-232)
  GPIOInitStruct.PORT_Pin    = PORT_Pin_5;
  GPIOInitStruct.PORT_FUNC   = PORT_FUNC_ALTER;
  GPIOInitStruct.PORT_OE     = PORT_OE_OUT;
  GPIOInitStruct.PORT_SPEED  = PORT_SPEED_MAXFAST;
  GPIOInitStruct.PORT_MODE   = PORT_MODE_DIGITAL;
  PORT_Init(MDR_PORTB, &GPIOInitStruct);
  
  //PB6 - UART1_RXD (from RS-232)
  GPIOInitStruct.PORT_Pin    = PORT_Pin_6;
  GPIOInitStruct.PORT_OE     = PORT_OE_IN;
  PORT_Init(MDR_PORTB, &GPIOInitStruct);
  
  //PD0 - UART2_RXD (from RS-485)
  GPIOInitStruct.PORT_Pin    = PORT_Pin_0;
  GPIOInitStruct.PORT_OE     = PORT_OE_IN;
  PORT_Init(MDR_PORTD, &GPIOInitStruct);
  
  //PD1 - UART2_TXD (to RS-485)
  GPIOInitStruct.PORT_Pin    = PORT_Pin_1;
  GPIOInitStruct.PORT_OE     = PORT_OE_OUT;
  PORT_Init(MDR_PORTD, &GPIOInitStruct);
  
  //PD2 - DE
  GPIOInitStruct.PORT_Pin    = PORT_Pin_2;
  GPIOInitStruct.PORT_FUNC   = PORT_FUNC_PORT;
  GPIOInitStruct.PORT_OE     = PORT_OE_OUT;
  GPIOInitStruct.PORT_MODE   = PORT_MODE_DIGITAL;
  PORT_Init(MDR_PORTD, &GPIOInitStruct);
  
  LED_OFF();
  
  RELAY_RESET_PIN_OFF();
  RELAY_SET_PIN_OFF();
  
  DE_OFF();
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

void InitUART(void)
{
  UART_InitTypeDef UART_InitStructure;
  
  /* Enables UART clocks */
  RST_CLK_PCLKcmd( ( RST_CLK_PCLK_UART1 | RST_CLK_PCLK_UART2 ), ENABLE);
  
  UART_DeInit(MDR_UART1);
  UART_DeInit(MDR_UART2);
 
  uartDataStr[0].RX_Head = uartDataStr[0].RX_Tail = 0;
  uartDataStr[0].RX_CommandCnt = 0;
  uartDataStr[0].TX_Head = 0;
  uartDataStr[0].TX_Tail = 0;
  uartDataStr[0].TX_Busy = 0;
  uartDataStr[1].RX_Head = uartDataStr[1].RX_Tail = 0;
  uartDataStr[1].RX_CommandCnt = 0;
  uartDataStr[1].TX_Head = 0;
  uartDataStr[1].TX_Tail = 0;
  uartDataStr[1].TX_Busy = 0;
  
  UART_StructInit(&UART_InitStructure);
  
  /* Set the HCLK division factor = 1 for UART1, UART2 */
  UART_BRGInit(MDR_UART1, UART_HCLKdiv16); //80 MHz / 16 = 5 MHz
  UART_BRGInit(MDR_UART2, UART_HCLKdiv16); //80 MHz / 16 = 5 MHz
  
  NVIC_EnableIRQ(UART1_IRQn);
  NVIC_EnableIRQ(UART2_IRQn);

  /* Initialize UART_InitStructure */
  UART_InitStructure.UART_BaudRate            = 230400;
  UART_InitStructure.UART_WordLength          = UART_WordLength8b;
  UART_InitStructure.UART_StopBits            = UART_StopBits1;
  UART_InitStructure.UART_Parity              = UART_Parity_No;
  UART_InitStructure.UART_FIFOMode            = UART_FIFO_OFF;
  UART_InitStructure.UART_HardwareFlowControl = UART_HardwareFlowControl_RXE | UART_HardwareFlowControl_TXE;

  /* Configure UART1 parameters*/
  UART_Init(MDR_UART1, &UART_InitStructure);
    
  /* Enable transmitter interrupt (UARTTXINTR)*/
  UART_ITConfig(MDR_UART1, UART_IT_TX | UART_IT_RX, ENABLE);

  /* Enables UART1 peripheral */
  UART_Cmd(MDR_UART1, ENABLE);

  /* Configure UART2 parameters*/
  UART_Init(MDR_UART2, &UART_InitStructure);

  /* Enable Receiver interrupt*/
  UART_ITConfig(MDR_UART2, UART_IT_TX | UART_IT_RX, ENABLE);

  /* Enables UART2 peripheral */
  UART_Cmd(MDR_UART2, ENABLE);
}

void LED_iter(LED_Typedef *led)
{
  switch (led->mode)
  {
  case LED_MODE_OFF:
    PORT_ResetBits(led->MDR_PORTx, led->PORT_Pin);
    break;
  case LED_MODE_CONTINUOUS:
    PORT_SetBits(led->MDR_PORTx, led->PORT_Pin);
    break;
  case LED_MODE_BLINK:
    if ( ( led->state == STATE_ON ) && 
         ( global_ticks - led->timeout >= LED_BLINK_ON_TIME_MS ) )
    {
      led->timeout = global_ticks;
      led->state = STATE_OFF;
      PORT_ResetBits(led->MDR_PORTx, led->PORT_Pin);
    }
    else
    {
      if ( led->state == STATE_OFF && global_ticks - led->timeout >= LED_BLINK_OFF_TIME_MS )
      {
        led->timeout = global_ticks;
        led->state = STATE_ON;
        PORT_SetBits(led->MDR_PORTx, led->PORT_Pin);
      }
    }
    break;
  }
}

void LED_main()
{
  LED_iter(&red_led);
  LED_iter(&green_led);
  
  if ( red_led.mode == LED_MODE_SWITCH && green_led.mode == LED_MODE_SWITCH )
  {
    if ( red_led.state == STATE_ON )
    {
      if ( global_ticks - red_led.timeout >= LED_BLINK_ON_TIME_MS )
      {
        red_led.state = STATE_OFF;
        green_led.state = STATE_ON;
        green_led.timeout = global_ticks;
        SWITCH_TO_GREEN();
      }
    } 
    else if (green_led.state == STATE_ON )
    {
      if ( global_ticks - green_led.timeout >= LED_BLINK_ON_TIME_MS )
      {
        red_led.state = STATE_ON;
        green_led.state = STATE_OFF;
        red_led.timeout = global_ticks;
        SWITCH_TO_RED();
      }
    } 
    else
    {
      red_led.state = STATE_ON;
      SWITCH_TO_RED();
      red_led.timeout = global_ticks;
    }
  }
}

void UART_push(MDR_UART_TypeDef* uart, uint8_t *data, size_t len)
{
  UART_Command_Typedef *command = NULL;
  if ( uart == MDR_UART1 )
  {
    command = &(uartDataStr[0].TX_Command[uartDataStr[0].TX_Tail]);
  }
  else if ( uart == MDR_UART2 )
  {
    command = &(uartDataStr[1].TX_Command[uartDataStr[1].TX_Tail]);
  }
  
  uartDataStr[0].TX_Tail = ( uartDataStr[0].TX_Tail + 1 ) % MAX_UART_TX_COMMAND_COUNT;
  
  command->current = 0;
  command->len = (uint8_t)len;
  memcpy( command->data, data, len );
}

void UART_main(void)
{
  uint8_t rx_data;
  //uint8_t buffer[10];
  if ( ( uartDataStr[0].RX_Tail != uartDataStr[0].RX_Head ) )
  {
    rx_data  = uartDataStr[0].RX_Buffer[uartDataStr[0].RX_Head];
    
    uartDataStr[0].RX_Head = ( uartDataStr[0].RX_Head + 1 ) % MAX_UART_RX_BUF_LEN;
    
    if ( uartDataStr[0].RX_CommandCnt == 0 )
    {
      uartDataStr[0].RX_CommandCode = rx_data;
      uartDataStr[0].RX_CommandCnt = 1;
    }
    else if ( uartDataStr[0].RX_CommandCnt == 1 )
    {
      switch ( uartDataStr[0].RX_CommandCode )
      {
        //Serial number request
      case 0xC8:
        uartDataStr[0].RX_CommandCnt = 0;
        //End of frame
        if ( rx_data == 0x0D )
        {
          UART_push( MDR_UART1, (uint8_t*)briefConverterInfoStr, strlen(briefConverterInfoStr) );
        }
        break;
        //Full converter info
      case 0x49:
        uartDataStr[0].RX_CommandCnt = 0;
        //End of frame
        if ( rx_data == 0x0D )
        {
          //buffer[0] = 0x50;
          //buffer[1] = 0x0D;
          //UART_push( MDR_UART1, buffer, 2 );
          UART_push( MDR_UART1, (uint8_t*)fullConverterInfoStr, strlen(fullConverterInfoStr) );
        }
          
        break;
        
      case 0x69:
        uartDataStr[0].RX_CommandCnt = 0;
        //End of frame
        if ( rx_data == 0x0D )
        {
          UART_push( MDR_UART1, (uint8_t*)fullConverterInfoStr, strlen(fullConverterInfoStr) );
        }
          
        break;
      default: uartDataStr[0].RX_CommandCnt = 0;
      }
    }
      
    UART_SendData( MDR_UART2, rx_data );
  }
  
  if ( ( uartDataStr[0].TX_Tail != uartDataStr[0].TX_Head ) && 
      ( uartDataStr[0].TX_Busy == 0 ) )
  {
    uartDataStr[0].TX_Busy = 1;
    UART_SendData(MDR_UART1, uartDataStr[0].TX_Command[uartDataStr[0].TX_Head].data[0]);
  }
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

/**
  * @brief  UART1 interrupt handler.
  * @param  None
  * @retval None
  */
void UART1_IRQHandler(void)
{
  uint8_t *current = &(uartDataStr[0].TX_Command[uartDataStr[0].TX_Head].current);
  uint8_t len = uartDataStr[0].TX_Command[uartDataStr[0].TX_Head].len;
  
  if ( UART_GetITStatusMasked(MDR_UART1, UART_IT_TX) == SET )
  {
    ++(*current);
    if ( *current < len )
    {
      UART_SendData(MDR_UART1, uartDataStr[0].TX_Command[uartDataStr[0].TX_Head].data[(*current)]);
    }
    else                
    {
      //Command is sended;
      uartDataStr[0].TX_Head = ( uartDataStr[0].TX_Head + 1 ) % MAX_UART_TX_COMMAND_COUNT;
      uartDataStr[0].TX_Busy = 0;
    }
    
    UART_ClearITPendingBit(MDR_UART1, UART_IT_TX);
  }
    
  if ( UART_GetITStatusMasked(MDR_UART1, UART_IT_RX) == SET )
  {
    timersCounters.connectionCounter = 0;
    green_led.mode = LED_MODE_BLINK;
    
    uartDataStr[0].RX_Buffer[uartDataStr[0].RX_Tail] = (uint8_t)UART_ReceiveData(MDR_UART1);
    
    uartDataStr[0].RX_Tail = ( uartDataStr[0].RX_Tail + 1 ) % MAX_UART_RX_BUF_LEN;
    UART_ClearITPendingBit(MDR_UART1, UART_IT_RX);
  }
}

/**
  * @brief  UART2 interrupt handler.
  * @param  None
  * @retval None
  */
void UART2_IRQHandler(void)
{
  if (UART_GetITStatusMasked(MDR_UART2, UART_IT_TX) == SET)
  {
    UART_ClearITPendingBit(MDR_UART2, UART_IT_TX);
  }
  
  if (UART_GetITStatusMasked(MDR_UART2, UART_IT_RX) == SET)
  {
    timersCounters.connectionCounter = 0;
    green_led.mode = LED_MODE_BLINK;
    
    uartDataStr[1].RX_Buffer[uartDataStr[1].RX_Tail++ % MAX_UART_RX_BUF_LEN] = (uint8_t)UART_ReceiveData(MDR_UART2);
    UART_ClearITPendingBit(MDR_UART2, UART_IT_RX);
  }
  
}

void SysTick_Handler()
{
  ++global_ticks;
  
  if ( ++timersCounters.connectionCounter >= CONNECTION_TIMEOUT )
  {
    timersCounters.connectionCounter = CONNECTION_TIMEOUT;
    green_led.mode = LED_MODE_OFF;
  }
}