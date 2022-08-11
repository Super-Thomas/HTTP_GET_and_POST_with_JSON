/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/main.c 
  * @author  MCD Application Team
  * @brief   This sample code implements a http server application based on 
  *          Netconn API of LwIP stack and FreeRTOS. This application uses 
  *          STM32F7xx the ETH HAL API to transmit and receive data. 
  *          The communication is done with a web browser of a remote PC.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2016 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "ethernetif.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "app_ethernet.h"
#include "httpserver-netconn.h"
#include "lcd_log.h"
// Thomas: Added header
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "cJSON.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
struct netif gnetif; /* network interface structure */

/* Private function prototypes -----------------------------------------------*/
static void SystemClock_Config(void);
static void StartThread(void const * argument);
static void BSP_Config(void);
static void Netif_Config(void);
static void MPU_Config(void);
static void Error_Handler(void);
static void CPU_CACHE_Enable(void);

/* Private functions ---------------------------------------------------------*/
// Thomas: Added variables
uint8_t gu8DataBuffer[1024];

// Thomas: Added functions
void HTTP_GET_Request(void) {
	int ret;
	struct sockaddr_in sin;
	int CControl;
	int i;
	uint8_t u8DataBuffer[1024];
	int nCounter;

	// Create TCP socket
	CControl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (CControl == -1) {
		LCD_UsrLog ((char *)"Impossible get a socket\r\n");
		for(;;);
	}

	vTaskDelay(1000/portTICK_RATE_MS);

	// Input HTTP server information that you want to connect to
	memset(&sin,0,sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80); // 80 port is standard
	sin.sin_addr.s_addr = inet_addr("178.128.25.248"); // 178.128.25.248 is IP of api.openweathermap.org
	
  // Connect to HTTP server
	ret = connect(CControl, (struct sockaddr *)&sin, sizeof(sin));
	if (ret == -1) {
		closesocket(CControl);
		LCD_UsrLog ((char *)"Impossible connect to remote TCP server\r\n");
		for(;;);
	}

	// Configure HTTP header for GET request
	uint8_t http_head[]="GET /data/2.5/weather?q=Seoul,KR&units=metric&appid=9dacd623d8637f89267d33608c32700c HTTP/1.0\r\nHost:api.openweathermap.org\r\n\r\n\r\n";
	// Send HTTP header to HTTP server
	ret = send(CControl, http_head, sizeof(http_head), 0);
	if (ret != sizeof(http_head)){
		for(;;);
	}

	nCounter = 0;
	memset(u8DataBuffer, 0x00, sizeof(u8DataBuffer));

	// Check response
	do {
		// Read response from HTTP server
		ret = read(CControl, &u8DataBuffer[nCounter], 1024);
		if (ret != 0) {
			if ((nCounter + ret) > 1023) {
				// Overrun
			}
			else {
				nCounter += ret;
			}
		}
	} while(ret != 0);

	closesocket(CControl);

	vTaskDelay(2000/portTICK_RATE_MS);

	// Get only body message without header in response message
	memset(&gu8DataBuffer[0], 0, sizeof(gu8DataBuffer));
	for (i=0; i<nCounter-4; i++) {
		if (u8DataBuffer[i] == 0x0D && u8DataBuffer[i+1] == 0x0A &&
			u8DataBuffer[i+2] == 0x0D && u8DataBuffer[i+3] == 0x0A) {
		memcpy(&gu8DataBuffer[0], &u8DataBuffer[i+4], nCounter-(i+4));
		}
	}

	// For debug
	//LCD_UsrLog ("%s\n", gu8DataBuffer);
}

