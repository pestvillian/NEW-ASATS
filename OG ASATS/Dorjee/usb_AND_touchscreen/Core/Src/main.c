/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/****** THIS IS THE PROGRAM FLOW SO YOU DON'T GET C *********/
/* Library includes
 * Definitions of macros
 * Setting up touchscreen variables
 * stm32 prewritten function definitions
 * private function definitions
 * QR scanner USB HID host user call back function event handling
 * Main initialization code: storing test protocols for debugging
 * Initialize hardware
 * Main loop calls the touch handler with interrupt and USB is handled by STM32
 * stm32 prewritten function implementations
 * flash memory functions come first
 * touchscreen functions after
 * lastly, interrupt handler
 */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbh_hid.h"
#include <string.h>
#include "stm32f429i_discovery_lcd.h"
#include "stm32f429i_discovery_ts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NULL_CHAR 0
#define SOH 1
#define NEWLINE_CHAR 10   //the '/r' in python is detected as a newline (10).
#define CARRIAGE_CHAR 13  //the '\n' is just ignored
#define TAB 9
#define TRASH 0xFF

/******** Flash Memory ********/
#define SECTOR_5      ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define SECTOR_6      ((uint32_t)0x08040000) /* Base @ of Sector 6, 128 Kbytes */
#define SECTOR_7      ((uint32_t)0x08060000) /* Base @ of Sector 7, 128 Kbytes */
#define SECTOR_8      ((uint32_t)0x08080000) /* Base @ of Sector 8, 128 Kbytes */
#define SECTOR_9      ((uint32_t)0x080A0000) /* Base @ of Sector 9, 128 Kbytes */
#define SECTOR_10     ((uint32_t)0x080C0000) /* Base @ of Sector 10, 128 Kbytes */
#define SECTOR_11     ((uint32_t)0x080E0000) /* Base @ of Sector 11, 128 Kbytes */
#define SECTOR_17     ((uint32_t)0x08120000) /* Base @ of Sector 5, 128 Kbytes */
#define SECTOR_18     ((uint32_t)0x08140000) /* Base @ of Sector 6, 128 Kbytes */
#define SECTOR_19     ((uint32_t)0x08160000) /* Base @ of Sector 7, 128 Kbytes */
#define SECTOR_20     ((uint32_t)0x08180000) /* Base @ of Sector 8, 128 Kbytes  */
#define SECTOR_21     ((uint32_t)0x081A0000) /* Base @ of Sector 9, 128 Kbytes  */
#define SECTOR_22     ((uint32_t)0x081C0000) /* Base @ of Sector 10, 128 Kbytes */
#define SECTOR_23     ((uint32_t)0x081E0000) /* Base @ of Sector 11, 128 Kbytes */

// Address where you want to store your data in flash
#define NUMBER_OF_SECTORS 14
#define MAX_PROTOCOLS_IN_SECTOR 3
#define PROTOCOL_SIZE 3000
#define MAX_LINES 100
#define MAX_LINE_LENGTH 32 //for linear movement it wont be more than 32 chars
#define MAX_TITLE_SIZE 12  //you can 10 chars, but also we need newline and null

/******** LCD DISPLAY ********/
#define BUTTON_WIDTH 160
#define BUTTON_HEIGHT 40
#define NUM_BUTTONS 3
#define Y_OFFSET 20
#define SCREEN_DELAY 50
#define MAX_QUEUE_SIZE 10

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

DMA2D_HandleTypeDef hdma2d;

I2C_HandleTypeDef hi2c3;

LTDC_HandleTypeDef hltdc;

SPI_HandleTypeDef hspi5;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim10;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart1;

SDRAM_HandleTypeDef hsdram1;

/* USER CODE BEGIN PV */
TS_StateTypeDef TS_State;           //store touchscreen state info here
static int count = 0;               //count for interrupt
static uint8_t checkTouchFlag = 0;  //interrupt flag for main loop

typedef enum {
	PAGE_MAIN, PAGE_SELECT, PAGE_QUEUE, PAGE_CONFIRMATION, PAGE_FINISH
} PageState;
PageState currentPage = PAGE_MAIN;

typedef enum {
	EMPTY = 0, NOT_EMPTY = 1
} ButtonStatus;

typedef struct {
	uint16_t x, y, w, h;
	char label[MAX_LINE_LENGTH];
	ButtonStatus status;
} Button;

Button buttons[NUM_BUTTONS] = { { 40, 50, BUTTON_WIDTH, BUTTON_HEIGHT,
		"Protocol 1", EMPTY }, { 40, 110, BUTTON_WIDTH, BUTTON_HEIGHT,
		"Protocol 2", EMPTY }, { 40, 170, BUTTON_WIDTH, BUTTON_HEIGHT,
		"Protocol 3", EMPTY } };

Button queueButton = { 10, 270, 80, 30, "Queue" }; //x, y, w, h, label
Button backButton = { 10, 270, 80, 30, "Back" }; //x, y, w, h, label
Button nextButton = { 150, 270, 80, 30, "Next" };
Button runButton = { 150, 270, 80, 30, "Run" };
Button confirmButton = { 70, 160, 120, 40, "Confirm" };
Button queueSelectButton = { 70, 60, 100, 40, "Queue" }; //x, y, w, h, label
Button selectButton = { 70, 120, 100, 40, "Run" };
Button deleteButton = { 70, 180, 100, 40, "Delete" };

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C3_Init(void);
static void MX_LTDC_Init(void);
static void MX_SPI5_Init(void);
static void MX_TIM1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM10_Init(void);
static void MX_UART5_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
void output_protocol(char line[MAX_LINES][MAX_LINE_LENGTH], int lines);

/****TOUCHSCREEN PRIVATE FUNCTIONS*******/
void erase_sector(uint32_t Sector);
void write_to_flash(const char *data, uint32_t flash_address);
uint8_t read_from_flash(char *output, uint32_t flash_address);
uint8_t storeProtocol(char gcode_file[MAX_LINES][MAX_LINE_LENGTH],
		uint32_t sector);
