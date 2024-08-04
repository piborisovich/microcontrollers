/**
  ******************************************************************************
  * @file    MDR32FxQI_eeprom.c
  * @author  Milandr Application Team
  * @version V2.1.0i
  * @date    30/05/2023
  * @brief   This file contains all the EEPROM firmware functions.
  ******************************************************************************
  * <br><br>
  *
  * THE PRESENT FIRMWARE IS FOR GUIDANCE ONLY. IT AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING MILANDR'S PRODUCTS IN ORDER TO FACILITATE
  * THE USE AND SAVE TIME. MILANDR SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES RESULTING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR A USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2023 Milandr</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "MDR32FxQI_eeprom.h"
#include "MDR32FxQI_utils.h"

/** @addtogroup __MDR32FxQI_StdPeriph_Driver MDR32FxQI Standard Peripherial Driver
  * @{
  */

/** @defgroup EEPROM EEPROM
  * @{
  */

/** @defgroup EEPROM_Private_Defines EEPROM Private Defines
  * @{
  */

#define EEPROM_REG_ACCESS_KEY       ((uint32_t)0x8AAA5551)

#if defined (USE_MDR32F9Q2I) || defined (USE_MDR32FG16S1QI)
    #define EEPROM_START_ADDR           ((uint32_t)0x08000000)
#elif defined (USE_MDR32F1QI)
    #define EEPROM_START_ADDR           ((uint32_t)0x00000000)
#endif

/** @} */ /* End of group EEPROM_Private_Defines */

/** @defgroup EEPROM_Private_Macros EEPROM Private Macros
  * @{
  */

#define IS_TWO_BYTE_ALLIGNED(ADDR)   (((ADDR) & 1) == 0)
#define IS_FOUR_BYTE_ALLIGNED(ADDR)  (((ADDR) & 3) == 0)

/** @} */ /* End of group EEPROM_Private_Macros */

/** @defgroup EEPROM_Private_Function_Prototypes EEPROM Private Functions Prototypes
  * @{
  */

#if defined (USE_MDR32F9Q2I) || defined (USE_MDR32FG16S1QI)
__RAMFUNC __STATIC_INLINE void EEPROM_UpdateDCacheC(void) __attribute__((section("EXECUTABLE_MEMORY_SECTION")));
#elif defined (USE_MDR32F1QI)
__RAMFUNC void EEPROM_UpdateDCacheAsm(void) __attribute__((section("EXECUTABLE_MEMORY_SECTION")));
#endif

/** @} */ /* End of group EEPROM_Private_Function_Prototypes */


/** @defgroup EEPROM_Exported_Functions EEPROM Exported Functions
  * @{
  */

#if defined (USE_MDR32FG16S1QI)
/**
  * @brief   Sets instructions and/or data cache usage.
  * @warning This function can be used only for MCU MDR32FG16S1QI.
  * @param   EEPROM_Cache - @ref EEPROM_Cache_Type - specifies the cache type to be used.
  * @retval  None
  */
void EEPROM_SetCacheState(EEPROM_Cache_Type EEPROM_Cache)
{
    /* Check the parameters */
    assert_param(IS_EEPROM_CACHE(EEPROM_Cache));

    /* Set the selected cache */
    MDR_EEPROM->CTRL = EEPROM_Cache;
}

/**
  * @brief   Gets instructions and/or data cache usage.
  * @warning This function can be used only for MCU MDR32FG16S1QI.
  * @param   None
  * @retval  @ref EEPROM_Cache_Type: used cache type.
  */
EEPROM_Cache_Type EEPROM_GetCacheState(void)
{
    return (EEPROM_Cache_Type)(MDR_EEPROM->CTRL & EEPROM_Cache_Msk);
}

#endif /* #if defined (USE_MDR32FG16S1QI) */

/**
  * @brief   Sets the code latency value.
  * @warning EEPROM latency cycles should be set before increase of clock frequency
  *          or after clock frequency reduction.
  *          EEPROM latency should not be changed in programming mode.
  * @param   FLASH_Latency - @ref EEPROM_Latency_Cycles - specifies the FLASH Latency value.
  * @retval  None
  */
