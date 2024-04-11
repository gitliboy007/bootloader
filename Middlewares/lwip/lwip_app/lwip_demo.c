/**
 ****************************************************************************************************
 * @file        lwip_demo
 * @author      ÕýµãÔ­×ÓÍÅ¶Ó(ALIENTEK)
 * @version     V1.0
 * @date        2022-08-01
 * @brief       lwIP SOCKET UDP×é²¥ ÊµÑé
 * @license     Copyright (c) 2020-2032, ¹ãÖÝÊÐÐÇÒíµç×Ó¿Æ¼¼ÓÐÏÞ¹«Ë¾
 ****************************************************************************************************
 * @attention
 *
 * ÊµÑéÆ½Ì¨:ÕýµãÔ­×Ó F407µç»ú¿ª·¢°å
 * ÔÚÏßÊÓÆµ:www.yuanzige.com
 * ¼¼ÊõÂÛÌ³:www.openedv.com
 * ¹«Ë¾ÍøÖ·:www.alientek.com
 * ¹ºÂòµØÖ·:openedv.taobao.com
 *
 ****************************************************************************************************
 */
 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>
#include <stdio.h>
#include <lwip/sockets.h>
#include "./BSP/LCD/lcd.h"
#include "./MALLOC/malloc.h"
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip_demo.h"
//#include "Legacy/stm32_hal_legacy.h"
#include "stm32f4xx_hal.h"
#include "./SYSTEM/delay/delay.h"






#define FLASH_WR_ON			1
#define APP_START_ADDR	0x08020000  // FIRST  128KB  BOOTLOADER  
/* socketÐÅÏ¢ */


#define IntToASCII(c)  ((c)>9? ((c)+0x37): ((c)+0x30))

void ArrayToHexString(const unsigned char *inbuf, int len, char *outbuf)
{
    char temp;
    int i = 0;
    for (i = 0; i < len; i++)
    {
        temp = inbuf[i] & 0xf0;
        outbuf[3 * i + 0] = IntToASCII(temp >> 4);
        temp = inbuf[i] & 0x0f;
        outbuf[3 * i + 1] = IntToASCII(temp);
        outbuf[3 * i + 2] = 0x20; // space
    }
    outbuf[3 * i] = 0;
}





struct link_socjet_info
{
    struct sockaddr_in client_addr; /* ÍøÂçµØÖ·ÐÅÏ¢ */
    socklen_t client_addr_len;      /* ÍøÂçµØÖ·ÐÅÏ¢³¤¶È */
    int optval;                     /* Îª´æ·ÅÑ¡ÏîÖµ */
    int sfd;                        /* socket¿ØÖÆ¿é */
    ip_mreq multicast_mreq;         /* ×é²¥¿ØÖÆ¿é */
    
    struct
    {
        uint8_t *buf;               /* »º³å¿Õ¼ä */
        uint32_t size;              /* »º³å¿Õ¼ä´óÐ¡ */
    } send;                         /* ·¢ËÍ»º³å */
    
    struct
    {
        uint8_t *buf;               /* »º³å¿Õ¼ä */
        uint32_t size;              /* »º³å¿Õ¼ä´óÐ¡ */
    } recv;                         /* ½ÓÊÕ»º³å */
};

/* ¶à²¥ÐÅÏ¢ */
struct ip_mreq_t
{
    struct ip_mreq mreq;            /* ¶à²¥ÐÅÏ¢¿ØÖÆ¿é */
    socklen_t mreq_len;             /* ¶à²¥ÐÅÏ¢³¤¶È */
};

#define LWIP_SEND_THREAD_PRIO       (tskIDLE_PRIORITY + 3) /* ·¢ËÍÊý¾ÝÏß³ÌÓÅÏÈ¼¶ */
void lwip_send_thread(void *pvParameters);
void lwip_send_thread1(void *pvParameters);
/* ½ÓÊÕÊý¾Ý»º³åÇø */
static uint8_t g_lwip_demo_recvbuf[1024];//1024
static uint8_t g_lwip_demo_sendbuf[] = {"    bootloader starting   !!!!!!!!\r\n"}; 
 uint16_t rec_num = 0;




/* ¶à²¥ IP µØÖ· */
#define GROUP_IP "224.1.1.1"/*224.0.1.0*/
#define GROUP_PORT	65000