uint32_t sector_mapping(uint32_t sector);
uint8_t get_num_protocols_in_sector(uint32_t sector);
uint8_t get_num_lines_in_protocol(char protocol[MAX_LINES][MAX_LINE_LENGTH]);
uint8_t getFreeSector(void);
void transmitProtocol(uint32_t sector, uint32_t offset);
void queueProtocol(uint32_t sector, uint32_t offset);
void transmitQueuedProtocols(uint8_t queueSize);
uint32_t get_sector_address(uint32_t sector);
void write_number_to_flash(uint32_t flash_address, uint32_t number);
void DrawQueuePage(uint8_t queueSize);
void Touch_Init(void);
uint8_t HandleTouch(void);
void DrawMainPage(uint8_t page_num);
void DrawInfoPage(char protocolTitle[MAX_LINE_LENGTH]);
void DrawConfirmationPage(uint32_t sector, uint32_t offset);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/********** QR SCANNER HANDLED THROUGH USB CALL BACK FUNCTION *********/
int i = 0;
int j = 0;
static char queueBuffer[MAX_QUEUE_SIZE][MAX_LINES][MAX_LINE_LENGTH];
static uint8_t queueSize = 0;
static uint8_t pageNum = 1;
static uint8_t USB_BUSY = 0;
char qr_code_data[MAX_LINES][MAX_LINE_LENGTH] = { { '\0' } }; //static initializes strings with all null characters
HID_KEYBD_Info_TypeDef *Keyboard_Info;

/***DORJEE YOU HAVE TO CHANGE USBH_HID.H WHEN YOU GENERATE CODE****/
void USBH_HID_EventCallback(USBH_HandleTypeDef *phost) { //2.6s for 54 lines
	Keyboard_Info = USBH_HID_GetKeybdInfo(phost);
	char key = USBH_HID_GetASCIICode(Keyboard_Info);
	//disable the touchscreen interrupt functionality when usb transmission starts
	//after tab it calls itself one more time, so disable after i=0
	if (i == 1) {
		USB_BUSY = 1;
	}

	//store incoming chars. ignore null characters that come in for some reason
	if ((key != NULL_CHAR) && (key != SOH)) {
		qr_code_data[i][j] = key;
		j++;
	}
	//handle new line
	if (key == NEWLINE_CHAR) {
		j = 0;
		i++;
	}
	//tab is the end of the qr code
	if (key == TAB) {
		for (int a = 0; a < 15; a++) {
			//printf("%s", qr_code_data[a]);
		}
		__disable_irq();
		//output_protocol(qr_code_data, i + 1);
		//check if the title is (10 chars + newline char) or less
		if (strlen(qr_code_data[0]) < (MAX_TITLE_SIZE + 1)) {
			uint8_t freeSectorNumber = getFreeSector();
			//printf("free sector is %d\n", freeSectorNumber);
			//only store the protocol if all memory isnt full
			if (freeSectorNumber <= 10) { //i only got 10 pages rn
				//printf("why\n");
				storeProtocol(qr_code_data, freeSectorNumber);
				//go to the page num the new protocol is on
				pageNum = freeSectorNumber;
				DrawMainPage(pageNum);
				currentPage = PAGE_MAIN;
				for (int a = 0; a < MAX_LINES; a++) {
					for (int b = 0; b < MAX_LINE_LENGTH; b++) {
						qr_code_data[a][b] = '\0';
					}
				}

			}
			__enable_irq();
			USB_BUSY = 0;
		}
		i = 0;
		j = 0;
	}

}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_CRC_Init();
	MX_DMA2D_Init();
	MX_FMC_Init();
	MX_I2C3_Init();
	MX_LTDC_Init();
	MX_SPI5_Init();
	MX_TIM1_Init();
	MX_USART1_UART_Init();
	MX_USB_HOST_Init();
	MX_TIM10_Init();
	MX_UART5_Init();
	/* USER CODE BEGIN 2 */
	printf("USB and Touchscreen Test\n");

	/****** manually erase sectors ****/
//	erase_sector(1);
//	erase_sector(2);
//	erase_sector(3);
//	erase_sector(4);
//	erase_sector(5);
//	erase_sector(6);
//	erase_sector(7);
//	erase_sector(8);
//	erase_sector(9);
//	erase_sector(10);
	printf("Sectors may have been erased\n");

	/******* TOUCHSCREEN INIT**********/
	BSP_LCD_Init();
	BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER);
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	Touch_Init();

	/******** START PROGRAM ************/
	DrawMainPage(1);
	HAL_TIM_Base_Start_IT(&htim10);   //start Interrupt Timer

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */
		MX_USB_HOST_Process();

		/* USER CODE BEGIN 3 */

		if (checkTouchFlag) {
			HandleTouch();
			checkTouchFlag = 0;
		}
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief CRC Initialization Function
 * @param None
 * @retval None
 */
static void MX_CRC_Init(void) {

	/* USER CODE BEGIN CRC_Init 0 */

	/* USER CODE END CRC_Init 0 */

	/* USER CODE BEGIN CRC_Init 1 */

	/* USER CODE END CRC_Init 1 */
	hcrc.Instance = CRC;
	if (HAL_CRC_Init(&hcrc) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN CRC_Init 2 */

	/* USER CODE END CRC_Init 2 */

}

/**
 * @brief DMA2D Initialization Function
 * @param None
 * @retval None
 */
static void MX_DMA2D_Init(void) {

	/* USER CODE BEGIN DMA2D_Init 0 */

	/* USER CODE END DMA2D_Init 0 */

	/* USER CODE BEGIN DMA2D_Init 1 */

	/* USER CODE END DMA2D_Init 1 */
	hdma2d.Instance = DMA2D;
	hdma2d.Init.Mode = DMA2D_M2M;
	hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
	hdma2d.Init.OutputOffset = 0;
	hdma2d.LayerCfg[1].InputOffset = 0;
	hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
	hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
	hdma2d.LayerCfg[1].InputAlpha = 0;
	if (HAL_DMA2D_Init(&hdma2d) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN DMA2D_Init 2 */

	/* USER CODE END DMA2D_Init 2 */

}

/**
 * @brief I2C3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C3_Init(void) {

	/* USER CODE BEGIN I2C3_Init 0 */

	/* USER CODE END I2C3_Init 0 */

	/* USER CODE BEGIN I2C3_Init 1 */

	/* USER CODE END I2C3_Init 1 */
	hi2c3.Instance = I2C3;
	hi2c3.Init.ClockSpeed = 100000;
	hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c3.Init.OwnAddress1 = 0;
	hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c3.Init.OwnAddress2 = 0;
	hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c3) != HAL_OK) {
		Error_Handler();
	}

	/** Configure Analogue filter
	 */
	if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE)
			!= HAL_OK) {
		Error_Handler();
	}

	/** Configure Digital filter
	 */
	if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C3_Init 2 */

	/* USER CODE END I2C3_Init 2 */

}

/**
 * @brief LTDC Initialization Function
 * @param None
 * @retval None
 */