void EEPROM_SetLatency(EEPROM_Latency_Cycles EEPROM_Latency)
{
    /* Check the parameters */
    assert_param(IS_EEPROM_LATENCY(EEPROM_Latency));

    /* Set the new latency value */
    MDR_EEPROM->CMD = EEPROM_Latency;
}

/**
  * @brief  Gets the code latency value.
  * @param  None
  * @retval @ref EEPROM_Latency_Cycles - EEPROM Latency cycles value.
  */
EEPROM_Latency_Cycles EEPROM_GetLatency(void)
{
    /* Get current latency value */
    return (EEPROM_Latency_Cycles)((MDR_EEPROM->CMD & EEPROM_Latency_MSK) >> EEPROM_CMD_DELAY_Pos);
}


/**
  * @brief  Reads the 8-bit EEPROM memory value.
  * @note   Hardware supports only 32-bit word reading.
  * @param  Address: The EEPROM memory byte address.
  * @param  BankSelector - @ref EEPROM_Mem_Bank - Selects EEPROM Bank (Main Bank or Information Bank).
  *         This parameter can be one of the following values:
  *             @arg EEPROM_Main_Bank_Select:      The EEPROM Main Bank selector.
  *             @arg EEPROM_Info_Bank_Select:      The EEPROM Information Bank selector.
  * @retval The selected EEPROM memory value.
  */
__RAMFUNC uint8_t EEPROM_ReadByte(uint32_t Address, EEPROM_Mem_Bank BankSelector)
{
    uint32_t Data;
    uint32_t Command;
    uint32_t Shift;

    assert_param(IS_EEPROM_BANK_SELECTOR(BankSelector));

    MDR_EEPROM->KEY = EEPROM_REG_ACCESS_KEY;
    Command = (MDR_EEPROM->CMD & EEPROM_CMD_DELAY_Msk) | EEPROM_CMD_CON;
    Command |= (BankSelector == EEPROM_Info_Bank_Select) ? EEPROM_CMD_IFREN : 0;
    MDR_EEPROM->CMD = Command;
    MDR_EEPROM->ADR = Address;
    MDR_EEPROM->CMD = Command | EEPROM_CMD_XE | EEPROM_CMD_YE | EEPROM_CMD_SE;
    MDR_EEPROM->DO;   /* Idle Reading for Delay */
    MDR_EEPROM->DO;   /* Idle Reading for Delay */
    MDR_EEPROM->DO;   /* Idle Reading for Delay */
    Data = MDR_EEPROM->DO;
    Command &= EEPROM_CMD_DELAY_Msk;
    MDR_EEPROM->CMD = Command;
    MDR_EEPROM->KEY = 0;

    Shift = (Address & 3) * 8;
    Data >>= Shift;

    return (uint8_t)Data;
}

/**
  * @brief  Reads the 16-bit EEPROM memory value.
  * @note   Hardware supports only 32-bit word reading.
  * @param  Address: The EEPROM memory half word address (two byte aligned).
  * @param  BankSelector - @ref EEPROM_Mem_Bank - Selects EEPROM Bank (Main Bank or Information Bank).
  *         This parameter can be one of the following values:
  *             @arg EEPROM_Main_Bank_Select:      The EEPROM Main Bank selector.
  *             @arg EEPROM_Info_Bank_Select:      The EEPROM Information Bank selector.
  * @retval The selected EEPROM memory value.
  */
