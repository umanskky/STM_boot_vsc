/**
 * @file    flash.c
 * @author  Ferenc Nemeth
 * @date    21 Dec 2018
 * @brief   This module handles the memory related functions.
 *
 *          Copyright (c) 2018 Ferenc Nemeth - https://github.com/ferenc-nemeth
 */

#include "flash.h"

#define FLASH_START_ADRESS    0x08000000
#define FLASH_PAGE_NBPERBANK  256
#define FLASH_BANK_NUMBER     2

#define USER_FLASH_END_ADDRESS        0x08080000

/* Function pointer for jumping to user application. */
typedef void (*fnc_ptr)(void);

/**
  * @brief  Unlocks Flash for write access
  * @param  None
  * @retval None
  */
void FLASH_Init(void)
{
  /* Unlock the Program memory */
  HAL_FLASH_Unlock();

  /* Clear all FLASH flags */
  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_PGSERR | FLASH_FLAG_WRPERR | FLASH_FLAG_OPTVERR);
  /* Unlock the Program memory */
  HAL_FLASH_Lock();
}

/**
 * @brief   This function erases the memory.
 * @param   address: First address to be erased (the last is the end of the flash).
 * @return  status: Report about the success of the erasing.
 */
flash_status flash_erase(uint32_t address)
{
	uint32_t NbrOfPages = 0;
  uint32_t PageError = 0;
  FLASH_EraseInitTypeDef pEraseInit;
	HAL_StatusTypeDef hal_status = HAL_OK;
	
  HAL_FLASH_Unlock();
	
	/* Get the number of page to  erase */
  NbrOfPages = (FLASH_START_ADRESS + FLASH_SIZE);
  NbrOfPages = (NbrOfPages - address) / FLASH_PAGE_SIZE;

  flash_status status = FLASH_ERROR;
  
	if(NbrOfPages > FLASH_PAGE_NBPERBANK)
  {
    pEraseInit.Banks = FLASH_BANK_1;
    pEraseInit.NbPages = NbrOfPages % FLASH_PAGE_NBPERBANK;
    pEraseInit.Page = FLASH_PAGE_NBPERBANK - pEraseInit.NbPages;
    pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    hal_status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
  
    NbrOfPages = FLASH_PAGE_NBPERBANK;
  }
  
  if(hal_status == HAL_OK)
  {
    pEraseInit.Banks = FLASH_BANK_2;
    pEraseInit.NbPages = NbrOfPages;
    pEraseInit.Page = FLASH_PAGE_NBPERBANK - pEraseInit.NbPages;
    pEraseInit.TypeErase = FLASH_TYPEERASE_PAGES;
    hal_status = HAL_FLASHEx_Erase(&pEraseInit, &PageError);
  }
	
  /* Do the actual erasing. */
  if (HAL_OK == hal_status)
  {
    status = FLASH_OK;
  }

  HAL_FLASH_Lock();

  return status;
}

/**
 * @brief   This function flashes the memory.
 * @param   address: First address to be written to.
 * @param   *data:   Array of the data that we want to write.
 * @param   *length: Size of the array.
 * @return  status: Report about the success of the writing.
 */
flash_status flash_write(uint32_t address, uint32_t *data, uint32_t length)
{
  flash_status status = FLASH_OK;
  uint32_t i = 0;
	
  HAL_FLASH_Unlock();
	
	/* DataLength must be a multiple of 64 bit */
  for (i = 0; (i < length/2) && (address <= (USER_FLASH_END_ADDRESS-8)); i++) //USER_FLASH_END_ADDRESS-8   FLASH_BANK1_END-0x10u
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, address, *((uint64_t *)(data+2*i))) == HAL_OK)      
    {
     /* Check the written value */
      if (*(uint64_t*)address != *(uint64_t *)(data+2*i))
      {
        /* Flash content doesn't match SRAM content */
        status |= FLASH_ERROR_READBACK;
        break;
      }
      /* Increment FLASH destination address */
      address += 8;
    }
    else
    {
      /* Error occurred while writing data in Flash memory */
      status |= FLASH_ERROR_WRITE;
      break;
    }
  }

  /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
  HAL_FLASH_Lock();

  return status;

}

/**
 * @brief   Actually jumps to the user application.
 * @param   void
 * @return  void
 */
void flash_jump_to_app(void)
{
  /* Function pointer to the address of the user application. */
  fnc_ptr jump_to_app;
  jump_to_app = (fnc_ptr)(*(volatile uint32_t*) (FLASH_APP_START_ADDRESS+4u));
  HAL_DeInit();
  /* Change the main stack pointer. */
  __set_MSP(*(volatile uint32_t*)FLASH_APP_START_ADDRESS);
  jump_to_app();
}

