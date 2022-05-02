#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../p2l.h"
#include "virtural_device.h"

#define PAGE_KBYTES_SIZE	16
#define PAGE_BYTES_SIZE		PAGE_KBYTES_SIZE * 1024

unsigned char* pCatchRegister = NULL;

int VirtualDevice_Init()
{
	//iInitDevConfig(0 /*devCap*/, 0 /*ddrSize*/, 0 /*chCnt*/, 0 /*planeCnt*/, 0 /*pageCnt*/, 0 /*bufPtr*/); //to do

	pCatchRegister = (unsigned char *)malloc(PAGE_BYTES_SIZE);

	if (pCatchRegister) {
		memset(pCatchRegister, 0, PAGE_BYTES_SIZE);
	}
}

void VirtualDevice_Deinit()
{
	if (pCatchRegister) {
		free(pCatchRegister);
		pCatchRegister = NULL;
	}
}

void VirtualDevice_GenAddress(t_dev_mgr dev_mgr, unsigned long address, int addressCnt, t_virtual_dev_addr *virtualDevAddr)
{
	bool isFullAddress = (addressCnt >= 5) ? true : false;

	if (isFullAddress) {
		address = address >> 16; //remove column addresses
	}

	virtualDevAddr->page = (address & (int)(pow(2, dev_mgr.pageBitNum) - 1));
	virtualDevAddr->plane = ((address >> dev_mgr.planeSftCnt) & (int)(pow(2, dev_mgr.planeBitNum) - 1)) << dev_mgr.planeSftCnt;
	virtualDevAddr->block = ((address >> dev_mgr.blkSftCnt) & (int)(pow(2, dev_mgr.blkBitNum) - 1)) << dev_mgr.blkSftCnt;
	virtualDevAddr->ch = ((address >> dev_mgr.chSftCnt) & (int)(pow(2, dev_mgr.chBitNum) - 1)) << dev_mgr.chSftCnt;
}

void VirtualDevice_PatternHandler(unsigned char* pPattern, unsigned char* pDataBuf, int DataBufLen)
{
	int				index = 0;
	int				currentMode = FLASH_MODE_STANDBY;
	unsigned long	address = 0;
	int				addressCnt = 0;

	t_pattern_header* header = (t_pattern_header*)pPattern;

	for (index = sizeof(t_pattern_header); index < header->lengthOfDescription; index++)
	{
		t_pattern_unit* pPatternUnit = (t_pattern_unit*)(pPattern + index);

		switch (pPatternUnit->raw.token)
		{
		case PATTERN_TOKEN_CMD:
			switch (pPatternUnit->cmd.val)
			{
				case FLASH_MODE_READ:
					currentMode = FLASH_MODE_READ;
					address = 0;
					addressCnt = 0;
					break;

				case FLASH_MODE_PROGRAM:
					currentMode = FLASH_MODE_PROGRAM;
					address = 0;
					addressCnt = 0;
					break;

				case FLASH_MODE_ERASE:
					currentMode = FLASH_MODE_READ;
					address = 0;
					addressCnt = 0;
					break;

				case FLASH_MODE_RESET:
					currentMode = FLASH_MODE_RESET;
					break;

				default:
					switch (currentMode)
					{
						case FLASH_MODE_READ:
							if (pPatternUnit->cmd.val == 0x30) {
								t_virtual_dev_addr virtualDevAddr = { 0 };

								VirtualDevice_GenAddress(dev_mgr, address, addressCnt, &virtualDevAddr);

								iFlashCmdHandler(E_CMD_READ, virtualDevAddr.ch, virtualDevAddr.block, virtualDevAddr.plane, virtualDevAddr.page, (int *)pCatchRegister);
							}
							else {
								//to do
							}
							break;

						case FLASH_MODE_PROGRAM:
							if (pPatternUnit->cmd.val == 0x10) {
								t_virtual_dev_addr virtualDevAddr = { 0 };

								VirtualDevice_GenAddress(dev_mgr, address, addressCnt, &virtualDevAddr);

								iFlashCmdHandler(E_CMD_WRITE, virtualDevAddr.ch, virtualDevAddr.block, virtualDevAddr.plane, virtualDevAddr.page, (int*)pCatchRegister);
							}
							else {
								//to do
							}
							break;

						case FLASH_MODE_ERASE:
							if (pPatternUnit->cmd.val == 0xD0) {
								t_virtual_dev_addr virtualDevAddr = { 0 };

								VirtualDevice_GenAddress(dev_mgr, address, addressCnt, &virtualDevAddr);

								iFlashCmdHandler(E_CMD_ERASE, virtualDevAddr.ch, virtualDevAddr.block, virtualDevAddr.plane, virtualDevAddr.page, 0);
							}
							else {
								//to do
							}
							break;
						}
					break;
			}
			index++;
			break;

		case PATTERN_TOKEN_ADDR:
			address |= (pPatternUnit->addr.val >> 8 * addressCnt) & 0xff;
			addressCnt++;
			index++;
			break;

		case PATTERN_TOKEN_READ_DATA_WITH_KBYTES:
			if (currentMode == FLASH_MODE_READ) {
				memcpy(pDataBuf, pCatchRegister, PAGE_BYTES_SIZE);
			}
			break;

		case PATTERN_TOKEN_WRITE_DATA:
			if (currentMode == FLASH_MODE_PROGRAM) {
				memcpy(pCatchRegister, pDataBuf, PAGE_BYTES_SIZE);
			}
			break;

		default:
			break;
		}
	}
}