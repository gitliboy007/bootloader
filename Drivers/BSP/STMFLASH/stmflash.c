/**
 ****************************************************************************************************
 * @file        stmflash.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2021-10-28
 * @brief       STM32内部FLASH读写 驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台：正点原子 F407电机开发板
 * 在线视频：www.yuanzige.com
 * 技术论坛：http://www.openedv.com/forum.php
 * 公司网址：www.alientek.com
 * 购买地址：zhengdianyuanzi.tmall.com
 *
 * 修改说明
 * V1.0 20211028
 * 第一次发布
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/STMFLASH/stmflash.h"

/**
 * @brief       从指定地址读取一个字 (32位数据)
 * @param       faddr   : 读取地址 (此地址必须为4倍数!!)
 * @retval      读取到的数据 (32位)
 */
uint32_t stmflash_read_word(uint32_t faddr)
{
    return *(volatile uint32_t *)faddr;
}

/**
 * @brief       从指定地址开始读出指定长度的数据
 * @param       raddr : 起始地址
 * @param       pbuf  : 数据指针
 * @param       length: 要读取的字(32位)数,即4个字节的整数倍
 * @retval      无
 */
void stmflash_read(uint32_t ReadAddr, uint32_t *pBuffer, uint32_t NumToRead)
{
    uint32_t  i;
    for (i = 0; i < NumToRead; i++)
    {
        pBuffer[i] = stmflash_read_word(ReadAddr);/* 读取4个字节 */
        ReadAddr += 4;/* 偏移4个字节 */
    }
}

/**
 * @brief       获取某个地址所在的flash扇区
 * @param       addr    : lash地址
 * @retval      0~11,即addr所在的扇区
 */
uint8_t  stmflash_get_flash_sector(uint32_t addr)
{
    if (addr < ADDR_FLASH_SECTOR_1) return FLASH_SECTOR_0;
    else if (addr < ADDR_FLASH_SECTOR_2) return FLASH_SECTOR_1;
    else if (addr < ADDR_FLASH_SECTOR_3) return FLASH_SECTOR_2;
    else if (addr < ADDR_FLASH_SECTOR_4) return FLASH_SECTOR_3;
    else if (addr < ADDR_FLASH_SECTOR_5) return FLASH_SECTOR_4;
    else if (addr < ADDR_FLASH_SECTOR_6) return FLASH_SECTOR_5;
    else if (addr < ADDR_FLASH_SECTOR_7) return FLASH_SECTOR_6;
    else if (addr < ADDR_FLASH_SECTOR_8) return FLASH_SECTOR_7;
    else if (addr < ADDR_FLASH_SECTOR_9) return FLASH_SECTOR_8;
    else if (addr < ADDR_FLASH_SECTOR_10) return FLASH_SECTOR_9;
    else if (addr < ADDR_FLASH_SECTOR_11) return FLASH_SECTOR_10;
    return FLASH_SECTOR_11;
}

/**
 * @brief       从指定地址开始写入指定长度的数据
 *   @note      特别注意：因为STM32F4的扇区实在太大,没办法本地保存扇区数据,所以本函数
 *              写地址如果非0XFF,那么会先擦除整个扇区且不保存扇区数据.所以
 *              写非0XFF的地址,将导致整个扇区数据丢失.建议写之前确保扇区里
 *              没有重要数据,最好是整个扇区先擦除了,然后慢慢往后写
 *              该函数对OTP区域也有效!可以用来写OTP区!
 *              OTP区域地址范围:0X1FFF7800~0X1FFF7A0F(注意：最后16字节，用于OTP数据块锁定，别乱写！！)
 * @param       WriteAddr:起始地址(此地址必须为4的倍数!!)
 * @param       pBuffer:数据指针
 * @param       NumToWrite:字(32位)数(就是要写入的32位数据的个数)
 * @retval      无
 */
void stmflash_write(uint32_t WriteAddr, uint32_t *pBuffer, uint32_t NumToWrite)
{
    FLASH_EraseInitTypeDef FlashEraseInit;
    HAL_StatusTypeDef FlashStatus = HAL_OK;
    uint32_t SectorError = 0;
    uint32_t  addrx = 0;
    uint32_t  endaddr = 0;
    if (WriteAddr < STM32_FLASH_BASE || WriteAddr%4) return;    /* 非法地址 */

    HAL_FLASH_Unlock();     /* 解锁 */
    addrx = WriteAddr;      /* 写入的起始地址 */
    endaddr = WriteAddr + NumToWrite * 4;    /* 写入的结束地址 */

    if (addrx < 0X1FFF0000)
    {
        while (addrx < endaddr)     /* 扫清一切障碍.(对非FFFFFFFF的地方,先擦除) */
        {
            if (stmflash_read_word(addrx) != 0XFFFFFFFF)                    /* 有非0XFFFFFFFF的地方,要擦除这个扇区 */
            {
                FlashEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;         /* 擦除类型，扇区擦除 */
                FlashEraseInit.Sector = stmflash_get_flash_sector(addrx);   /* 要擦除的扇区 */
                FlashEraseInit.NbSectors = 1;                               /* 一次只擦除一个扇区 */
                FlashEraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;        /* 电压范围，VCC=2.7~3.6V之间!! */
                if (HAL_FLASHEx_Erase(&FlashEraseInit,&SectorError) != HAL_OK)
                {
                    break;  /* 发生错误了 */
                }
            } 
            else
            {
               addrx += 4;
            }
            FLASH_WaitForLastOperation(FLASH_WAITETIME);                    /* 等待上次操作完成 */
        }
    }
    FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME);                /* 等待上次操作完成 */
    if (FlashStatus == HAL_OK)
    {
        while (WriteAddr < endaddr) /* 写数据 */
        {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, WriteAddr, *pBuffer) != HAL_OK)    /* 写入数据 */
            {
                break;              /* 写入异常 */
            }
            WriteAddr += 4;
            pBuffer++;
        }
    }
    HAL_FLASH_Lock();               /* 上锁 */
}

/******************************************************************************************/
/* 测试用代码 */

/**
 * @brief       测试写数据(写1个字)
 * @param       waddr : 起始地址
 * @param       wdata : 要写入的数据
 * @retval      读取到的数据
 */
void test_write(uint32_t WriteAddr, uint32_t WriteData)
{
    stmflash_write(WriteAddr, &WriteData, 1);/* 写入一个字 */
}