__RAMFUNC uint16_t EEPROM_ReadHalfWord(uint32_t Address, EEPROM_Mem_Bank BankSelector)
{
    uint32_t Data;
    uint32_t Command;
    uint32_t Shift;

    assert_param(IS_EEPROM_BANK_SELECTOR(BankSelector));
    assert_param(IS_TWO_BYTE_ALLIGNED(Address));

    MDR_EEPROM->KEY = EEPROM_REG_ACCESS_KEY;
    Command = (MDR_EEPROM->CMD & EEPROM_CMD_DELAY_Msk) | EEPROM_CMD_CON;
    Command |= (BankSelector == EEPROM_Info_Bank_Select) ? EEPROM_CMD_IFREN : 0;
    MDR_EEPROM->CMD = Command;
    MDR_EEPROM->ADR = Address;
    MDR_EEPROM->CMD = Command | EEPROM_CMD_XE | EEPROM_CMD_YE | EEPROM_CMD_SE;
    MDR_EEPROM->DO;   /* Idle Reading for Delay */
    MDR_EEPROM->DO;   /* Idle Reading for Delay */
    MDR_EEPROM->DO;   /* Idle Reading for Delay */
    Data = MDR_EEPROM->DO;
    Command &= EEPROM_CMD_DELAY_Msk;
    MDR_EEPROM->CMD = Command;
    MDR_EEPROM->KEY = 0;

    Shift = (Address & 2) * 8;
    Data >>= Shift;

    return (uint16_t)Data;
}

/**
  * @brief  Reads the 32-bit EEPROM memory value.
  * @param  Address: The EEPROM memory word address (four byte aligned).
  * @param  BankSelector - @ref EEPROM_Mem_Bank - Selects EEPROM Bank (Main Bank or Information Bank).
  *         This parameter can be one of the following values:
  *             @arg EEPROM_Main_Bank_Select:      The EEPROM Main Bank selector.
  *             @arg EEPROM_Info_Bank_Select:      The EEPROM Information Bank selector.
  * @retval The selected EEPROM memory value.
  */
__RAMFUNC uint32_t EEPROM_ReadWord(uint32_t Address, EEPROM_Mem_Bank BankSelector)
{
    uint32_t Command;
    uint32_t Data;

    assert_param(IS_EEPROM_BANK_SELECTOR(BankSelector));
    assert_param(IS_FOUR_BYTE_ALLIGNED(Address));

    MDR_EEPROM->KEY = EEPROM_REG_ACCESS_KEY;
    Command = (MDR_EEPROM->CMD & EEPROM_CMD_DELAY_Msk) | EEPROM_CMD_CON;
    Command |= (BankSelector == EEPROM_Info_Bank_Select) ? EEPROM_CMD_IFREN : 0;
    MDR_EEPROM->CMD = Command;
    MDR_EEPROM->ADR = Address;
    MDR_EEPROM->CMD = Command | EEPROM_CMD_XE | EEPROM_CMD_YE | EEPROM_CMD_SE;
    MDR_EEPROM->DO;   /* Idle Reading for Delay */
    MDR_EEPROM->DO;   /* Idle Reading for Delay */
    MDR_EEPROM->DO;   /* Idle Reading for Delay */
    Data = MDR_EEPROM->DO;
    Command &= EEPROM_CMD_DELAY_Msk;
    MDR_EEPROM->CMD = Command;
    MDR_EEPROM->KEY = 0;

    return Data;
}

/**
  * @brief  Erases one page of the selected EEPROM memory bank.
  * @param  Address: Page Address in the EEPROM memory.
  * @param  BankSelector - @ref EEPROM_Mem_Bank - Selects EEPROM Bank (Main Bank or Information Bank).
  *         This parameter can be one of the following values:
  *             @arg EEPROM_Main_Bank_Select:      The EEPROM Main Bank selector.
  *             @arg EEPROM_Info_Bank_Select:      The EEPROM Information Bank selector.
  * @retval None
  */