void JSON_Parse(void) {
	//char strTestData[1024] = {"{\"coord\":{\"lon\":126.9778,\"lat\":37.5683},\"weather\":[{\"id\":800,\"main\":\"Clear\",\"description\":\"clear sky\",\"icon\":\"01n\"}],\"base\":\"stations\",\"main\":{\"temp\":-4.67,\"feels_like\":-13.52,\"temp_min\":-5,\"temp_max\":-4,\"pressure\":1024,\"humidity\":39},\"visibility\":10000,\"wind\":{\"speed\":7.72,\"deg\":320},\"clouds\":{\"all\":0},\"dt\":1610966264,\"sys\":{\"type\":1,\"id\":8105,\"country\":\"KR\",\"sunrise\":1610923480,\"sunset\":1610959186},\"timezone\":32400,\"id\":1835848,\"name\":\"Seoul\",\"cod\":200}"};
	/*
	{
    	"coord":
    	{
        	"lon":126.9778,
        	"lat":37.5683
    	},
        "weather":
    	[
    		{
        		"id":800,
        		"main":"Clear",
        		"description":"clear sky",
        		"icon":"01n"
    		}
    	],
    	"base":"stations",
    	"main":
    	{
        	"temp":-4.67,
        	"feels_like":-13.52,
        	"temp_min":-5,
        	"temp_max":-4,
        	"pressure":1024,
        	"humidity":39
    	},
        "visibility":10000,
        "wind":
    	{
        	"speed":7.72,
        	"deg":320
    	},
    }
	*/
	cJSON *root = cJSON_Parse(&gu8DataBuffer[0]);
	cJSON *weather = cJSON_GetObjectItem(root, "weather");
	cJSON *wind = cJSON_GetObjectItem(root, "wind");
	int nArrayLength = cJSON_GetArraySize(weather);
	int i;

	for (i = 0; i<nArrayLength; i++) {
		cJSON *weatherArray = cJSON_GetArrayItem(weather, i);
		cJSON *id = cJSON_GetObjectItem(weatherArray, "id");
		cJSON *icon = cJSON_GetObjectItem(weatherArray, "icon");
		if (cJSON_IsNumber(id)) {
			LCD_UsrLog ((char *)"weather_id: %d\n", id->valueint);
		}
		if (cJSON_IsString(icon)) {
			LCD_UsrLog ((char *)"weather_icon: %d\n", icon->valuestring);
		}
	}

	cJSON *speed = cJSON_GetObjectItem(wind, "speed");
	cJSON *deg = cJSON_GetObjectItem(wind, "deg");
	if (cJSON_IsNumber(speed)) {
		LCD_UsrLog ((char *)"weather_wind_speed: %f\n", speed->valuedouble);
	}
	if (cJSON_IsNumber(deg)) {
		LCD_UsrLog ((char *)"weather_wind_degree: %d\n", deg->valueint);
	}

	cJSON_Delete(root);
}

void JSON_Make(void) {
	/*
	{
	   	"cars":
	   	[
	       	{
	           	"CarType":"BMW",
	          	"carID":"bmw123"
	       	},
	       	{
	           	"CarType":"mercedes",
	           	"carID":"mercedes123"
	      	}
	  	]
	}
	*/
	char *pOut;
	cJSON *root, *cars, *car;

	/* create root node and array */
	root = cJSON_CreateObject();
	cars = cJSON_CreateArray();

	/* add cars array to root */
	cJSON_AddItemToObject(root, "cars", cars);

	/* add 1st car to cars array */
	cJSON_AddItemToArray(cars, car = cJSON_CreateObject());
	cJSON_AddItemToObject(car, "CarType", cJSON_CreateString("BMW"));
	cJSON_AddItemToObject(car, "carID", cJSON_CreateString("bmw123"));

	/* add 2nd car to cars array */
	cJSON_AddItemToArray(cars, car = cJSON_CreateObject());
	cJSON_AddItemToObject(car, "CarType", cJSON_CreateString("mercedes"));
	cJSON_AddItemToObject(car, "carID", cJSON_CreateString("mercedes123"));

	/* print everything */
	pOut = cJSON_Print(root);
	memset(&gu8DataBuffer[0], 0, sizeof(gu8DataBuffer));
	memcpy(&gu8DataBuffer[0], pOut, strlen(pOut));

	free(pOut);
	cJSON_Delete(root);

	// For debug
	LCD_UsrLog ("%s\n", gu8DataBuffer);
}

void HTTP_POST_Request(void) {
	int ret;
	struct sockaddr_in sin;
	int CControl;

	// Create TCP socket
	CControl = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (CControl == -1) {
		LCD_UsrLog ((char *)"Impossible get a socket\r\n");
		for(;;);
	}

	vTaskDelay(1000/portTICK_RATE_MS);

	// Input HTTP server information that you want to connect to
	memset(&sin,0,sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80); // 80 port is standard
	sin.sin_addr.s_addr = inet_addr("46.4.105.116"); //46.4.105.116 is IP of webhook.site

	// Connect to HTTP server
	ret = connect(CControl, (struct sockaddr *)&sin, sizeof(sin));
	if (ret == -1) {
		closesocket(CControl);
		LCD_UsrLog ((char *)"Impossible connect to remote TCP server\r\n");
		for(;;);
	}

	// Configure HTTP header for POST request
	uint8_t http_head[1024];
	memset(&http_head[0], 0, sizeof(http_head));
	sprintf(&http_head[0], "POST /9aa08aba-4ead-4fd9-9278-1ac46e0f3224 HTTP/1.0\r\nHost:webhook.site\r\nContent-Type:application/json\r\nContent-Length:%d\r\n\r\n%s",
		strlen(gu8DataBuffer),
		gu8DataBuffer);

	// Send HTTP header to HTTP server
	ret = send(CControl, http_head, sizeof(http_head), 0);
	if (ret != sizeof(http_head)){
		for(;;);
	}

	closesocket(CControl);

	vTaskDelay(2000/portTICK_RATE_MS);
}

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /* Configure the MPU attributes as Device memory for ETH DMA descriptors */
  MPU_Config();

  /* Enable the CPU Cache */
  CPU_CACHE_Enable();

  /* STM32F7xx HAL library initialization:
       - Configure the Flash ART accelerator on ITCM interface
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
     */
  HAL_Init();  
  
  /* Configure the system clock to 200 MHz */
  SystemClock_Config(); 
  
  /* Init thread */