void IAP_FlashReadSome(void *buf,uint32_t len,uint32_t addr)
{
volatile uint32_t i;
volatile uint8_t *data,*fdata;
	  data=buf;
		fdata=(uint8_t *)addr;
		for(i=0;i<len;i++)
		data[i]=fdata[i];
}

/* Base address of the Flash sectors */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */

#define FLASH_TIMEOUT_VALUE       50000U /* 50 s */


/****************************************************************************
* ¹¦    ÄÜ: »ñÈ¡ÉÈÇø±àºÅ
* Èë¿Ú²ÎÊý£ºaddr£ºµØÖ·
* ³ö¿Ú²ÎÊý£ºÉÈÇøºÅ
* Ëµ    Ã÷£ºÎÞ
* µ÷ÓÃ·½·¨£ºÎÞ
****************************************************************************/
uint32_t GetSector(uint32_t Address)
{
  uint32_t sector = 0;

  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_SECTOR_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_SECTOR_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_SECTOR_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_SECTOR_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_SECTOR_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_SECTOR_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_SECTOR_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_SECTOR_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_SECTOR_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_SECTOR_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_SECTOR_10;  
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_SECTOR_11;  
  }

  return sector;
}



/**
  * @brief  Programs a half word (16-bit) at a specified address. 
  * @note   This function must be used when the device voltage range is from 2.1V to 3.6V. 
  *
  * @note   If an erase and a program operations are requested simultaneously,    
  *         the erase operation is performed before the program one.
  * 
  * @param  Address: specifies the address to be programmed.
  *         This parameter can be any address in Program memory zone or in OTP zone.  
  * @param  Data: specifies the data to be programmed.
  * @retval FLASH Status: The returned value can be: FLASH_BUSY, FLASH_ERROR_PROGRAM,
  *                       FLASH_ERROR_WRP, FLASH_ERROR_OPERATION or FLASH_COMPLETE.
  */
HAL_StatusTypeDef FLASH_ProgramHalfWord(uint32_t Address, uint16_t Data)
{
  HAL_StatusTypeDef status = HAL_ERROR;

  /* Check the parameters */
  assert_param(IS_FLASH_ADDRESS(Address));

  /* Wait for last operation to be completed */
  status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

  if(status == HAL_OK)
  {
    /* if the previous operation is completed, proceed to program the new data */
    FLASH->CR &= CR_PSIZE_MASK;
    FLASH->CR |= FLASH_PSIZE_HALF_WORD;
    FLASH->CR |= FLASH_CR_PG;

    *(__IO uint16_t*)Address = Data;

    /* Wait for last operation to be completed */
    status = FLASH_WaitForLastOperation(FLASH_TIMEOUT_VALUE);

    /* if the program operation is completed, disable the PG Bit */
    FLASH->CR &= (~FLASH_CR_PG);
  } 
  /* Return the Program Status */
  return status;
}


/**
  * @brief  Clears the FLASH's pending flags.
  * @param  FLASH_FLAG: specifies the FLASH flags to clear.
  *          This parameter can be any combination of the following values:
  *            @arg FLASH_FLAG_EOP: FLASH End of Operation flag 
  *            @arg FLASH_FLAG_OPERR: FLASH operation Error flag 
  *            @arg FLASH_FLAG_WRPERR: FLASH Write protected error flag 
  *            @arg FLASH_FLAG_PGAERR: FLASH Programming Alignment error flag 
  *            @arg FLASH_FLAG_PGPERR: FLASH Programming Parallelism error flag
  *            @arg FLASH_FLAG_PGSERR: FLASH Programming Sequence error flag
  *            @arg FLASH_FLAG_RDERR: FLASH Read Protection error flag (STM32F42xx/43xxx and STM32F401xx/411xE devices)   
  * @retval None
  */
void FLASH_ClearFlag(uint32_t FLASH_FLAG)
{
  /* Check the parameters */
  assert_param(IS_FLASH_CLEAR_FLAG(FLASH_FLAG));

  /* Clear the flags */
  FLASH->SR = FLASH_FLAG;
}