static void MX_LTDC_Init(void) {

	/* USER CODE BEGIN LTDC_Init 0 */

	/* USER CODE END LTDC_Init 0 */

	LTDC_LayerCfgTypeDef pLayerCfg = { 0 };

	/* USER CODE BEGIN LTDC_Init 1 */

	/* USER CODE END LTDC_Init 1 */
	hltdc.Instance = LTDC;
	hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
	hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
	hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
	hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
	hltdc.Init.HorizontalSync = 9;
	hltdc.Init.VerticalSync = 1;
	hltdc.Init.AccumulatedHBP = 29;
	hltdc.Init.AccumulatedVBP = 3;
	hltdc.Init.AccumulatedActiveW = 269;
	hltdc.Init.AccumulatedActiveH = 323;
	hltdc.Init.TotalWidth = 279;
	hltdc.Init.TotalHeigh = 327;
	hltdc.Init.Backcolor.Blue = 0;
	hltdc.Init.Backcolor.Green = 0;
	hltdc.Init.Backcolor.Red = 0;
	if (HAL_LTDC_Init(&hltdc) != HAL_OK) {
		Error_Handler();
	}
	pLayerCfg.WindowX0 = 0;
	pLayerCfg.WindowX1 = 240;
	pLayerCfg.WindowY0 = 0;
	pLayerCfg.WindowY1 = 320;
	pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
	pLayerCfg.Alpha = 255;
	pLayerCfg.Alpha0 = 0;
	pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
	pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
	pLayerCfg.FBStartAdress = 0xD0000000;
	pLayerCfg.ImageWidth = 240;
	pLayerCfg.ImageHeight = 320;
	pLayerCfg.Backcolor.Blue = 0;
	pLayerCfg.Backcolor.Green = 0;
	pLayerCfg.Backcolor.Red = 0;
	if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN LTDC_Init 2 */

	/* USER CODE END LTDC_Init 2 */

}

/**
 * @brief SPI5 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI5_Init(void) {

	/* USER CODE BEGIN SPI5_Init 0 */

	/* USER CODE END SPI5_Init 0 */

	/* USER CODE BEGIN SPI5_Init 1 */

	/* USER CODE END SPI5_Init 1 */
	/* SPI5 parameter configuration*/
	hspi5.Instance = SPI5;
	hspi5.Init.Mode = SPI_MODE_MASTER;
	hspi5.Init.Direction = SPI_DIRECTION_2LINES;
	hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi5.Init.NSS = SPI_NSS_SOFT;
	hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
	hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi5.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi5) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI5_Init 2 */

	/* USER CODE END SPI5_Init 2 */

}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void) {

	/* USER CODE BEGIN TIM1_Init 0 */

	/* USER CODE END TIM1_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM1_Init 1 */

	/* USER CODE END TIM1_Init 1 */
	htim1.Instance = TIM1;
	htim1.Init.Prescaler = 167;
	htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim1.Init.Period = 65535;
	htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim1.Init.RepetitionCounter = 0;
	htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM1_Init 2 */

	/* USER CODE END TIM1_Init 2 */

}

/**
 * @brief TIM10 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM10_Init(void) {

	/* USER CODE BEGIN TIM10_Init 0 */

	/* USER CODE END TIM10_Init 0 */

	/* USER CODE BEGIN TIM10_Init 1 */

	/* USER CODE END TIM10_Init 1 */
	htim10.Instance = TIM10;
	htim10.Init.Prescaler = 1680 - 1;
	htim10.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim10.Init.Period = 10000;
	htim10.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim10.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(&htim10) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN TIM10_Init 2 */

	/* USER CODE END TIM10_Init 2 */

}

/**
 * @brief UART5 Initialization Function
 * @param None
 * @retval None
 */