#if defined(__GNUC__)
  osThreadDef(Start, StartThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 5);
#else
  osThreadDef(Start, StartThread, osPriorityNormal, 0, configMINIMAL_STACK_SIZE * 2);
#endif
  
  osThreadCreate (osThread(Start), NULL);
  
  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */
  for( ;; );
}

/**
  * @brief  Start Thread 
  * @param  argument not used
  * @retval None
  */
static void StartThread(void const * argument)
{ 
  /* Initialize LCD */
  BSP_Config();
  
  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);
  
  /* Initialize the LwIP stack */
  Netif_Config();
  
  /* Initialize webserver demo */
  //http_server_netconn_init(); // Thomas: Disabled webserver demo
  
  /* Notify user about the network interface config */
  User_notification(&gnetif);
  
#ifdef USE_DHCP
  /* Start DHCPClient */
  osThreadDef(DHCP, DHCP_thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 2);
  osThreadCreate (osThread(DHCP), &gnetif);
#endif

  ////////////////////////////////////////////////////////////////////////////
  // Thomas: Added
  HTTP_GET_Request();
  JSON_Parse();
  JSON_Make();
  HTTP_POST_Request();
  ////////////////////////////////////////////////////////////////////////////

  for( ;; )
  {
    /* Delete the Init Thread */ 
    osThreadTerminate(NULL);
  }
}

/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
static void Netif_Config(void)
{ 
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
 
#ifdef USE_DHCP
  ip_addr_set_zero_ip4(&ipaddr);
  ip_addr_set_zero_ip4(&netmask);
  ip_addr_set_zero_ip4(&gw);
#else
  IP_ADDR4(&ipaddr,IP_ADDR0,IP_ADDR1,IP_ADDR2,IP_ADDR3);
  IP_ADDR4(&netmask,NETMASK_ADDR0,NETMASK_ADDR1,NETMASK_ADDR2,NETMASK_ADDR3);
  IP_ADDR4(&gw,GW_ADDR0,GW_ADDR1,GW_ADDR2,GW_ADDR3);
#endif /* USE_DHCP */
  
  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);
  
  /*  Registers the default network interface. */
  netif_set_default(&gnetif);
  
  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called.*/
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }
}

/**
  * @brief  Initializes the STM327546G-Discovery's LCD  resources.
  * @param  None
  * @retval None
  */
static void BSP_Config(void)
{
  /* Initialize the LCD */
  BSP_LCD_Init();
  
  /* Initialize the LCD Layers */
  BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS);
  
  /* Set LCD Foreground Layer  */
  BSP_LCD_SelectLayer(1);
  
  BSP_LCD_SetFont(&LCD_DEFAULT_FONT);
  
  /* Initialize LCD Log module */
  LCD_LOG_Init();
  
  /* Show Header and Footer texts */
  LCD_LOG_SetHeader((uint8_t *)"Webserver Application Netconn API");
  LCD_LOG_SetFooter((uint8_t *)"STM32746G-DISCO board");
  
  LCD_UsrLog ((char *)"  State: Ethernet Initialization ...\n");
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 200000000
  *            HCLK(Hz)                       = 200000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /* activate the OverDrive */
  if(HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }
  
  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
static void Error_Handler(void)
{
  /* User may add here some code to deal with this error */
  while(1)
  {
  }
}

/**
  * @brief  Configure the MPU attributes .
  * @param  None
  * @retval None
  */
static void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct;
  
  /* Disable the MPU */
  HAL_MPU_Disable();
  
  /* Configure the MPU as Normal Non Cacheable for Ethernet Buffers in the SRAM2 */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x2004C000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_16KB;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  
  /* Configure the MPU as Device for Ethernet Descriptors in the SRAM2 */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.BaseAddress = 0x2004C000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256B;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.SubRegionDisable = 0x00;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  
  /* Enable the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
  * @brief  CPU L1-Cache enable.
  * @param  None
  * @retval None
  */
static void CPU_CACHE_Enable(void)
{
  /* Enable I-Cache */
  SCB_EnableICache();

  /* Enable D-Cache */
  SCB_EnableDCache();
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