/****************************************************************************
* ¹¦    ÄÜ: Ð´Èë³¤¶ÈÎªlengthµÄ16Î»Êý¾Ý
* Èë¿Ú²ÎÊý£ºaddr£ºµØÖ·
leng£º Êý¾Ý³¤¶È
buf£ºÒªÐ´ÈëµÄÊý¾ÝÖ¸Õë
* ³ö¿Ú²ÎÊý£ºÎÞ
* Ëµ    Ã÷£ºÎÞ
* µ÷ÓÃ·½·¨£ºÎÞ
****************************************************************************/
void IAP_FlashWritSome(void *buf,uint32_t len,uint32_t addr)
{
volatile uint32_t i,e_sector;
volatile uint16_t *data;
//const uint16_t *rdata;
	data=buf;
	len>>=1;
HAL_FLASH_Unlock(); //½âËøFLASHºó²ÅÄÜÏòFLASHÖÐÐ´Êý¾Ý¡£
FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

//ÊÇ·ñÐèÒª²Á³ý
//	rdata=(const uint16_t *)addr;
//		for(i=0;i<len;i++)
//			{
//				if((rdata[i]&0xffff)!=0xffff)
//				{
//					e_sector=GetSector(addr);
//					FLASH_Erase_Sector(e_sector,FLASH_VOLTAGE_RANGE_3);
//					break;
//				}
//			}
		for(i=0; i<len; i++)
		{
			if (FLASH_ProgramHalfWord(addr, data[i]) == HAL_OK)   
			{
				 addr = addr + 2;
			}
		}
HAL_FLASH_Lock();  //¶ÁFLASH²»ÐèÒªFLASH´¦ÓÚ½âËø×´Ì¬¡£
}

void IAP_FlashReadSome(void *buf,uint32_t len,uint32_t addr);





/**
 * @brief       ²âÊÔ´úÂë
 * @param       ÎÞ
 * @retval      ÎÞ
 */
