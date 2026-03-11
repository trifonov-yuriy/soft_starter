#include <stm32f10x.h>
#include <stm32f10x_flash.h>

#define FLASH_PAGE_ADDRESS   0x08000000
#define FLASH_WRITED         0xAA        //Была запись во флэш-память

uint32_t Flash_Read(uint8_t pageNumber, uint16_t byteNumber);

void Flash_Write(uint8_t pageNumber, uint16_t byteNumber,  uint32_t data);
void Flash_WriteParameters(uint8_t pageNumber, uint32_t *dataArray, uint8_t lengthArray);