__RAMFUNC void EEPROM_ErasePage(uint32_t Address, EEPROM_Mem_Bank BankSelector)
{
    uint32_t Command, DelayConstUs;
    uint32_t Offset;

    assert_param(IS_EEPROM_BANK_SELECTOR(BankSelector));

    DelayConstUs = DELAY_PROGRAM_GET_CONST_US((uint32_t)FLASH_PROG_FREQ_MHZ*1E6);

    MDR_EEPROM->KEY = EEPROM_REG_ACCESS_KEY;
    Command = (MDR_EEPROM->CMD & EEPROM_CMD_DELAY_Msk) | EEPROM_CMD_CON;
    Command |= (BankSelector == EEPROM_Info_Bank_Select) ? EEPROM_CMD_IFREN : 0;
    MDR_EEPROM->CMD = Command;

    for (Offset = 0; Offset < (4 << 2); Offset += 4)
    {
        MDR_EEPROM->ADR = Address + Offset;     /* Page Address */
        MDR_EEPROM->DI = 0;
        Command |= EEPROM_CMD_XE | EEPROM_CMD_ERASE;
        MDR_EEPROM->CMD = Command;
        DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(5, DelayConstUs));       /* Wait for 5 us */
        Command |= EEPROM_CMD_NVSTR;
        MDR_EEPROM->CMD = Command;
        DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(30000, DelayConstUs));   /* Wait for 30 ms */
        Command &= ~EEPROM_CMD_ERASE;
        MDR_EEPROM->CMD = Command;
        DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(5, DelayConstUs));       /* Wait for 5 us */
        Command &= ~(EEPROM_CMD_XE | EEPROM_CMD_NVSTR);
        MDR_EEPROM->CMD = Command;
        DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(1, DelayConstUs));       /* Wait for 1 us */
    }

    Command &= EEPROM_CMD_DELAY_Msk;
    MDR_EEPROM->CMD = Command;
    MDR_EEPROM->KEY = 0;

    DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(4000, DelayConstUs));        /* Wait for 4 ms */
}

/**
  * @brief  Erases all pages of the selected EEPROM memory bank.
  * @note   Erasing all pages of Information Bank automatically erases Main Bank.
  * @param  BankSelector - @ref EEPROM_Mem_Bank - Selects EEPROM Bank (Main Bank or Main and Information Banks).
  *         This parameter can be one of the following values:
  *             @arg EEPROM_Main_Bank_Select:      The EEPROM Main Bank selector.
  *             @arg EEPROM_All_Banks_Select:      The EEPROM Main and Information Banks selector.
  * @retval None
  */
__RAMFUNC void EEPROM_EraseAllPages(EEPROM_Mem_Bank BankSelector)
{
    uint32_t Command, DelayConstUs;
    uint32_t Offset;

    assert_param(IS_EEPROM_ERASE_SELECTOR(BankSelector));

    DelayConstUs = DELAY_PROGRAM_GET_CONST_US((uint32_t)FLASH_PROG_FREQ_MHZ*1E6);

    MDR_EEPROM->KEY = EEPROM_REG_ACCESS_KEY;
    Command = (MDR_EEPROM->CMD & EEPROM_CMD_DELAY_Msk) | EEPROM_CMD_CON;
    Command |= (BankSelector == EEPROM_All_Banks_Select) ? EEPROM_CMD_IFREN : 0;

    MDR_EEPROM->CMD = Command;

    for (Offset = 0; Offset < (4 << 2); Offset += 4)
    {
        MDR_EEPROM->ADR = Offset;
        MDR_EEPROM->DI = 0;
        Command |= EEPROM_CMD_XE | EEPROM_CMD_MAS1 | EEPROM_CMD_ERASE;
        MDR_EEPROM->CMD = Command;
        DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(5, DelayConstUs));       /* Wait for 5 us */
        Command |= EEPROM_CMD_NVSTR;
        MDR_EEPROM->CMD = Command;
        DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(30000, DelayConstUs));   /* Wait for 30 ms */
        Command &= ~EEPROM_CMD_ERASE;
        MDR_EEPROM->CMD = Command;
        DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(100, DelayConstUs));     /* Wait for 100 us */
        Command &= ~(EEPROM_CMD_XE | EEPROM_CMD_MAS1 | EEPROM_CMD_NVSTR);
        MDR_EEPROM->CMD = Command;
        DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(1, DelayConstUs));       /* Wait for 1 us */
    }

    Command &= EEPROM_CMD_DELAY_Msk;
    MDR_EEPROM->CMD = Command;
    MDR_EEPROM->KEY = 0;

    DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(4000, DelayConstUs));        /* Wait for 4 ms */
}