void lwip_demo(void)
{
	
		//unsigned char in[1024] = {};
		char* out = (char*)malloc(1024);
		uint16_t recnum,num = 1;
		typedef void (*UserApplication)(void) ; 
		UserApplication userApp;

    struct link_socjet_info *socket_info;
    struct ip_mreq_t *mreq_info;
    
    socket_info = mem_malloc(sizeof(struct link_socjet_info));
    mreq_info = mem_malloc(sizeof(struct ip_mreq_t));
		recnum  = 0;
 

    socket_info->sfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    if (socket_info->sfd < 0)
    {
        printf("socket failed!\n");
    }

    socket_info->client_addr.sin_family = AF_INET;
    socket_info->client_addr.sin_addr.s_addr = htonl(INADDR_ANY);   /* ´ýÓë socket °ó¶¨µÄ±¾µØÍøÂç½Ó¿Ú IP */   
    socket_info->client_addr.sin_port = htons(GROUP_PORT);                /*9999 ´ýÓë socket °ó¶¨µÄ±¾µØ¶Ë¿ÚºÅ */
    socket_info->client_addr_len = sizeof(socket_info->client_addr);
    
    /* ÉèÖÃ½ÓÊÕºÍ·¢ËÍ»º³åÇø */
    socket_info->recv.buf = g_lwip_demo_recvbuf;
    socket_info->recv.size = sizeof(g_lwip_demo_recvbuf);
    socket_info->send.buf = g_lwip_demo_sendbuf;
    socket_info->send.size = sizeof(g_lwip_demo_sendbuf);



	

    /* ½« Socket Óë±¾µØÄ³ÍøÂç½Ó¿Ú°ó¶¨ */
    int ret = bind(socket_info->sfd, (struct sockaddr*)&socket_info->client_addr, socket_info->client_addr_len);
    
    if (ret < 0)
    {
        printf(" bind error!\n ");
    }

    mreq_info->mreq.imr_multiaddr.s_addr = inet_addr(GROUP_IP);     /* ¶à²¥×é IP µØÖ·ÉèÖÃ */
    mreq_info->mreq.imr_interface.s_addr = htonl(INADDR_ANY);       /* ´ý¼ÓÈë¶à²¥×éµÄ IP µØÖ· */
    mreq_info->mreq_len = sizeof(struct ip_mreq);

    /* Ìí¼Ó¶à²¥×é³ÉÔ±£¨¸ÃÓï¾äÖ®Ç°£¬socket Ö»Óë Ä³µ¥²¥IPµØÖ·Ïà¹ØÁª Ö´ÐÐ¸ÃÓï¾äºó ½«Óë¶à²¥µØÖ·Ïà¹ØÁª£© */
    ret = setsockopt(socket_info->sfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq_info->mreq,mreq_info->mreq_len);
    
    if (ret < 0)
    {
        printf("setsockopt failed !");
    }
    else
    {
        printf("setsockopt success\r\n");
    }
    
    int length = 0;
    struct sockaddr_in sender;
    int sender_len = sizeof(sender);


	 
	


    sys_thread_new("lwip_send_thread", lwip_send_thread, (void *)socket_info, 512, LWIP_SEND_THREAD_PRIO );

	//sys_thread_new("lwip_send_thread1", lwip_send_thread1, (void *)socket_info1, 512, LWIP_SEND_THREAD_PRIO );

    while(1)
    {
        length = recvfrom(socket_info->sfd,socket_info->recv.buf,socket_info->recv.size,0,(struct sockaddr*)&sender,(socklen_t *)&sender_len);
				//socket_info->recv.buf[length]='\0';
				//printf("%s %d : %d\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), length/*socket_info->recv.buf*/);
//	ArrayToHexString(socket_info->recv.buf, length, out);			
//printf("The output string: %s", out);
//free(out);
//	printf("%s %d : %s\n", inet_ntoa(sender.sin_addr), ntohs(sender.sin_port), /*length*/out);
				
			//printf("%d  %d  %d %d %d %d\r\n",length,socket_info->recv.buf[0],socket_info->recv.buf[1],socket_info->recv.buf[2],socket_info->recv.buf[3],socket_info->recv.buf[4]);
				
			
				recnum = socket_info->recv.buf[3];
				recnum = (recnum << 8) + socket_info->recv.buf[4];
				if((num == recnum)&&(socket_info->recv.buf[0] == 0x62))
				{
					recnum = socket_info->recv.buf[3];
					recnum = (recnum << 8) + socket_info->recv.buf[4];
					if((recnum % 2) == 1)
					{
						rec_num = recnum/2;
						printf("received: %d kByte!\r\n", rec_num);
						
						//strcat((char)g_lwip_demo_sendbuf,(char)ntohs(rec_num/2));
						//strcat((char*)g_lwip_demo_sendbuf," kByte received!\r\n");
					}
#if FLASH_WR_ON 
					IAP_FlashWritSome(&socket_info->recv.buf[5],512,APP_START_ADDR + (num-1) * 512);
#endif
					num++;
				}
				else if(socket_info->recv.buf[0] == 0x61)
				{
					
				}
				else if((socket_info->recv.buf[0] == 'E')&&(length == 5))
				{
					rec_num = 1;
					printf("Erase Flashing !\r\n");
#if FLASH_WR_ON 
					HAL_FLASH_Unlock(); //½âËøFLASHºó²ÅÄÜÏòFLASHÖÐÐ´Êý¾Ý¡£
					FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

					//ÊÇ·ñÐèÒª²Á³ý
						
					//printf("Erase Flashing !\r\n");
					FLASH_Erase_Sector(GetSector(0x08020000),FLASH_VOLTAGE_RANGE_3);
					FLASH_Erase_Sector(GetSector(0x08040000),FLASH_VOLTAGE_RANGE_3);

					HAL_FLASH_Lock();  //¶ÁFLASH²»ÐèÒªFLASH´¦ÓÚ½âËø×´Ì¬¡
					//delay_ms(1000);
					//printf("Erase completed!\r\n");					
#else 
					delay_ms(2000);
#endif					
					printf("Erase completed!\r\n");	
					rec_num  = 2;
				}
				else
				{
#if FLASH_WR_ON 	
//					HAL_FLASH_Unlock(); //½âËøFLASHºó²ÅÄÜÏòFLASHÖÐÐ´Êý¾Ý¡£
//					FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

//                //ÊÇ·ñÐèÒª²Á³ý
//							
//					FLASH_Erase_Sector(GetSector(APP_START_ADDR),FLASH_VOLTAGE_RANGE_3);
//					HAL_FLASH_Lock();  //¶ÁFLASH²»ÐèÒªFLASH´¦ÓÚ½âËø×´Ì¬¡£
#endif					
						__disable_irq();
						NVIC_SystemReset();
					
				}
					
			
				if((g_lwip_demo_recvbuf[0] == 0x62)&&(recnum > 0)&&((length < (512+5)||(length == 5))))
				{	
					rec_num = 0xffff;
					vTaskDelay(500);
				
				if(((*(volatile uint32_t*)APP_START_ADDR)&0x2FF00000)==0x20000000) //0x08020000
					{
						printf("Program   completed  restarting!!!!  \r\n");	
						__disable_irq();
						userApp = (UserApplication)*(__IO uint32_t *)(APP_START_ADDR+4);
						__set_MSP(*(__IO uint32_t*)(APP_START_ADDR));
						userApp();
											
					/*	__disable_irq();
						NVIC_SystemReset();*/
						
					}
					else
					{
						__disable_irq();
						NVIC_SystemReset();
					}
				}
		
		vTaskDelay(10);
    }
}
/**
 * @brief       ²âÊÔ´úÂë
 * @param       ÎÞ
 * @retval      ÎÞ
 */


