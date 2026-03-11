#include "Flash/flash.h"

void Flash_init()
{

}

void Flash_Write(uint8_t pageNumber, uint16_t byteNumber,  uint32_t data)
{
    //static uint8_t dataFromFlash;
    //dataFromFlash = FLASH_Read(pageNumber * FLASH_PAGE_ADDRESS);

	uint32_t address = pageNumber * 1024 + byteNumber + FLASH_PAGE_ADDRESS;
	uint32_t pageAddress = pageNumber * 1024 + FLASH_PAGE_ADDRESS;

	FLASH_Unlock();
    FLASH_ErasePage(pageAddress);

    FLASH_ProgramWord(address, data);
    FLASH_Lock();
}

void Flash_WriteParameters(uint8_t pageNumber, uint32_t *dataArray, uint8_t lengthArray)
{
    //static uint8_t dataFromFlash;
    //dataFromFlash = FLASH_Read(pageNumber * FLASH_PAGE_ADDRESS);
	static uint8_t i = 0;

	static uint32_t address = 0;
	uint32_t pageAddress = pageNumber * 1024 + FLASH_PAGE_ADDRESS;

	FLASH_Unlock();
    FLASH_ErasePage(pageAddress);
    for(i = 0; i < lengthArray; i++)
    {
    	address = pageAddress + i * 4;
    	FLASH_ProgramWord(address, dataArray[i]);
    }
    FLASH_Lock();
}

uint32_t Flash_Read(uint8_t pageNumber, uint16_t byteNumber)
{
	uint32_t address = pageNumber * 1024 + (byteNumber * 4) + FLASH_PAGE_ADDRESS;
    return (*(__IO uint32_t*)address);
}