/**
  * @brief  Programs the 8-bit EEPROM memory value.
  * @note   Hardware supports only 32-bit word programming.
  * @param  Address: The EEPROM memory byte address.
  * @param  BankSelector - @ref EEPROM_Mem_Bank - Selects EEPROM Bank (Main Bank or Information Bank).
  *         This parameter can be one of the following values:
  *             @arg EEPROM_Main_Bank_Select:      The EEPROM Main Bank selector.
  *             @arg EEPROM_Info_Bank_Select:      The EEPROM Information Bank selector.
  * @param  Data: The data value to be programmed.
  * @retval None
  */
__RAMFUNC void EEPROM_ProgramByte(uint32_t Address, EEPROM_Mem_Bank BankSelector, uint32_t Data)
{
    uint32_t Mask;
    uint32_t Tmp;
    uint32_t Shift;

    assert_param(IS_EEPROM_BANK_SELECTOR(BankSelector));

    Shift = (Address & 3) * 8;
    Data = Data << Shift;
    Mask = 0xFF << Shift;

    Tmp = EEPROM_ReadWord(Address, BankSelector);

    EEPROM_ProgramWord(Address, BankSelector, (Tmp & ~Mask) | (Data & Mask));
}

/**
  * @brief  Programs the 16-bit EEPROM memory value.
  * @note   Hardware supports only 32-bit word programming.
  * @param  Address: The EEPROM memory half word address (two byte aligned).
  * @param  BankSelector - @ref EEPROM_Mem_Bank - Selects EEPROM Bank (Main Bank or Information Bank).
  *         This parameter can be one of the following values:
  *             @arg EEPROM_Main_Bank_Select:      The EEPROM Main Bank selector.
  *             @arg EEPROM_Info_Bank_Select:      The EEPROM Information Bank selector.
  * @param  Data: The data value to be programmed.
  * @retval None
  */
__RAMFUNC void EEPROM_ProgramHalfWord(uint32_t Address, EEPROM_Mem_Bank BankSelector, uint32_t Data)
{
    uint32_t Mask;
    uint32_t Tmp;
    uint32_t Shift;

    assert_param(IS_EEPROM_BANK_SELECTOR(BankSelector));
    assert_param(IS_TWO_BYTE_ALLIGNED(Address));

    Shift = (Address & 2) * 8;
    Data = Data << Shift;
    Mask = 0xFFFF << Shift;

    Tmp = EEPROM_ReadWord(Address, BankSelector);

    EEPROM_ProgramWord(Address, BankSelector, (Tmp & ~Mask) | (Data & Mask));
}

/**
  * @brief  Programs the 32-bit EEPROM memory value.
  * @param  Address: The EEPROM memory word address (four byte aligned).
  * @param  BankSelector - @ref EEPROM_Mem_Bank - Selects EEPROM Bank (Main Bank or Information Bank).
  *         This parameter can be one of the following values:
  *             @arg EEPROM_Main_Bank_Select:      The EEPROM Main Bank selector.
  *             @arg EEPROM_Info_Bank_Select:      The EEPROM Information Bank selector.
  * @param  Data: The data value to be programmed.
  * @retval None
  */