/**
 * @brief       ·¢ËÍÊý¾ÝÏß³Ìº¯Êý
 * @param       pvParameters : ´«Èëstruct link_socjet_info½á¹¹Ìå
 * @retval      ÎÞ
 */
void lwip_send_thread(void *pvParameters)
{

    struct link_socjet_info *socket_info = pvParameters;
    socket_info->client_addr.sin_addr.s_addr = inet_addr(GROUP_IP); /* ×é²¥ip */
	  uint16_t temp = 0;
	typedef void (*UserApplication1)(void) ; 
		UserApplication1 userApp1;
	
    uint8_t timeadd = 0;
    while (1)
    {
        /* Êý¾Ý¹ã²¥ */
			if(rec_num == 0 )
			{
				timeadd ++;
				g_lwip_demo_sendbuf[0] = '\0';
				sprintf((char*)g_lwip_demo_sendbuf, "Bootloader Waitting: %02dS!!!", timeadd);
				printf("Bootloader Waitting: %02dS!!!!\r\n", timeadd);
				g_lwip_demo_sendbuf[24] = '\0';				
				
			}
			else if((rec_num == 1)&&(socket_info->recv.buf[0] == 'E'))
			{
				g_lwip_demo_sendbuf[0] = '\0';
				sprintf((char*)g_lwip_demo_sendbuf, "Eraseing Starting !!!!!!");
				g_lwip_demo_sendbuf[24] = '\0';
			}
			else if((rec_num == 2)&&(socket_info->recv.buf[0] == 'E'))
			{
				g_lwip_demo_sendbuf[0] = '\0';
				sprintf((char*)g_lwip_demo_sendbuf, "Eraseing done     !!!!!!");
				g_lwip_demo_sendbuf[24] = '\0';
			}
			else if((rec_num > 0)&&(socket_info->recv.buf[0] == 'b'))
			{
					if(temp != rec_num)
					{
						timeadd = 0;
					}
					else
					{
						timeadd++;
						printf("Bootloader Waitting: %02dS!!!!\r\n", timeadd);
						if(timeadd>10)
						{
#if FLASH_WR_ON 	
					HAL_FLASH_Unlock(); //½âËøFLASHºó²ÅÄÜÏòFLASHÖÐÐ´Êý¾Ý¡£
					FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);

//ÊÇ·ñÐèÒª²Á³ý
							
					FLASH_Erase_Sector(GetSector(APP_START_ADDR),FLASH_VOLTAGE_RANGE_3);
					HAL_FLASH_Lock();  //¶ÁFLASH²»ÐèÒªFLASH´¦ÓÚ½âËø×´Ì¬¡£
#endif
							timeadd = 36;
						}
					}
					if(rec_num == 0xffff)
					{
						g_lwip_demo_sendbuf[0] = '\0';
					sprintf((char*)g_lwip_demo_sendbuf, "  Program   done!!!!  \r\n");
					g_lwip_demo_sendbuf[24] = '\0';
						
					}
					else
					{		
						
					g_lwip_demo_sendbuf[0] = '\0';
					sprintf((char*)g_lwip_demo_sendbuf, "received:  %03d  kByte!!!", rec_num);
					g_lwip_demo_sendbuf[24] = '\0';
				  temp = rec_num;
					}
			}
			
			if(timeadd>35)
			{
				timeadd = 0;
				if(((*(volatile uint32_t*)APP_START_ADDR)&0x2FF00000)==0x20000000) //0x08020000
					{
						
						__disable_irq();
						userApp1 = (UserApplication1)*(__IO uint32_t *)(APP_START_ADDR+4);
						__set_MSP(*(__IO uint32_t*)(APP_START_ADDR));
						userApp1();
											
					/*	__disable_irq();
						NVIC_SystemReset();*/
						
					}
					else
					{
						__disable_irq();
						NVIC_SystemReset();
					}
			}
			
			
        sendto(socket_info->sfd, socket_info->send.buf, 24 /*socket_info->send.size + 1*/, 0, (struct sockaddr*)&socket_info->client_addr,socket_info->client_addr_len);
        vTaskDelay(400);
    }
}
