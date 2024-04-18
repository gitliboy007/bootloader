/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.02
 * @date        2022-6-18
 * @brief       lwIP SOCKET UDP组播 实验
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
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/LCD/lcd.h"
//#include "./USMART/usmart.h"
#include "./BSP/KEY/key.h"
#include "./MALLOC/malloc.h"
#include "./BSP/TIMER/btim.h"
#include "freertos_demo.h"
#include "stdio.h"
void  IAP_Init(void);
	
int main(void)
{
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 设置时钟,168Mhz */
    delay_init(168);                    /* 延时初始化 */
    usart_init(115200);                 /* 串口初始化为115200 */
   // usmart_dev.init(84);                /* 初始化USMART */
    led_init();                         /* 初始化LED */
    //lcd_init();                         /* 初始化LCD */
    //key_init();                         /* 初始化按键 */
    
    my_mem_init(SRAMIN);                /* 初始化内部SRAM内存池 */
    my_mem_init(SRAMCCM);               /* 初始化内部SRAMCCM内存池 */

		printf ("bootloader starting!!\r\n");
		IAP_Init();
    freertos_demo();                    /* 创建lwIP的任务函数 */
}

#define ApplicationAddressOne    0x8020000
typedef void(*pFunction)(void);
void  IAP_Init(void)
{

	__IO uint32_t JumpAddress;
	pFunction   Jump_To_Application;
	


	if(!__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))//如果下载完成或不是U盘挂载软复位则跳转
	{
			__HAL_RCC_CLEAR_RESET_FLAGS();//清除复位标志
			//if(((*(volatile uint32_t*)ApplicationAddressOne)&0x2FFE0000)==0x20000000) //0x08020000
			{ 
				
				
				printf ("jump to userapp!!!\r\n");
				 delay_ms(500);
					__set_PRIMASK(1);
				__disable_irq();      // 关闭所有中断
				
				
					//跳转至用户代码  One
					JumpAddress = *(volatile uint32_t *)(ApplicationAddressOne + 4);  					
					Jump_To_Application = (pFunction)JumpAddress;  
					//初始化用户程序的堆栈指针  
					__set_MSP(*(volatile uint32_t*)ApplicationAddressOne); 
					/* Set the Vector Table base location at the application base address */
					SCB->VTOR = ApplicationAddressOne;
					Jump_To_Application();//执行 用户程序 						
			}	
	}
	__set_PRIMASK(0);
	__enable_irq();      // 打开所有中断
	
	printf ("jump to bootloader!!!\r\n");
	
	
	
}