__RAMFUNC void EEPROM_ProgramWord(uint32_t Address, EEPROM_Mem_Bank BankSelector, uint32_t Data)
{
    uint32_t Command, DelayConstUs;

    assert_param(IS_EEPROM_BANK_SELECTOR(BankSelector));
    assert_param(IS_FOUR_BYTE_ALLIGNED(Address));

    DelayConstUs = DELAY_PROGRAM_GET_CONST_US((uint32_t)FLASH_PROG_FREQ_MHZ*1E6);

    MDR_EEPROM->KEY = EEPROM_REG_ACCESS_KEY;
    Command = MDR_EEPROM->CMD & EEPROM_CMD_DELAY_Msk;
    Command |= EEPROM_CMD_CON;
    Command |= (BankSelector == EEPROM_Info_Bank_Select) ? EEPROM_CMD_IFREN : 0;
    MDR_EEPROM->CMD = Command;
    MDR_EEPROM->ADR = Address;
    MDR_EEPROM->DI  = Data;
    Command |= EEPROM_CMD_XE | EEPROM_CMD_PROG;
    MDR_EEPROM->CMD = Command;
    DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(5, DelayConstUs));       /* Wait for 5 us */
    Command |= EEPROM_CMD_NVSTR;
    MDR_EEPROM->CMD = Command;
    DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(10, DelayConstUs));      /* Wait for 10 us */
    Command |= EEPROM_CMD_YE;
    MDR_EEPROM->CMD = Command;
    DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(30, DelayConstUs));      /* Wait for 30 us */
    Command &= ~EEPROM_CMD_YE;
    MDR_EEPROM->CMD = Command;
    Command &= ~EEPROM_CMD_PROG;
    MDR_EEPROM->CMD = Command;
    DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(5, DelayConstUs));       /* Wait for 5 us */
    Command &= ~(EEPROM_CMD_XE | EEPROM_CMD_NVSTR);
    MDR_EEPROM->CMD = Command;
    DELAY_PROGRAM_WaitLoopsAsm(DELAY_PROGRAM_GET_US_LOOPS(1, DelayConstUs));       /* Wait for 1 us */

    MDR_EEPROM->CMD = Command & EEPROM_CMD_DELAY_Msk;
    MDR_EEPROM->KEY = 0;
}

/**
  * @brief   Updates data cache.
  * @warning C implementation is used for MCU MDR32F9Q2I and MDR32FG16S1QI.
  *          ASM implementation is used for MCU MDR32F1QI (errata 0007).
  * @param   None
  * @retval  None
  */
__RAMFUNC void EEPROM_UpdateDCache(void)
{
#if defined (USE_MDR32F9Q2I) || defined (USE_MDR32FG16S1QI)
    EEPROM_UpdateDCacheC();
#elif defined (USE_MDR32F1QI)
    EEPROM_UpdateDCacheAsm();
#endif
}

/** @} */ /* End of group EEPROM_Exported_Functions */

/** @defgroup EEPROM_Private_Functions EEPROM Private Functions
  * @{
  */

#if defined (USE_MDR32F9Q2I) || defined (USE_MDR32FG16S1QI)
/**
  * @brief   Updates data cache.
  * @warning This C-style function is only used for MCU MDR32F9Q2I and MDR32FG16S1QI.
  * @param   None
  * @retval  None
  */
__RAMFUNC __STATIC_INLINE void EEPROM_UpdateDCacheC(void)
{
    volatile uint32_t i, tmp;
    volatile uint32_t *ptr_eeprom = (uint32_t *) (EEPROM_START_ADDR);
    
#if defined (USE_MDR32FG16S1QI)
    if(MDR_EEPROM->CTRL & EEPROM_Cache_Data)
    {
        i = 40;
    }
    else
    {
        i = 5;
    }
#else
    i = 5;
#endif /* #if defined (USE_MDR32FG16S1QI) */
    
    /* Reading EEPROM memory to update the data cache */
    for(; i > 0; i--)
    {
        tmp = ptr_eeprom[i << 2];
    }
}
#endif /* #if defined (USE_MDR32F9Q2I) || defined (USE_MDR32FG16S1QI) */

/** @} */ /* End of group EEPROM_Private_Functions */

/** @} */ /* End of group EEPROM */

/** @} */ /* End of group __MDR32FxQI_StdPeriph_Driver */

/*********************** (C) COPYRIGHT 2023 Milandr ****************************
*
* END OF FILE MDR32FxQI_eeprom.c */


