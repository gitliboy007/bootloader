/**
 ****************************************************************************************************
 * @file        main.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.02
 * @date        2022-6-18
 * @brief       lwIP SOCKET UDP�鲥 ʵ��
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨������ԭ�� F407���������
 * ������Ƶ��www.yuanzige.com
 * ������̳��http://www.openedv.com/forum.php
 * ��˾��ַ��www.alientek.com
 * �����ַ��zhengdianyuanzi.tmall.com
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
    HAL_Init();                         /* ��ʼ��HAL�� */
    sys_stm32_clock_init(336, 8, 2, 7); /* ����ʱ��,168Mhz */
    delay_init(168);                    /* ��ʱ��ʼ�� */
    usart_init(115200);                 /* ���ڳ�ʼ��Ϊ115200 */
   // usmart_dev.init(84);                /* ��ʼ��USMART */
    led_init();                         /* ��ʼ��LED */
    //lcd_init();                         /* ��ʼ��LCD */
    //key_init();                         /* ��ʼ������ */
    
    my_mem_init(SRAMIN);                /* ��ʼ���ڲ�SRAM�ڴ�� */
    my_mem_init(SRAMCCM);               /* ��ʼ���ڲ�SRAMCCM�ڴ�� */

		printf ("bootloader starting!!\r\n");
		IAP_Init();
    freertos_demo();                    /* ����lwIP�������� */
}

#define ApplicationAddressOne    0x8020000
typedef void(*pFunction)(void);
void  IAP_Init(void)
{

	__IO uint32_t JumpAddress;
	pFunction   Jump_To_Application;
	


	if(!__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))//���������ɻ���U�̹�����λ����ת
	{
			__HAL_RCC_CLEAR_RESET_FLAGS();//�����λ��־
			//if(((*(volatile uint32_t*)ApplicationAddressOne)&0x2FFE0000)==0x20000000) //0x08020000
			{ 
				
				
				printf ("jump to userapp!!!\r\n");
				 delay_ms(500);
					__set_PRIMASK(1);
				__disable_irq();      // �ر������ж�
				
				
					//��ת���û�����  One
					JumpAddress = *(volatile uint32_t *)(ApplicationAddressOne + 4);  					
					Jump_To_Application = (pFunction)JumpAddress;  
					//��ʼ���û�����Ķ�ջָ��  
					__set_MSP(*(volatile uint32_t*)ApplicationAddressOne); 
					/* Set the Vector Table base location at the application base address */
					SCB->VTOR = ApplicationAddressOne;
					Jump_To_Application();//ִ�� �û����� 						
			}	
	}
	__set_PRIMASK(0);
	__enable_irq();      // �������ж�
	
	printf ("jump to bootloader!!!\r\n");
	
	
	
}