static void MX_UART5_Init(void) {

	/* USER CODE BEGIN UART5_Init 0 */

	/* USER CODE END UART5_Init 0 */

	/* USER CODE BEGIN UART5_Init 1 */

	/* USER CODE END UART5_Init 1 */
	huart5.Instance = UART5;
	huart5.Init.BaudRate = 115200;
	huart5.Init.WordLength = UART_WORDLENGTH_8B;
	huart5.Init.StopBits = UART_STOPBITS_1;
	huart5.Init.Parity = UART_PARITY_NONE;
	huart5.Init.Mode = UART_MODE_TX_RX;
	huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart5.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart5) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN UART5_Init 2 */

	/* USER CODE END UART5_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void) {

	/* USER CODE BEGIN FMC_Init 0 */

	/* USER CODE END FMC_Init 0 */

	FMC_SDRAM_TimingTypeDef SdramTiming = { 0 };

	/* USER CODE BEGIN FMC_Init 1 */

	/* USER CODE END FMC_Init 1 */

	/** Perform the SDRAM1 memory initialization sequence
	 */
	hsdram1.Instance = FMC_SDRAM_DEVICE;
	/* hsdram1.Init */
	hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
	hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
	hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
	hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
	hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
	hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
	hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
	hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
	hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
	hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;
	/* SdramTiming */
	SdramTiming.LoadToActiveDelay = 2;
	SdramTiming.ExitSelfRefreshDelay = 7;
	SdramTiming.SelfRefreshTime = 4;
	SdramTiming.RowCycleDelay = 7;
	SdramTiming.WriteRecoveryTime = 3;
	SdramTiming.RPDelay = 2;
	SdramTiming.RCDDelay = 2;

	if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK) {
		Error_Handler();
	}

	/* USER CODE BEGIN FMC_Init 2 */

	/* USER CODE END FMC_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */
	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, NCS_MEMS_SPI_Pin | CSX_Pin | OTG_FS_PSO_Pin,
			GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(ACP_RST_GPIO_Port, ACP_RST_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD, RDX_Pin | WRX_DCX_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOG, LD3_Pin | LD4_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pins : NCS_MEMS_SPI_Pin CSX_Pin OTG_FS_PSO_Pin */
	GPIO_InitStruct.Pin = NCS_MEMS_SPI_Pin | CSX_Pin | OTG_FS_PSO_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : B1_Pin MEMS_INT1_Pin MEMS_INT2_Pin TP_INT1_Pin */
	GPIO_InitStruct.Pin = B1_Pin | MEMS_INT1_Pin | MEMS_INT2_Pin | TP_INT1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : ACP_RST_Pin */
	GPIO_InitStruct.Pin = ACP_RST_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(ACP_RST_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : OTG_FS_OC_Pin */
	GPIO_InitStruct.Pin = OTG_FS_OC_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(OTG_FS_OC_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : BOOT1_Pin */
	GPIO_InitStruct.Pin = BOOT1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : TE_Pin */
	GPIO_InitStruct.Pin = TE_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(TE_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : RDX_Pin WRX_DCX_Pin */
	GPIO_InitStruct.Pin = RDX_Pin | WRX_DCX_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/*Configure GPIO pin : PD5 */
	GPIO_InitStruct.Pin = GPIO_PIN_5;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/*Configure GPIO pins : LD3_Pin LD4_Pin */
	GPIO_InitStruct.Pin = LD3_Pin | LD4_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */
	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/******************* DORJEE'S FUNCTIONS ***************/

//debugging print of qr scanned data
void output_protocol(char line[MAX_LINES][MAX_LINE_LENGTH], int lines) {
	for (int i = 0; i < lines; i++) {
		printf("%s\n", line[i]);
	}
}
int _write(int file, char *ptr, int len) {
	HAL_UART_Transmit(&huart1, (uint8_t*) ptr, len, HAL_MAX_DELAY);
	return len;
}

/******************** Flash Memory ********************/

/**
 * @brief: Erase the memory from an entire sector
 * @param Sector: The sector number
 * @retval: none
 */
void erase_sector(uint32_t Sector) {
	HAL_FLASH_Unlock();  // Unlock flash to enable erasing
	FLASH_Erase_Sector(sector_mapping(Sector), VOLTAGE_RANGE_3);
	HAL_FLASH_Lock();  // Lock flash after erasing
}

/**
 * @brief: Write a string to flash memory
 * @param data: string to be stored in flash memory
 * @param flash_address: address of string in flash memory that will be stored
 * @retval: none
 * Note: PLEASE UNLOCK AND LOCK FLASH BEFORE AND AFTER USING THIS FUNCTION,
 * IT DOES NOT DO IT ITSELF
 */
void write_to_flash(const char *data, uint32_t flash_address) {
	//HAL_FLASH_Unlock();  // Unlock flash for writing

	for (uint8_t i = 0; i < strlen(data); i++) {
		//printf("%c", data[i]);
		HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, flash_address + i, data[i]); // Write to flash
	}

	//HAL_FLASH_Lock();  // Lock flash after writing
}

/**
 * @brief: Read a string from flash memory. If failure, don't read
 * @param output: store the string in an output to be used
 * @param flash_address: address of string in flash memory that will be read
 * @retval: failure if its reading trash from flash memory.
 * it literally does not read from flash btw if it detects 0xFF at the start
 * that means your output buffer will not be updated
 */
uint8_t read_from_flash(char *output, uint32_t flash_address) {
	// Read byte by byte from flash memory, get a whole string in output variable
	for (uint8_t i = 0; i < MAX_LINE_LENGTH; i++) {
		char key = *(char*) (flash_address + i);
		//check if reading trash (0xFF) from flash memory
		if (key == 0xFF) {
			if (i == 0) {
				return 0;
			}
		} else {
			output[i] = key;
		}
	}
	output[strlen(output)] = '\0'; //add null to the end of string for printing
	//printf("output is %s and strlen: %d\n", output, strlen(output));

	//error handling. G-codes are less than 32 chars, anything more is trash
	if (strlen(output) < MAX_LINE_LENGTH) {
		return 1;
	} else {
		return 0;
	}

}

/**
 * @brief: Store array of G-code commands (protocol) into flash memory
 * @param num: number of G-code commands to store
 * @param gcode_file: array of G-code commands
 * @param sector: which sector to store protocol in
 * @retval: failure if sector is "full"
 */
uint8_t storeProtocol(char new_protocol[MAX_LINES][MAX_LINE_LENGTH],
		uint32_t sector) {

	//initialize variables
	int i = 0;  //protocol index
	int j = 0;  //line index
	char protocolStorage[MAX_PROTOCOLS_IN_SECTOR][MAX_LINES][MAX_LINE_LENGTH] =
			{ { { '\0' } } };
	uint32_t flash_address = get_sector_address(sector);

	//check if all sectors are full before storing a protocol
	uint8_t num_protocols = get_num_protocols_in_sector(sector);
	if (num_protocols == MAX_PROTOCOLS_IN_SECTOR) {
		return 0;
	}

	//read existing protocols in sector and put into the temp protocol storage
	for (i = 0; i < num_protocols; i++) {
		for (j = 0; j < MAX_LINES; j++) {
			//get the address for the current line in the current protocol
			uint32_t temp_address = flash_address + i * PROTOCOL_SIZE
					+ j * MAX_LINE_LENGTH;
			//read lines from memory into buffer until you reach garbage
			read_from_flash(protocolStorage[i][j], temp_address); //only updates protocolStorage if its a valid line from memory
		}
	}
	//output: we now have the existing protocols in the buffer.
	//they only have their first number of lines in. the rest of lines are null

	//put the new protocol into the temp protocol storage
	for (j = 0; j < MAX_LINES; j++) {
		//copy each line into the buffer from new protocol, should be null terminated from qr scanner
		if (new_protocol[j][0] != '\0') {
			strcpy(protocolStorage[i][j], new_protocol[j]);
		}
	}
	//we now have the new protocol in the buffer.
	//it only has the first number of lines in. the rest of lines are null

	output_protocol(protocolStorage[0], 10);
	output_protocol(protocolStorage[1], 10);

	//write existing protocols and new protocol to flash memory, string-by-string
	erase_sector(sector); //erase sector before writing
	HAL_FLASH_Unlock(); // Unlock flash for writing. note: the erase_sector() unlocks and locks itself
	for (i = 0; i < (num_protocols + 1); i++) {
		for (j = 0; j < MAX_LINES; j++) {
			uint32_t temp_address = flash_address + i * PROTOCOL_SIZE
					+ j * MAX_LINE_LENGTH;
			//write all strings that dont start with null to flash memory
			if (protocolStorage[i][j][0] != NULL) {
				write_to_flash(protocolStorage[i][j], temp_address);
			}

//			//if first character of a line is null from qr scanner or trash from flash, ignore it
//			if ((new_protocol[j][0] != '\0') && (new_protocol[j][0] != 0xFF)) {
//				uint32_t temp_address = flash_address + i * PROTOCOL_SIZE
//						+ j * MAX_LINE_LENGTH;
//				//printf("len: %d and data: %s", strlen(protocolStorage[i][j]), protocolStorage[i][j]);
//				write_to_flash(protocolStorage[i][j], temp_address);
//			}
		}
	}
	HAL_FLASH_Lock();  // Unlock flash for writing

	//output:flash memory now has the lines of protocol storage in there
	//the remaining space is still 0xFF in flash
	return 1;
}

//return number of protocols in the sector
//NOTE: it doesnt actually do that. it checks the top of the sector first
//if there is no protocol there. it will just assume the rest is empty.
uint8_t get_num_protocols_in_sector(uint32_t sector) {
	uint32_t flash_address = get_sector_address(sector);

	uint8_t i = 0;
	for (i = 0; i < MAX_PROTOCOLS_IN_SECTOR; i++) {
		uint8_t first_char = *(uint8_t*) (flash_address + i * PROTOCOL_SIZE);
		//memory in flash is default 0xFF
		if (first_char == 0xFF) {
			return i;
		}
	}
	return i;
}

//check each sector sequentially for any space.
//return the first sector number you find that has space
uint8_t getFreeSector(void) {
	uint8_t i = 1;
	//check every sector starting at 1
	for (i = 1; i < (NUMBER_OF_SECTORS + 1); i++) {
		//check each potential protocol slot in the current sector
		for (uint8_t j = 0; j < MAX_PROTOCOLS_IN_SECTOR; j++) {
			//check first char of the given protocol in the current sector
			uint32_t flash_address = get_sector_address(i) + j * PROTOCOL_SIZE;
			char first_char = *(char*) (flash_address);
			//if memory address is empty, this sector is free
			if (first_char == 0xFF) {
				return i;
			}
		}
	}
	//if somehow all sectors are full, return 15 or higher (16 in this case)
	return i + 1;
}

/**
 * @brief: Transmit protocol from flash memory to ESP32 via UART string-by-string
 * @param sector: which sector the protocol is stored in
 */
void transmitProtocol(uint32_t sector, uint32_t offset) {
	char output[MAX_LINE_LENGTH] = { '\0' };
	uint32_t flash_address = get_sector_address(sector) + offset * PROTOCOL_SIZE;

	//printf("\nNew Protocol\n"); //just for debugging

	//we need to process up to 1028 G-code commands
	//but if we run into an invalid string, gcode file is over
	for (uint8_t i = 0; i < MAX_LINES; i++) {
		//an alternative to resetting the temp buffer is to only read up to newline
		memset(output, 0, MAX_LINE_LENGTH); // Sets all elements of buffer to 0
		if (read_from_flash(output, flash_address + i * MAX_LINE_LENGTH)) {
			//printf("%s", output);
			HAL_UART_Transmit(&huart1, (uint8_t*) output, strlen(output),
			HAL_MAX_DELAY);
			HAL_UART_Transmit(&huart5, (uint8_t*) output, strlen(output),
			HAL_MAX_DELAY);
		}
	}
}

/**
 * @brief: Queue protocol from flash memory to RAM buffer
 * @param sector: which sector the protocol is stored in
 */
void queueProtocol(uint32_t sector, uint32_t offset) {
	char output[MAX_LINE_LENGTH] = { '\0' };
	uint32_t flash_address = get_sector_address(sector) + offset * PROTOCOL_SIZE;

	for (uint8_t i = 0; i < MAX_LINES; i++) {
		memset(output, 0, MAX_LINE_LENGTH); // Sets all elements of buffer to 0
		if (read_from_flash(output, flash_address + i * MAX_LINE_LENGTH)) {
			strcpy(queueBuffer[queueSize][i], output);
		}
	}
}

/**
 * @brief: Transmit queued protocols from RAM buffer to ESP32 via UART string-by-string
 * @param queueSize:
 */
void transmitQueuedProtocols(uint8_t queueSize) {
	for (uint8_t i = 0; i < queueSize; i++) {
		for (uint8_t j = 0; j < MAX_LINES; j++) {
			HAL_UART_Transmit(&huart1, (uint8_t*) queueBuffer[i][j],
					strlen(queueBuffer[i][j]), HAL_MAX_DELAY);
		}
	}
}

/**
 * @brief: Delete protocol from flash memory to ESP32 via UART string-by-string
 * @param sector: which sector the protocol is stored in
 * @param offset: the exact slot in the sector the protocol is stored in
 * @retval: none
 */
void deleteProtocol(uint32_t sector, uint32_t offset) {
	//initialize variables
	uint8_t new_offset = offset + 1; //have it range from 1-3
	int i = 0;
	int j = 0; //indices for moving through flash memory
	char protocolStorage[MAX_PROTOCOLS_IN_SECTOR][MAX_LINES][MAX_LINE_LENGTH] =
			{ { { '\0' } } };
	uint32_t flash_address = get_sector_address(sector);
	char trashLine[MAX_LINE_LENGTH];
	memset(trashLine, 0xFF, sizeof(trashLine));
	char nullLine[MAX_LINE_LENGTH];
	memset(nullLine, 0x00, sizeof(nullLine));
	uint8_t num_protocols = get_num_protocols_in_sector(sector);

	int a = 0;
	int b = 0; //separate indices for protocolstorage

	//copy all protocols in sector to local buffer
	for (i = 0; i < MAX_PROTOCOLS_IN_SECTOR; i++) {
		if (i != offset) {
			//copy all lines of the protocol into the buffer
			for (j = 0; j < MAX_LINES; j++) {
				//get the address for the current line in the current protocol
				uint32_t temp_address = flash_address + i * PROTOCOL_SIZE
						+ j * MAX_LINE_LENGTH;
				//read lines from memory into buffer until you reach garbage
				read_from_flash(protocolStorage[a][b], temp_address); //only updates protocolStorage if its a valid line from memory
				b++;
			}
			a++;
			b = 0;
		}
	}

	//write existing protocols except the one to be deleted back to flash
	erase_sector(sector); //erase sector before writing
	HAL_FLASH_Unlock(); // Unlock flash for writing. note: the erase_sector() unlocks and locks itself
	for (i = 0; i < MAX_PROTOCOLS_IN_SECTOR; i++) {
		for (j = 0; j < MAX_LINES; j++) {
			//if first character of a line is null from qr scanner or trash from flash, ignore it
			uint32_t temp_address = flash_address + i * PROTOCOL_SIZE
					+ j * MAX_LINE_LENGTH;
			write_to_flash(protocolStorage[i][j], temp_address);

		}
	}
	HAL_FLASH_Lock();  // Unlock flash for writing
}

// Function to write a uint32_t number to flash memory
void write_number_to_flash(uint32_t flash_address, uint32_t number) {
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_address, number);
}

/******************** Touchscreen *********************/

void Touch_Init(void) {
	if (BSP_TS_Init(240, 320) != TS_OK) {
		BSP_LCD_DisplayStringAt(0, 10, (uint8_t*) "Failure", CENTER_MODE);
		printf("Touchscreen Initialization failure\n");
	} else {
		BSP_LCD_DisplayStringAt(0, 300, (uint8_t*) "Success", CENTER_MODE);
		printf("Touchscreen Initialization success\n");
	}
}
/**
 * @brief: Draw main page and handle button label and status updates
 * @param: page_num: determines which page should be drawn from 1-10
 * @retval: none
 */
void DrawMainPage(uint8_t page_num) {
	//Draw "Protocol Title" box
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	char empty[20] = "Empty\n"; //strings from flash memory come with newline at end
	char pageTitle[20] = "";
	sprintf(pageTitle, "Page %d", page_num);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetFont(&Font20);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(10, 10, (uint8_t*) pageTitle, CENTER_MODE);

	//get button label info from flash memory
	char readBuffer1[MAX_LINE_LENGTH] = ""; //initialize with nulls
	char readBuffer2[MAX_LINE_LENGTH] = ""; //initialize with nulls
	char readBuffer3[MAX_LINE_LENGTH] = ""; //initialize with nulls

	//if the protocol exists, read from it. else, name it empty
	if (read_from_flash(readBuffer1, get_sector_address(page_num))) {
		strcpy(buttons[0].label, readBuffer1);
		buttons[0].status = NOT_EMPTY;
	} else {
		strcpy(buttons[0].label, empty);
		buttons[0].status = EMPTY;
	}
	if (read_from_flash(readBuffer2,
			get_sector_address(page_num) + PROTOCOL_SIZE)) {
		strcpy(buttons[1].label, readBuffer2);
		buttons[1].status = NOT_EMPTY;
	} else {
		strcpy(buttons[1].label, empty);
		buttons[1].status = EMPTY;
	}
	if (read_from_flash(readBuffer3,
			get_sector_address(page_num) + 2 * PROTOCOL_SIZE)) {
		strcpy(buttons[2].label, readBuffer3);
		buttons[2].status = NOT_EMPTY;
	} else {
		strcpy(buttons[2].label, empty);
		buttons[2].status = EMPTY;
	}

	// Draw buttons
	BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
	for (int i = 0; i < NUM_BUTTONS; i++) {
		BSP_LCD_FillRect(buttons[i].x, buttons[i].y, buttons[i].w,
				buttons[i].h);
		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
		if (buttons[i].label[10] == NEWLINE_CHAR) {
		}
		buttons[i].label[strlen(buttons[i].label) - 1] = ' '; //dont display null terminator
		BSP_LCD_DisplayStringAt(buttons[i].x + 10, buttons[i].y + 10,
				(uint8_t*) buttons[i].label, LEFT_MODE);
		BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
	}

	// Draw "Next" button
	if (page_num != 10) {
		BSP_LCD_SetTextColor(LCD_COLOR_RED);
		BSP_LCD_FillRect(nextButton.x, nextButton.y, nextButton.w,
				nextButton.h);
		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		BSP_LCD_SetBackColor(LCD_COLOR_RED);
		BSP_LCD_DisplayStringAt(nextButton.x + 10, nextButton.y + 10,
				(uint8_t*) nextButton.label, LEFT_MODE);
	}

	// Draw "Back" button
	if (page_num != 1) {
		BSP_LCD_SetTextColor(LCD_COLOR_RED);
		BSP_LCD_FillRect(backButton.x, backButton.y, backButton.w,
				backButton.h);
		BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
		BSP_LCD_SetBackColor(LCD_COLOR_RED);
		BSP_LCD_DisplayStringAt(backButton.x + 10, backButton.y + 10,
				(uint8_t*) backButton.label, LEFT_MODE);
	}

	//draw "Queue" button on page 1
	if (page_num == 1) {
		BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
		BSP_LCD_FillRect(queueButton.x, queueButton.y, queueButton.w,
				queueButton.h);
		BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
		BSP_LCD_SetBackColor(LCD_COLOR_YELLOW);
		BSP_LCD_DisplayStringAt(queueButton.x + 10, queueButton.y + 10,
				(uint8_t*) queueButton.label, LEFT_MODE);
	}
}

void DrawInfoPage(char protocolTitle[MAX_LINE_LENGTH]) {
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	//Display protocol name at top of screen
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(10, 10, (uint8_t*) protocolTitle, CENTER_MODE);

	//Draw "QueueSelect" button on select page
	BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
	BSP_LCD_FillRect(queueSelectButton.x, queueSelectButton.y,
			queueSelectButton.w, queueSelectButton.h);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
	BSP_LCD_DisplayStringAt(queueSelectButton.x + 10, queueSelectButton.y + 10,
			(uint8_t*) queueSelectButton.label, LEFT_MODE);

	//Draw "Select" button
	BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
	BSP_LCD_FillRect(selectButton.x, selectButton.y, selectButton.w,
			selectButton.h);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
	BSP_LCD_DisplayStringAt(selectButton.x + 10, selectButton.y + 10,
			(uint8_t*) selectButton.label, LEFT_MODE);

	//Draw "Delete" button
	BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
	BSP_LCD_FillRect(deleteButton.x, deleteButton.y, deleteButton.w,
			deleteButton.h);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
	BSP_LCD_DisplayStringAt(deleteButton.x + 10, deleteButton.y + 10,
			(uint8_t*) deleteButton.label, LEFT_MODE);

	// Draw "Back" button
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_FillRect(backButton.x, backButton.y, backButton.w, backButton.h);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(backButton.x + 10, backButton.y + 10,
			(uint8_t*) backButton.label, LEFT_MODE);
}

void DrawQueuePage(uint8_t queueSize) {
	BSP_LCD_Clear(LCD_COLOR_WHITE);
	char pageTitle[20] = "";
	sprintf(pageTitle, "Queue Size: %d", queueSize);
	//Display Queue at top of the screen
	BSP_LCD_SetFont(&Font20);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(10, 10, (uint8_t*) pageTitle, CENTER_MODE);

	//display protocols in queue here
	BSP_LCD_SetFont(&Font16);
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
	char format[20] = "";
	for (uint8_t i = 0; i < queueSize; i++) {
		sprintf(format, "%d.) ", i + 1);
		BSP_LCD_DisplayStringAt(10, 50 + i * 20, (uint8_t*) format, LEFT_MODE);
		BSP_LCD_DisplayStringAt(60, 50 + i * 20, (uint8_t*) queueBuffer[i][0],
				LEFT_MODE);
	}
	BSP_LCD_SetFont(&Font20);

	//Draw "Run" button
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_FillRect(runButton.x, runButton.y, runButton.w, runButton.h);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(runButton.x + 10, runButton.y + 10,
			(uint8_t*) runButton.label, LEFT_MODE);

	// Draw "Back" button
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_FillRect(backButton.x, backButton.y, backButton.w, backButton.h);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(backButton.x + 10, backButton.y + 10,
			(uint8_t*) backButton.label, LEFT_MODE);
}

void DrawConfirmationPage(uint32_t sector, uint32_t offset) {
	BSP_LCD_Clear(LCD_COLOR_WHITE);

	//display confirmation text
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
	BSP_LCD_DisplayStringAt(10, 10, (uint8_t*) "Confirm Delete:", LEFT_MODE);

	//Display protocol name for deletion
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
	BSP_LCD_DisplayStringAt(10, 30, (uint8_t*) buttons[offset].label,
			CENTER_MODE);

	//Draw "Confirm" button
	BSP_LCD_SetTextColor(LCD_COLOR_BLUE);
	BSP_LCD_FillRect(confirmButton.x, confirmButton.y, confirmButton.w,
			confirmButton.h);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_BLUE);
	BSP_LCD_DisplayStringAt(confirmButton.x + 10, confirmButton.y + 10,
			(uint8_t*) confirmButton.label, LEFT_MODE);

	// Draw "Back" button
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_FillRect(backButton.x, backButton.y, backButton.w, backButton.h);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(backButton.x + 10, backButton.y + 10,
			(uint8_t*) backButton.label, LEFT_MODE);
}

void DrawPageFinish() {
	BSP_LCD_Clear(LCD_COLOR_WHITE);

	//Draw "Success!" box
	BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(selectButton.x + 10, selectButton.y + 10,
			(uint8_t*) "Success!", LEFT_MODE);

	// Draw "Back" button
	BSP_LCD_SetTextColor(LCD_COLOR_RED);
	BSP_LCD_FillRect(backButton.x, backButton.y, backButton.w, backButton.h);
	BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
	BSP_LCD_SetBackColor(LCD_COLOR_RED);
	BSP_LCD_DisplayStringAt(backButton.x + 10, backButton.y + 10,
			(uint8_t*) "Done", LEFT_MODE);
}

//fix this function and give it a description
uint8_t HandleTouch(void) {
	//store the coordinates of the user touch in "TS_State"
	BSP_TS_GetState(&TS_State);
	//do nothing if no touch detect
	if (!TS_State.TouchDetected) {
		return 0;
	}

	//init variables if a touch is detected
	char Empty_str[20] = "Empty\n"; //strings from flash memory come with newline at end
	static uint8_t protocol_num;
	static uint8_t protocol_offset = 0;
	//the y value has issues of being flipped and also needing to click below button
	TS_State.Y = (BSP_LCD_GetYSize() - TS_State.Y) - Y_OFFSET;

	//start state machine for touch handling
	switch (currentPage) {
	case PAGE_MAIN:
		//back button
		if (TS_State.X >= backButton.x
				&& TS_State.X <= (backButton.x + backButton.w)
				&& TS_State.Y >= backButton.y
				&& TS_State.Y <= (backButton.y + backButton.h)
				&& (pageNum != 1)) {
			pageNum--;
			DrawMainPage(pageNum);
			if (pageNum == 1) {
				//next page wont register touch until you let go
				while (TS_State.TouchDetected) {
					BSP_TS_GetState(&TS_State);
					if (!TS_State.TouchDetected) {
					}
					HAL_Delay(20); //WOOOO CANT TOUCH UNLESS YOU RELASE. the clk speed is super fast, need delay. ok if you slide the pen it breaks, but that dont count
				}
			}
			HAL_Delay(SCREEN_DELAY);
		}
		//next button
		if (TS_State.X >= nextButton.x
				&& TS_State.X <= (nextButton.x + nextButton.w)
				&& TS_State.Y >= nextButton.y
				&& TS_State.Y <= (nextButton.y + nextButton.h)
				&& (pageNum != 10)) {
			pageNum++;
			DrawMainPage(pageNum);
			HAL_Delay(SCREEN_DELAY);
		}
		//queue button
		if (TS_State.X >= queueButton.x
				&& TS_State.X <= (queueButton.x + queueButton.w)
				&& TS_State.Y >= queueButton.y
				&& TS_State.Y <= (queueButton.y + queueButton.h)
				&& (pageNum == 1)) {
			DrawQueuePage(queueSize);
			currentPage = PAGE_QUEUE;
			//next page wont register touch until you let go
			while (TS_State.TouchDetected) {
				BSP_TS_GetState(&TS_State);
				if (!TS_State.TouchDetected) {
				}
				HAL_Delay(20); //WOOOO CANT TOUCH UNLESS YOU RELASE. the clk speed is super fast, need delay. ok if you slide the pen it breaks, but that dont count
			}
			HAL_Delay(SCREEN_DELAY);
		}
		//3 protocol buttons
		for (int i = 0; i < NUM_BUTTONS; i++) {
			if (TS_State.X >= buttons[i].x
					&& TS_State.X <= (buttons[i].x + buttons[i].w)
					&& TS_State.Y >= buttons[i].y
					&& TS_State.Y <= (buttons[i].y + buttons[i].h)) {
				//check which button has been pressed
				if (i == 0) {
					protocol_num = 1;
					protocol_offset = 0;
				} else if (i == 1) {
					protocol_num = 1;
					protocol_offset = 1;
				} else if (i == 2) {
					protocol_num = 1;
					protocol_offset = 2;
				}
				//dont draw next page if the button says "Empty"
				if (buttons[i].status == NOT_EMPTY) {
					currentPage = PAGE_SELECT;
					DrawInfoPage(buttons[i].label);
				}
				//next page wont register touch until you let go
				while (TS_State.TouchDetected) {
					BSP_TS_GetState(&TS_State);
					if (!TS_State.TouchDetected) {
					}
					HAL_Delay(20); //WOOOO CANT TOUCH UNLESS YOU RELASE. the clk speed is super fast, need delay. ok if you slide the pen it breaks, but that dont count
				}
				return;
			}
		}
		break;
	case PAGE_SELECT:
		//back button
		if (TS_State.X >= backButton.x
				&& TS_State.X <= (backButton.x + backButton.w)
				&& TS_State.Y >= backButton.y
				&& TS_State.Y <= (backButton.y + backButton.h)) {
			currentPage = PAGE_MAIN;
			DrawMainPage(pageNum);
		}
		//queueSelect button
		if (TS_State.X >= queueSelectButton.x
				&& TS_State.X <= (queueSelectButton.x + queueSelectButton.w)
				&& TS_State.Y >= queueSelectButton.y
				&& TS_State.Y <= (queueSelectButton.y + queueSelectButton.h)) {
			HAL_Delay(20);
			//store the protocol in queueBuffer
			if (queueSize < MAX_QUEUE_SIZE) {
				queueProtocol(pageNum, protocol_offset);
				queueSize++;
				currentPage = PAGE_QUEUE;
				DrawQueuePage(queueSize);
			} else {
				//handle queue buffer being full
			}
		}
		//select button
		if (TS_State.X >= selectButton.x
				&& TS_State.X <= (selectButton.x + selectButton.w)
				&& TS_State.Y >= selectButton.y
				&& TS_State.Y <= (selectButton.y + selectButton.h)) {
			HAL_Delay(20);
			//transmit protocol and move to finish page
			transmitProtocol(pageNum, protocol_offset);
			currentPage = PAGE_FINISH;
			DrawPageFinish();
		}
		//delete button
		if (TS_State.X >= deleteButton.x
				&& TS_State.X <= (deleteButton.x + deleteButton.w)
				&& TS_State.Y >= deleteButton.y
				&& TS_State.Y <= (deleteButton.y + deleteButton.h)) {
			//move to delete confirmation page
			currentPage = PAGE_CONFIRMATION;
			DrawConfirmationPage(pageNum, protocol_offset);
		}
		//next page wont register touch until you let go
		while (TS_State.TouchDetected) {
			BSP_TS_GetState(&TS_State);
			if (!TS_State.TouchDetected) {
			}
			HAL_Delay(20); //WOOOO CANT TOUCH UNLESS YOU RELASE. the clk speed is super fast, need delay. ok if you slide the pen it breaks, but that dont count
		}
		break;
	case PAGE_QUEUE:
		//back button
		if (TS_State.X >= backButton.x
				&& TS_State.X <= (backButton.x + backButton.w)
				&& TS_State.Y >= backButton.y
				&& TS_State.Y <= (backButton.y + backButton.h)) {
			currentPage = PAGE_MAIN;
			DrawMainPage(pageNum);
			//next page wont register touch until you let go
			while (TS_State.TouchDetected) {
				BSP_TS_GetState(&TS_State);
				if (!TS_State.TouchDetected) {
				}
				HAL_Delay(20); //WOOOO CANT TOUCH UNLESS YOU RELASE. the clk speed is super fast, need delay. ok if you slide the pen it breaks, but that dont count
			}
		}
		//run button
		if (TS_State.X >= runButton.x
				&& TS_State.X <= (runButton.x + runButton.w)
				&& TS_State.Y >= runButton.y
				&& TS_State.Y <= (runButton.y + runButton.h)) {
			HAL_Delay(20);
			//handle queue functionality here dorjee
			transmitQueuedProtocols(queueSize);
			queueSize = 0;
			currentPage = PAGE_FINISH;
			DrawPageFinish();
		}
		break;
	case PAGE_CONFIRMATION:
		//back button
		if (TS_State.X >= backButton.x
				&& TS_State.X <= (backButton.x + backButton.w)
				&& TS_State.Y >= backButton.y
				&& TS_State.Y <= (backButton.y + backButton.h)) {
			currentPage = PAGE_SELECT;
			DrawInfoPage(buttons[i].label);
		}
		//confirm button
		if (TS_State.X >= confirmButton.x
				&& TS_State.X <= (confirmButton.x + confirmButton.w)
				&& TS_State.Y >= confirmButton.y
				&& TS_State.Y <= (confirmButton.y + confirmButton.h)) {
			HAL_Delay(20);
			//delete protocol and go back to main page
			deleteProtocol(pageNum, protocol_offset);
			currentPage = PAGE_MAIN;
			DrawMainPage(pageNum);
		}
		//next page wont register touch until you let go
		while (TS_State.TouchDetected) {
			BSP_TS_GetState(&TS_State);
			if (!TS_State.TouchDetected) {
			}
			HAL_Delay(20); //WOOOO CANT TOUCH UNLESS YOU RELASE. the clk speed is super fast, need delay. ok if you slide the pen it breaks, but that dont count
		}
		break;
	case PAGE_FINISH:
		if (TS_State.X >= backButton.x
				&& TS_State.X <= (backButton.x + backButton.w)
				&& TS_State.Y >= backButton.y
				&& TS_State.Y <= (backButton.y + backButton.h)) {
			currentPage = PAGE_MAIN;
			DrawMainPage(pageNum);
		}
		break;
	}
}

//I name sectors from 1-14. the actual sectors are from 5-23. this functions maps them
uint32_t get_sector_address(uint32_t sector) {
	switch (sector) {
	case 1:
		return SECTOR_5;
	case 2:
		return SECTOR_6;
	case 3:
		return SECTOR_7;
	case 4:
		return SECTOR_8;
	case 5:
		return SECTOR_9;
	case 6:
		return SECTOR_10;
	case 7:
		return SECTOR_11;
	case 8:
		return SECTOR_17;
	case 9:
		return SECTOR_18;
	case 10:
		return SECTOR_19;
	case 11:
		return SECTOR_20;
	case 12:
		return SECTOR_21;
	case 13:
		return SECTOR_22;
	case 14:
		return SECTOR_23;
	default:
		break;
	}
}

uint32_t sector_mapping(uint32_t sector) {
	switch (sector) {
	case 1:
		return 5;
	case 2:
		return 6;
	case 3:
		return 7;
	case 4:
		return 8;
	case 5:
		return 9;
	case 6:
		return 10;
	case 7:
		return 11;
	case 8:
		return 17;
	case 9:
		return 18;
	case 10:
		return 19;
	case 11:
		return 20;
	case 12:
		return 21;
	case 13:
		return 22;
	case 14:
		return 23;
	}
}

/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM6 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	/* USER CODE BEGIN Callback 0 */

	/* USER CODE END Callback 0 */
	if (htim->Instance == TIM6) {
		HAL_IncTick();
	}
	/* USER CODE BEGIN Callback 1 */

	//10 hz interrupt
	if (htim == &htim10) {
		if (!USB_BUSY) {
			checkTouchFlag = 1;
		}
		count++;
	}

	//use this for a slower frequency application
	if (count == 5) {
		//checkTouchFlag = 1;
		count = 0;
	}
	/* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
