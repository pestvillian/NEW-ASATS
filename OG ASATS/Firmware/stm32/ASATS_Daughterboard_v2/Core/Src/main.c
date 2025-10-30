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
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "usbh_hid.h"
#include "stdio.h"
#include "ili9341.h"
//#include "XPT2046_touch.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/**** TOUCHSCREEN DEFINITIONS *****/
#define TS_LEFT 0
#define TS_RIGHT 320
#define TS_TOP 0
#define TS_BOT 240
#define BACK_BUTTON_OFFSET 60 //experimentally determined for UI smoothness
#define NEXT_BUTTON_OFFSET 60 //experimentally determined for UI smoothness
#define QUEUE_BUTTON_OFFSET 40 //experimentally determined for UI smoothness
#define PROTOCOL_BUTTON_OFFSET 20
#define DEBOUNCE_DELAY_MS 30

/***** USB DEFINITIONS *****/
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

/***** FLASH MEMORY DEFINITIONS *****/
#define NUMBER_OF_SECTORS 14
#define MAX_PROTOCOLS_IN_SECTOR 3
#define PROTOCOL_SIZE 6000
#define MAX_LINES 200
#define MAX_LINE_LENGTH 32 //for linear movement it wont be more than 32 chars
#define MAX_TITLE_SIZE 14  //you can 10 chars, but also we need newline and null

/******** LCD DISPLAY DEFINITIONS ********/
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
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

typedef enum {
	PAGE_MAIN,
	PAGE_SELECT,
	PAGE_QUEUE,
	PAGE_CONFIRMATION,
	PAGE_FINISH,
	PAGE_PROGRESS,
	PAGE_PROTOCOL_INFO,
	PAGE_STOP
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

Button buttons[NUM_BUTTONS] = { { 80, 50, BUTTON_WIDTH, BUTTON_HEIGHT,
		"Protocol 1", EMPTY }, { 80, 110, BUTTON_WIDTH, BUTTON_HEIGHT,
		"Protocol 2", EMPTY }, { 80, 170, BUTTON_WIDTH, BUTTON_HEIGHT,
		"Protocol 3", EMPTY } };

Button queueButton = { 240, 0, 80, 30, "Queue" }; //x, y, w, h, label
Button runButton = { 260, 200, 60, 40, "Run" };
Button confirmButton = { 100, 120, 120, 40, "Confirm" };
Button queueSelectButton = { 110, 50, 100, 40, "Queue" }; //x, y, w, h, label
Button selectButton = { 110, 110, 100, 40, "Run" };
Button deleteButton = { 110, 170, 100, 40, "Delete" };
Button backButton = { 0, 205, 60, 40, "Back" }; //x, y, w, h, label
Button nextButton = { 260, 205, 60, 40, "Next" };
Button yesButton = { 100, 80, 120, 40, "Yes" };
Button noButton = { 100, 140, 120, 40, "No" };
Button protocolInfoButton = { 240, 0, 80, 30, "Info" };

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM2_Init(void);
static void MX_SPI1_Init(void);
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
void deleteProtocol(uint32_t sector, uint32_t offset);
void queueProtocol(uint32_t sector, uint32_t offset);
void transmitQueuedProtocols(uint8_t queueSize);
uint32_t get_sector_address(uint32_t sector);
void write_number_to_flash(uint32_t flash_address, uint32_t number);
void DrawQueuePage(uint8_t queueSize);
void Touch_Init(void);
uint8_t HandleTouch(void);
void DrawMainPage(uint8_t page_num);
void DrawInfoPage(char protocolTitle[MAX_LINE_LENGTH]);
void DrawProtocolInfoPage(uint32_t sector, uint32_t offset);
void DrawConfirmationPage(uint32_t sector, uint32_t offset);
void DrawProgressPage(char protocolTitle[20], uint8_t rx_byte, char *rx_data);
uint32_t ConvertCharsToInt(char x, char y, char z);
void DrawStopPage(void);
void SendStopMotorsMessage(void);
uint8_t handleTouch();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len) {
	for (int i = 0; i < len; i++) {
		ITM_SendChar((*ptr++));  // Send the character via ITM
	}
	return len;
}

volatile protocolTimer = 0; //volatile because it is changed in interrupt?

//UART variables
uint8_t rx_byte;
char rx_bind[20];
char rx_pause[3];
char rx_move[7];
char rx_data[20];
char currentProtocolTitle[20];

volatile uint8_t currentRepeatNum = 1;
volatile uint32_t last_interrupt_time = 0;
static bool prevTouchedState = 1; //pretty sure it starts high
uint8_t count = 0;
static uint8_t touchFlag = 0;
static uint8_t page_num = 1;

int i = 0;
int j = 0;
static char queueBuffer[MAX_QUEUE_SIZE][MAX_LINES][MAX_LINE_LENGTH];
static uint8_t queueSize = 0;
//static uint8_t pageNum = 1;
static uint8_t USB_BUSY = 0;
char qr_code_data[MAX_LINES][MAX_LINE_LENGTH] = { { '\0' } }; //static initializes strings with all null characters
HID_KEYBD_Info_TypeDef *Keyboard_Info;

/***DORJEE YOU HAVE TO CHANGE USBH_HID.H WHEN YOU GENERATE CODE****/
void USBH_HID_EventCallback(USBH_HandleTypeDef *phost) { //2.6s for 54 lines
	Keyboard_Info = USBH_HID_GetKeybdInfo(phost);
	char key = USBH_HID_GetASCIICode(Keyboard_Info);
	printf("%c\n", key);
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
				page_num = freeSectorNumber;
				DrawMainPage(page_num);
				currentPage = PAGE_MAIN;
				for (int a = 0; a < MAX_LINES; a++) {
					for (int b = 0; b < MAX_LINE_LENGTH; b++) {
						qr_code_data[a][b] = '\0';
					}
				}
			} else {
				DrawQueuePage(queueSize);
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
int main(void)
{

  /* USER CODE BEGIN 1 */

	//printf("Hello\n");
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

//	HAL_GPIO_WritePin(OTG_FS_PSO_GPIO_Port, OTG_FS_PSO_Pin, GPIO_PIN_SET);
//	HAL_Delay(50);

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USB_HOST_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_TIM2_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

	printf("start program\n");

//  	erase_sector(1);
//  	erase_sector(2);
//  	erase_sector(3);
//  	erase_sector(4);
//  	erase_sector(5);
//  	erase_sector(6);
//  	erase_sector(7);
//  	erase_sector(8);
//  	erase_sector(9);
//  	erase_sector(10);
	HAL_GPIO_WritePin(LCD_NRST_GPIO_Port, LCD_NRST_Pin, GPIO_PIN_SET);
	HAL_Delay(50);
	HAL_GPIO_WritePin(LCD_NRST_GPIO_Port, LCD_NRST_Pin, GPIO_PIN_SET);
	HAL_Delay(50);

	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
	htim3.Instance->CCR2 = 50;

	lcdInit();
	//lcdTest();
	lcdSetOrientation(LCD_ORIENTATION_LANDSCAPE);
	DrawMainPage(1);

	HAL_GPIO_WritePin(OTG_FS_PSO_GPIO_Port, OTG_FS_PSO_Pin, GPIO_PIN_RESET); //turn fan on

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */

		if (touchFlag) {
			printf("touced\n");
			handleTouch();
			touchFlag = 0;
			HAL_Delay(100);
		}
		if (currentPage == PAGE_PROGRESS) {
			//when protocol finishes naturally, it will send a done signal

			if (HAL_UART_Receive(&huart2, &rx_byte, 1, 0) == HAL_OK) {
				//protocol starts, get the protocl title
				if (rx_byte == 'T') {
					memset(currentProtocolTitle, 0, MAX_LINE_LENGTH);
					HAL_UART_Receive(&huart2, (uint8_t*) currentProtocolTitle,
							sizeof(currentProtocolTitle), 100);
					DrawProgressPage("FILL", rx_byte, rx_data);
				}
				//repeat update
				if (rx_byte == 'R') {
					char repeatUpdate[2];
					HAL_UART_Receive(&huart2, (uint8_t*) repeatUpdate,
							sizeof(repeatUpdate), 100);
					currentRepeatNum = atoi(repeatUpdate);
					printf("%d\n", currentRepeatNum);
					//repeat
					char repeat[20] = "";
					uint32_t repeatInt = ConvertCharsToInt('0', rx_data[12],
							rx_data[13]);
					//sprintf(repeat, "Repeat: %c%c", rx_data[11], rx_data[12]);
					sprintf(repeat, "Repeat: %d/%d", currentRepeatNum,
							repeatInt);
					lcdSetCursor(10, 150);
					lcdPrintf(repeat);
				}
				//protocol starts, update progress screen
				if (rx_byte == 'B') {
					currentRepeatNum = 1;
					HAL_UART_Receive(&huart2, (uint8_t*) rx_data,
							sizeof(rx_data), 100);
					DrawProgressPage("FILL", rx_byte, rx_data);
				}
				if (rx_byte == 'M') {
					HAL_UART_Receive(&huart2, (uint8_t*) rx_data,
							sizeof(rx_data), 100);
					DrawProgressPage("FILL", rx_byte, rx_data);
				}
				if (rx_byte == 'P') {
					HAL_UART_Receive(&huart2, (uint8_t*) rx_data,
							sizeof(rx_data), 100);
					DrawProgressPage("FILL", rx_byte, rx_data);
				}

				//protocol finishes
				if (rx_byte == 'D') {
					//go to success page
					currentPage = PAGE_FINISH;
					DrawPageFinish();
				}

				//reset rx_byte i guess?
				//rx_byte = 0;

			}
		}
		//HAL_Delay(100);
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_1LINE;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 8400-1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 10000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 168-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 100-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM2;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, FAN_EN_Pin|DEBUG_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_NRST_GPIO_Port, LCD_NRST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(OTG_FS_PSO_GPIO_Port, OTG_FS_PSO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : FAN_EN_Pin DEBUG_LED_Pin */
  GPIO_InitStruct.Pin = FAN_EN_Pin|DEBUG_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB14 PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_NRST_Pin */
  GPIO_InitStruct.Pin = LCD_NRST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_NRST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : T_IRQ_Pin OTG_FS_OC_Pin */
  GPIO_InitStruct.Pin = T_IRQ_Pin|OTG_FS_OC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_PSO_Pin */
  GPIO_InitStruct.Pin = OTG_FS_PSO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(OTG_FS_PSO_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/**
 * @brief: Draw main page and handle button label and status updates
 * @param: page_num: determines which page should be drawn from 1-10
 * @retval: none
 */
void DrawMainPage(uint8_t page_num) {
	//Draw "Protocol Title" box
	lcdFillRGB(COLOR_WHITE);
	char empty[20] = "Empty\n"; //strings from flash memory come with newline at end
	char pageTitle[20] = "";
	sprintf(pageTitle, "Page %d", page_num);
	lcdSetCursor(10, 10);
	lcdSetTextColor(COLOR_BLACK, COLOR_WHITE);
	lcdSetTextFont(&Font20);
	lcdPrintf(pageTitle);
	lcdSetTextFont(&Font16);

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
	for (int i = 0; i < NUM_BUTTONS; i++) {
		lcdDrawRect(buttons[i].x, buttons[i].y, buttons[i].w, buttons[i].h,
		COLOR_BLACK);
		buttons[i].label[strlen(buttons[i].label) - 1] = ' '; //dont display null terminator
		lcdSetCursor(buttons[i].x + 10, buttons[i].y + 10);
		if (buttons[i].status == EMPTY) {
			lcdPrintf("EMPTY");
		} else {
			lcdPrintf(buttons[i].label);
		}
	}

	// Draw "Next" button
	if (page_num != 10) {
		//BSP_LCD_SetTextColor(LCD_COLOR_RED);
		lcdDrawRect(nextButton.x, nextButton.y, nextButton.w, nextButton.h,
		COLOR_BLACK);
		lcdSetCursor(nextButton.x + 5, nextButton.y + 5);
		lcdPrintf(nextButton.label);
	}

	// Draw "Back" button
	if (page_num != 1) {
		lcdDrawRect(backButton.x, backButton.y, backButton.w, backButton.h,
		COLOR_BLACK);
		lcdSetCursor(backButton.x + 5, backButton.y + 5);
		lcdPrintf(backButton.label);
	}
//
	//draw "Queue" button on page 1
	lcdDrawRect(queueButton.x, queueButton.y, queueButton.w, queueButton.h,
	COLOR_BLACK);
	lcdSetCursor(queueButton.x + 5, queueButton.y + 5);
	lcdPrintf(queueButton.label);
}

/**
 * @brief: Draw select page with run, delete and queue buttons
 * @param: protocolTitle: title of the protocol to be displayed
 * @retval: none
 */
void DrawInfoPage(char protocolTitle[MAX_LINE_LENGTH]) {
	lcdFillRGB(COLOR_WHITE);
	//Display protocol name at top of screen
	lcdSetCursor(90, 10);
	lcdSetTextFont(&Font20);
	lcdPrintf(protocolTitle);
	lcdSetTextFont(&Font16);

	//Draw "QueueSelect" button on select page
	lcdDrawRect(queueSelectButton.x, queueSelectButton.y, queueSelectButton.w,
			queueSelectButton.h, COLOR_BLACK);
	lcdSetCursor(queueSelectButton.x + 10, queueSelectButton.y + 10);
	lcdPrintf(queueSelectButton.label);

	//Draw "Select" button
	lcdDrawRect(selectButton.x, selectButton.y, selectButton.w, selectButton.h,
	COLOR_BLACK);
	lcdSetCursor(selectButton.x + 10, selectButton.y + 10);
	lcdPrintf(selectButton.label);

	//Draw "Delete" button
	lcdDrawRect(deleteButton.x, deleteButton.y, deleteButton.w, deleteButton.h,
	COLOR_BLACK);
	lcdSetCursor(deleteButton.x + 10, deleteButton.y + 10);
	lcdPrintf(deleteButton.label);

	// Draw "Info" button
	lcdDrawRect(protocolInfoButton.x, protocolInfoButton.y,
			protocolInfoButton.w, protocolInfoButton.h,
			COLOR_BLACK);
	lcdSetCursor(protocolInfoButton.x + 10, protocolInfoButton.y + 10);
	lcdPrintf(protocolInfoButton.label);

	// Draw "Back" button
	lcdDrawRect(backButton.x, backButton.y, backButton.w, backButton.h,
	COLOR_BLACK);
	lcdSetCursor(backButton.x + 10, backButton.y + 10);
	lcdPrintf(backButton.label);
}

/**
 * @brief: Confirmation page to delete a protocol
 * @param: sector: which sector in flash memory the protocol exists
 * @param: offset: which section of the sector the protocol exists
 * @retval: none
 */
void DrawConfirmationPage(uint32_t sector, uint32_t offset) {
	lcdFillRGB(COLOR_WHITE);

	//display confirmation text
	lcdSetCursor(10, 10);
	lcdSetTextFont(&Font20);
	lcdPrintf("Delete: ");
	lcdPrintf(buttons[offset].label);
	lcdPrintf("?");
	lcdSetTextFont(&Font16);

	//Draw "Confirm" button
	lcdDrawRect(confirmButton.x, confirmButton.y, confirmButton.w,
			confirmButton.h, COLOR_BLACK);
	lcdSetCursor(confirmButton.x + 10, confirmButton.y + 10);
	lcdPrintf(confirmButton.label);

	// Draw "Back" button
	lcdDrawRect(backButton.x, backButton.y, backButton.w, backButton.h,
	COLOR_BLACK);
	lcdSetCursor(backButton.x + 10, backButton.y + 10);
	lcdPrintf(backButton.label);
}

/**
 * @brief: Queue page has up to 10 protocols lined up
 * @param: queueSize: number of protocols in the queue
 * @retval: none
 */
void DrawQueuePage(uint8_t queueSize) {
	lcdFillRGB(COLOR_WHITE);
	char pageTitle[20] = "";
	sprintf(pageTitle, "Queue Size: %d", queueSize);
	//Display Queue at top of the screen
	lcdSetCursor(60, 10);
	lcdSetTextFont(&Font20);
	lcdPrintf(pageTitle);
	lcdSetTextFont(&Font16);

	//display protocols in queue here
	char format[20] = "";
	int startSpotX, startSpotY;
	for (uint8_t i = 0; i < queueSize; i++) {
		if (i < 5) {
			startSpotX = 10;
			startSpotY = 50;
		} else {
			startSpotX = 160;
			startSpotY = 50 - 100;	//convoluted ik
		}
		sprintf(format, "%d.) ", i + 1);
		lcdSetCursor(startSpotX, startSpotY + i * 20);
		lcdSetTextFont(&Font12);
		lcdPrintf(format);
		lcdSetTextFont(&Font16);
		lcdSetCursor(startSpotX + 24, startSpotY + i * 20);
		lcdPrintf(queueBuffer[i][0]);

	}

	//Draw "Run" button
	if (queueSize > 0) {
		lcdDrawRect(runButton.x, runButton.y, runButton.w, runButton.h,
		COLOR_BLACK);
		lcdSetCursor(runButton.x + 10, runButton.y + 10);
		lcdPrintf(runButton.label);
	}

	// Draw "Back" button
	lcdDrawRect(backButton.x, backButton.y, backButton.w, backButton.h,
	COLOR_BLACK);
	lcdSetCursor(backButton.x + 10, backButton.y + 10);
	lcdPrintf(backButton.label);
}

/**
 * @brief: Draw progress page during motor operation
 * @param: protocolTitle: display protocol being run
 * @param: rx_byte: current motor movement type
 * @param: rx_data: current motor movement info
 * @retval: none
 */
void DrawProgressPage(char protocolTitle[20], uint8_t rx_byte, char *rx_data) {
	lcdFillRGB(COLOR_WHITE);

	//protocol title
	lcdSetCursor(100, 10);
	lcdPrintf(currentProtocolTitle);

	//bind
	if (rx_byte == 'B') {
		//protocol type
		char protocolType[20] = "";
		sprintf(protocolType, "Bind");
		lcdSetCursor(10, 30);
		lcdSetTextFont(&Font16);
		lcdPrintf(protocolType);
		//lcdSetTextFont(&Font16);
		//speed
		char speed[20] = "";
		printf("start test\n");
		uint32_t speedInt = ConvertCharsToInt('0', '0', rx_data[1]);
		sprintf(speed, "Speed: %d", speedInt);
		//printf("%d\n", speedInt);
		lcdSetCursor(10, 50);
		lcdPrintf(speed);
		//duration
		char duration[20] = "";
		uint32_t durationInt = ConvertCharsToInt('0', rx_data[2], rx_data[3]);
		//sprintf(duration, "Duration: %c%c", rx_data[2], rx_data[3]);
		sprintf(duration, "Duration: %d", durationInt);
		lcdSetCursor(10, 70);
		lcdPrintf(duration);
		//volume
		char volume[20] = "";
		uint32_t volumeInt = ConvertCharsToInt(rx_data[4], rx_data[5],
				rx_data[6]);
		//sprintf(volume, "Volume: %c%c%c", rx_data[4], rx_data[5], rx_data[6]);
		sprintf(volume, "Volume: %d", volumeInt);
		lcdSetCursor(10, 90);
		lcdPrintf(volume);
		//depth
		char depth[20] = "";
		uint32_t depthInt = ConvertCharsToInt(rx_data[7], rx_data[8],
				rx_data[9]);
		//sprintf(depth, "Depth: %c%c%c", rx_data[7], rx_data[8], rx_data[9]);
		sprintf(depth, "Depth: %d", depthInt);
		lcdSetCursor(10, 110);
		lcdPrintf(depth);
		//pauseDuration
		char pauseDuration[20] = "";
		uint32_t pauseInt = ConvertCharsToInt('0', rx_data[10], rx_data[11]);
		sprintf(pauseDuration, "PauseDuration: %d", pauseInt);
		lcdSetCursor(10, 130);
		lcdPrintf(pauseDuration);
		//repeat
		char repeat[20] = "";
		uint32_t repeatInt = ConvertCharsToInt('0', rx_data[12], rx_data[13]);
		//sprintf(repeat, "Repeat: %c%c", rx_data[11], rx_data[12]);
		sprintf(repeat, "Repeat: %d/%d", currentRepeatNum, repeatInt);
		lcdSetCursor(10, 150);
		lcdPrintf(repeat);
	}

	//pause
	if (rx_byte == 'P') {
		char protocolType[20] = "";
		sprintf(protocolType, "Pause");
		lcdSetCursor(60, 30);
		lcdSetTextFont(&Font20);
		lcdPrintf(protocolType);
		lcdSetTextFont(&Font16);
		//duration
		char duration[20] = "";
		sprintf(duration, "Duration: %c", rx_data[1]);
		lcdSetCursor(60, 50);
		lcdPrintf(duration);
	}

	//move
	if (rx_byte == 'M') {
		char protocolType[20] = "";
		sprintf(protocolType, "Magnetize");
		lcdSetCursor(60, 30);
		lcdSetTextFont(&Font20);
		lcdPrintf(protocolType);
		lcdSetTextFont(&Font16);
		//init surface time
		char initTime[20] = "";
		uint32_t initTimeInt = ConvertCharsToInt(rx_data[1], rx_data[2],
				rx_data[3]);
		sprintf(initTime, "Init Time: %d", initTimeInt);
		lcdSetCursor(60, 50);
		lcdPrintf(initTime);
		//speed
		char speed[20] = "";
		sprintf(speed, "Speed: %d", ConvertCharsToInt('0', '0', rx_data[4]));
		lcdSetCursor(60, 70);
		lcdPrintf(speed);
		//Stop at Sequences
		char stopNumber[20] = "";
		sprintf(stopNumber, "Stop Number: %d",
				ConvertCharsToInt('0', '0', rx_data[5]));
		lcdSetCursor(60, 90);
		lcdPrintf(stopNumber);
		//Sequence Pause Time
		char sequencePause[20] = "";
		sprintf(sequencePause, "Sequence Pause: %d",
				ConvertCharsToInt('0', '0', rx_data[6]));
		lcdSetCursor(60, 110);
		lcdPrintf(sequencePause);
	}

	//time remaining in protocol
//	char protocolTimeRemaining[25] = "";
//	sprintf(protocolTimeRemaining, "Remaining Time: %d", protocolTimer);
//	lcdSetCursor(60, 130);
//	lcdPrintf(protocolTimeRemaining);
	lcdSetCursor(10, 170);
	DrawCountdownTime();

	// Draw "Stop" button
	lcdDrawRect(backButton.x, backButton.y, backButton.w, backButton.h,
	COLOR_BLACK);
	lcdSetCursor(backButton.x + 10, backButton.y + 10);
	lcdPrintf("Stop");
}

//dont want to update the whole progress page, just the countdown timer
void DrawCountdownTime(void) {
	//time remaining in protocol
	uint8_t minutes = protocolTimer / 60;
	uint8_t seconds = protocolTimer % 60;
	char protocolTimeRemaining[25] = "";
	if (seconds < 10) {
		memset(protocolTimeRemaining, 0, 25); // Sets all elements of buffer to 0
		sprintf(protocolTimeRemaining, "Remaining Time: %d:0%d", minutes,
				seconds);
	} else {
		memset(protocolTimeRemaining, 0, 25); // Sets all elements of buffer to 0
		sprintf(protocolTimeRemaining, "Remaining Time: %d:%d", minutes,
				seconds);
	}
	protocolTimeRemaining[strlen(protocolTimeRemaining)] = NULL_CHAR;
	lcdSetCursor(10, 170);
	lcdPrintf(protocolTimeRemaining);
}

/**
 * @brief: Take in multiple chars and convert them into an integer
 * @example: '3' '4' '8' gets converted to an integer value of 348
 * @param: x: 100's place of the digit
 * @param: y: 10's place of the digit
 * @param: z: 1's place of the digit
 * @retval: converted integer
 */
uint32_t ConvertCharsToInt(char x, char y, char z) {
	uint32_t returnVal = ((x - '0') * 100) + ((y - '0') * 10) + (z - '0');
	return returnVal;
}

/**
 * @brief: Draw progress page during motor operation
 * @param: protocolTitle: display protocol being run
 * @param: rx_byte: current motor movement type
 * @param: rx_data: current motor movement info
 * @retval: none
 */
void DrawProtocolInfoPage(uint32_t page_num, uint32_t offset) {
	lcdFillRGB(COLOR_WHITE);

	//protocol title
	lcdSetCursor(100, 10);
	char protocolTitle[20] = "";
	uint32_t current_flash_address = get_sector_address(
			page_num) + offset * PROTOCOL_SIZE;
	read_from_flash(protocolTitle, current_flash_address);
	lcdPrintf(protocolTitle);

	//get the next protocol line
	current_flash_address += MAX_LINE_LENGTH;
	//read_from_flash()

//	//bind
//	if (rx_byte == 'B') {
//		//protocol type
//		char protocolType[20] = "";
//		sprintf(protocolType, "Bind");
//		lcdSetCursor(10, 30);
//		lcdSetTextFont(&Font16);
//		lcdPrintf(protocolType);
//		//lcdSetTextFont(&Font16);
//		//speed
//		char speed[20] = "";
//		printf("start test\n");
//		uint32_t speedInt = ConvertCharsToInt('0', '0', rx_data[1]);
//		sprintf(speed, "Speed: %d", speedInt);
//		//printf("%d\n", speedInt);
//		lcdSetCursor(10, 50);
//		lcdPrintf(speed);
//		//duration
//		char duration[20] = "";
//		uint32_t durationInt = ConvertCharsToInt('0', rx_data[2], rx_data[3]);
//		//sprintf(duration, "Duration: %c%c", rx_data[2], rx_data[3]);
//		sprintf(duration, "Duration: %d", durationInt);
//		lcdSetCursor(10, 70);
//		lcdPrintf(duration);
//		//volume
//		char volume[20] = "";
//		uint32_t volumeInt = ConvertCharsToInt(rx_data[4], rx_data[5], rx_data[6]);
//		//sprintf(volume, "Volume: %c%c%c", rx_data[4], rx_data[5], rx_data[6]);
//		sprintf(volume, "Volume: %d", volumeInt);
//		lcdSetCursor(10, 90);
//		lcdPrintf(volume);
//		//depth
//		char depth[20] = "";
//		uint32_t depthInt = ConvertCharsToInt(rx_data[7], rx_data[8], rx_data[9]);
//		//sprintf(depth, "Depth: %c%c%c", rx_data[7], rx_data[8], rx_data[9]);
//		sprintf(depth, "Depth: %d", depthInt);
//		lcdSetCursor(10, 110);
//		lcdPrintf(depth);
//		//pauseDuration
//		char pauseDuration[20] = "";
//		uint32_t pauseInt = ConvertCharsToInt('0', rx_data[10], rx_data[11]);
//		sprintf(pauseDuration, "PauseDuration: %d", pauseInt);
//		lcdSetCursor(10, 130);
//		lcdPrintf(pauseDuration);
//		//repeat
//		char repeat[20] = "";
//		uint32_t repeatInt = ConvertCharsToInt('0', rx_data[12], rx_data[13]);
//		//sprintf(repeat, "Repeat: %c%c", rx_data[11], rx_data[12]);
//		sprintf(repeat, "Repeat: %d/%d", currentRepeatNum, repeatInt);
//		lcdSetCursor(10, 150);
//		lcdPrintf(repeat);
//	}
//
//	//pause
//	if (rx_byte == 'P') {
//		char protocolType[20] = "";
//		sprintf(protocolType, "Pause");
//		lcdSetCursor(60, 30);
//		lcdSetTextFont(&Font20);
//		lcdPrintf(protocolType);
//		lcdSetTextFont(&Font16);
//		//duration
//		char duration[20] = "";
//		sprintf(duration, "Duration: %c", rx_data[1]);
//		lcdSetCursor(60, 50);
//		lcdPrintf(duration);
//	}
//
//	//move
//	if (rx_byte == 'M') {
//		char protocolType[20] = "";
//		sprintf(protocolType, "Magnetize");
//		lcdSetCursor(60, 30);
//		lcdSetTextFont(&Font20);
//		lcdPrintf(protocolType);
//		lcdSetTextFont(&Font16);
//		//init surface time
//		char initTime[20] = "";
//		uint32_t initTimeInt = ConvertCharsToInt(rx_data[1], rx_data[2], rx_data[3]);
//		sprintf(initTime, "Init Time: %d", initTimeInt);
//		lcdSetCursor(60, 50);
//		lcdPrintf(initTime);
//		//speed
//		char speed[20] = "";
//		sprintf(speed, "Speed: %d", ConvertCharsToInt('0', '0', rx_data[4]));
//		lcdSetCursor(60, 70);
//		lcdPrintf(speed);
//		//Stop at Sequences
//		char stopNumber[20] = "";
//		sprintf(stopNumber, "Stop Number: %d", ConvertCharsToInt('0', '0', rx_data[5]));
//		lcdSetCursor(60, 90);
//		lcdPrintf(stopNumber);
//		//Sequence Pause Time
//		char sequencePause[20] = "";
//		sprintf(sequencePause, "Sequence Pause: %d", ConvertCharsToInt('0', '0', rx_data[6]));
//		lcdSetCursor(60, 110);
//		lcdPrintf(sequencePause);
//	}

	// Draw "Back" button
	lcdDrawRect(backButton.x, backButton.y, backButton.w, backButton.h,
	COLOR_BLACK);
	lcdSetCursor(backButton.x + 10, backButton.y + 10);
	lcdPrintf(backButton.label);
}

void DrawStopPage(void) {
	lcdFillRGB(COLOR_WHITE);

	//display confirmation text
	lcdSetCursor(100, 10);
	lcdSetTextFont(&Font20);
	lcdPrintf("Stop?");
	lcdSetTextFont(&Font16);

	//Draw "Yes" button
	lcdDrawRect(yesButton.x, yesButton.y, yesButton.w, yesButton.h,
	COLOR_BLACK);
	lcdSetCursor(yesButton.x + 10, yesButton.y + 10);
	lcdPrintf(yesButton.label);

	//Draw "No" button
	lcdDrawRect(noButton.x, noButton.y, noButton.w, noButton.h, COLOR_BLACK);
	lcdSetCursor(noButton.x + 10, noButton.y + 10);
	lcdPrintf(noButton.label);
}

void DrawPageFinish(void) {
	lcdFillRGB(COLOR_WHITE);

	//Draw "Success!" box
	lcdSetCursor(selectButton.x + 10, selectButton.y + 10);
	lcdPrintf("Success!");

	// Draw "Back" button
	lcdDrawRect(backButton.x, backButton.y, backButton.w, backButton.h,
	COLOR_BLACK);
	lcdSetCursor(backButton.x + 10, backButton.y + 10);
	lcdPrintf("Done");
}

uint8_t handleTouch() {
	static uint8_t protocol_num;
	static uint8_t protocol_offset = 0;
	uint16_t x = 0, y = 0;
	if (!XPT2046_TouchGetCoordinates(&x, &y)) {
		//printf("failed\n");
		return 0;
	}
	//map the x coordinate to be left is 0. also slight offset
	x = (TS_RIGHT - x) - 0;
//	printf("touched\n");
//	printf("%d, %d\n", x, y);

	switch (currentPage) {
	case PAGE_MAIN:
		//next button
		if ((x >= nextButton.x) && (x <= nextButton.x + nextButton.w)
				&& (y >= nextButton.y)
				&& (y <= nextButton.y + nextButton.h + NEXT_BUTTON_OFFSET)
				&& (page_num != 10)) {
			//printf("touched\n");
			page_num++;
			DrawMainPage(page_num);
		}
		//back button
		if ((x >= backButton.x) && (x <= backButton.x + backButton.w)
				&& (y >= backButton.y)
				&& (y <= backButton.y + backButton.h + BACK_BUTTON_OFFSET)
				&& (page_num != 1)) {
			//printf("touched\n");
			page_num--;
			DrawMainPage(page_num);
		}
		//queue button
		if (x >= queueButton.x && x <= (queueButton.x + queueButton.w)
				&& y >= queueButton.y
				&& y <= (queueButton.y + queueButton.h + QUEUE_BUTTON_OFFSET)) {
			DrawQueuePage(queueSize);
			currentPage = PAGE_QUEUE;
		}
		//three protocol buttons
		for (int i = 0; i < NUM_BUTTONS; i++) {
			if ((x >= buttons[i].x) && (x <= buttons[i].x + buttons[i].w)
					&& (y >= buttons[i].y - PROTOCOL_BUTTON_OFFSET)
					&& (y
							<= buttons[i].y + buttons[i].h
									+ PROTOCOL_BUTTON_OFFSET)) {
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
				HAL_Delay(100); //delay between switch to select page
			}
		}
		break;

	case PAGE_SELECT:
		//back button
		if (x >= backButton.x && x <= (backButton.x + backButton.w)
				&& y >= backButton.y && y <= (backButton.y + backButton.h)) {
			currentPage = PAGE_MAIN;
			DrawMainPage(page_num);
		}
		//queueSelect button
		if (x >= queueSelectButton.x
				&& x <= (queueSelectButton.x + queueSelectButton.w)
				&& y >= queueSelectButton.y
				&& y <= (queueSelectButton.y + queueSelectButton.h)) {
			//store the protocol in queueBuffer
			if (queueSize < MAX_QUEUE_SIZE) {

				queueProtocol(page_num, protocol_offset);
				queueSize++;
				currentPage = PAGE_QUEUE;
				DrawQueuePage(queueSize);
			} else {
				//handle queue buffer being full
			}
		}
		//select button
		if (x >= selectButton.x && x <= (selectButton.x + selectButton.w)
				&& y >= selectButton.y
				&& y <= (selectButton.y + selectButton.h)) {
			//transmit protocol and move to finish page
			transmitProtocol(page_num, protocol_offset);
			currentPage = PAGE_PROGRESS;
			protocolTimer = 5999; //change this to the actual value dorjee
			uint32_t flash_address = get_sector_address(
					page_num) + protocol_offset * PROTOCOL_SIZE;
			read_from_flash(currentProtocolTitle, flash_address);
			DrawProgressPage("FILL", rx_byte, rx_data);

			//start timer 2 interrupt for protocol timer count down
			HAL_TIM_Base_Start_IT(&htim2); //triggers every second
			HAL_GPIO_WritePin(FAN_EN_GPIO_Port, FAN_EN_Pin, GPIO_PIN_SET); //turn fan on
		}
		//info button
		if (x >= protocolInfoButton.x
				&& x <= (protocolInfoButton.x + protocolInfoButton.w)
				&& y >= protocolInfoButton.y
				&& y <= (protocolInfoButton.y + protocolInfoButton.h)) {
			//move to delete confirmation page
			currentPage = PAGE_PROTOCOL_INFO;
			DrawProtocolInfoPage(page_num, protocol_offset);
		}
		//delete button
		if (x >= deleteButton.x && x <= (deleteButton.x + deleteButton.w)
				&& y >= deleteButton.y
				&& y <= (deleteButton.y + deleteButton.h)) {
			//move to delete confirmation page
			currentPage = PAGE_CONFIRMATION;
			DrawConfirmationPage(page_num, protocol_offset);
		}
		break;

	case PAGE_QUEUE:
		//back button
		if (x >= backButton.x && x <= (backButton.x + backButton.w)
				&& y >= backButton.y && y <= (backButton.y + backButton.h)) {
			currentPage = PAGE_MAIN;
			DrawMainPage(page_num);
		}
		//run button
		if (x >= runButton.x && x <= (runButton.x + runButton.w)
				&& y >= runButton.y && y <= (runButton.y + runButton.h)
				&& (queueSize > 0)) {
			//handle queue functionality here dorjee
			transmitQueuedProtocols(queueSize);
			queueSize = 0;
			currentPage = PAGE_PROGRESS;
			DrawProgressPage("FILL", rx_byte, rx_data);
		}
		break;

	case PAGE_CONFIRMATION:
		//back button
		if (x >= backButton.x && x <= (backButton.x + backButton.w)
				&& y >= backButton.y && y <= (backButton.y + backButton.h)) {
			currentPage = PAGE_SELECT;
			DrawInfoPage(buttons[protocol_offset].label);
		}
		//confirm button
		if (x >= confirmButton.x && x <= (confirmButton.x + confirmButton.w)
				&& y >= confirmButton.y
				&& y <= (confirmButton.y + confirmButton.h)) {
			//delete protocol and go back to main page
			deleteProtocol(page_num, protocol_offset);
			currentPage = PAGE_MAIN;
			DrawMainPage(page_num);
		}
		break;

	case PAGE_PROGRESS:
		//stop button
		if ((x >= backButton.x) && (x <= backButton.x + backButton.w)
				&& (y >= backButton.y)
				&& (y <= backButton.y + backButton.h + BACK_BUTTON_OFFSET)) {
			//printf("touched\n");
			currentPage = PAGE_STOP;
			DrawStopPage();
		}
		//when protocol finishes naturally, it will send a done signal
		uint8_t done_signal;
		if (HAL_UART_Receive(&huart2, &done_signal, 1, 0) == HAL_OK) {
			if (done_signal == 'D') {
				//go to success page
				currentPage = PAGE_FINISH;
				DrawPageFinish();

				//stop the interrupt and turn fan off
				HAL_TIM_Base_Stop_IT(&htim2);
				HAL_GPIO_WritePin(FAN_EN_GPIO_Port, FAN_EN_Pin, GPIO_PIN_RESET);
			}
		}
		break;

	case PAGE_STOP:
		//no button
		if (x >= noButton.x && x <= (noButton.x + noButton.w) && y >= noButton.y
				&& y <= (noButton.y + noButton.h)) {
			currentPage = PAGE_PROGRESS;
			DrawProgressPage("FILL", rx_byte, rx_data);
		}
		//yes button
		if (x >= yesButton.x && x <= (yesButton.x + yesButton.w)
				&& y >= yesButton.y && y <= (yesButton.y + yesButton.h)) {
			//Send stop signal to ESP32 and go back to main page
			SendStopMotorsMessage();
			currentPage = PAGE_MAIN;
			DrawMainPage(page_num);

			//stop the interrupt and turn fan off
			HAL_TIM_Base_Stop_IT(&htim2);
			HAL_GPIO_WritePin(FAN_EN_GPIO_Port, FAN_EN_Pin, GPIO_PIN_RESET);
		}
		break;

	case PAGE_PROTOCOL_INFO:
		//back button
		if (x >= backButton.x && x <= (backButton.x + backButton.w)
				&& y >= backButton.y && y <= (backButton.y + backButton.h)) {
			currentPage = PAGE_SELECT;
			DrawInfoPage(buttons[i].label);
		}
		break;

	case PAGE_FINISH:
		if (x >= backButton.x && x <= (backButton.x + backButton.w)
				&& y >= backButton.y && y <= (backButton.y + backButton.h)) {
			currentPage = PAGE_MAIN;
			DrawMainPage(page_num);
		}
		break;
	}

	HAL_Delay(SCREEN_DELAY); //debouncing delay
	return 1;
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

	//write existing protocols and new protocol to flash memory, string-by-string
	erase_sector(sector); //erase sector before writing
	HAL_FLASH_Unlock(); // Unlock flash for writing. note: the erase_sector() unlocks and locks itself
	for (i = 0; i < (num_protocols + 1); i++) {
		for (j = 0; j < MAX_LINES; j++) {
			//if first character of a line is null from qr scanner or trash from flash, ignore it
			if ((new_protocol[j][0] != '\0') && (new_protocol[j][0] != 0xFF)) {
				uint32_t temp_address = flash_address + i * PROTOCOL_SIZE
						+ j * MAX_LINE_LENGTH;
				//printf("len: %d and data: %s", strlen(protocolStorage[i][j]), protocolStorage[i][j]);
				write_to_flash(protocolStorage[i][j], temp_address);
			}
		}
	}
	HAL_FLASH_Lock();  // Unlock flash for writing

	//output:flash memory now has the lines of protocol storage in there
	//the remaining space is still 0xFF in flash
	return 1;
}

//return number of protocols in the sector
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
			HAL_UART_Transmit(&huart2, (uint8_t*) output, strlen(output),
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
	uint8_t last_line = 0;
	for (uint8_t i = 0; i < MAX_LINES; i++) {
		memset(output, 0, MAX_LINE_LENGTH); // Sets all elements of buffer to 0
		if (read_from_flash(output, flash_address + i * MAX_LINE_LENGTH)) {
			strcpy(queueBuffer[queueSize][i], output);
			last_line = i;
		}
	}
	//dont store the tab operator of protocol for queueing
	queueBuffer[queueSize][last_line][strlen(queueBuffer[queueSize][last_line])
			- 1] = NULL_CHAR; //strlen gives the size of string. tab operator is size-1
}

/**
 * @brief: Transmit queued protocols from RAM buffer to ESP32 via UART string-by-string
 * @param queueSize:
 */
void transmitQueuedProtocols(uint8_t queueSize) {
	//i need to send a filler title
	//printf("FillerTitle\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) "Filler\r\n", strlen("Filler\r\n"),
	HAL_MAX_DELAY);

	//transmit the queued protocols
	for (uint8_t i = 0; i < queueSize; i++) {
		for (uint8_t j = 1; j < MAX_LINES; j++) {
			//printf(queueBuffer[i][j]);
			HAL_UART_Transmit(&huart2, (uint8_t*) queueBuffer[i][j],
					strlen(queueBuffer[i][j]), HAL_MAX_DELAY);
		}
	}
//	//debugging print
//	for (int a = 0; a < queueSize; a++) {
//		printf("%s\n", queueBuffer[a]);
//	}
	//finish with a tab operator
	HAL_UART_Transmit(&huart2, (uint8_t*) "\t", 1, HAL_MAX_DELAY);
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
//void deleteProtocol(uint32_t sector, uint32_t offset) {
//	//initialize variables
//	int i = 0;  //protocol index
//	int j = 0;  //line index
//	char protocolStorage[MAX_PROTOCOLS_IN_SECTOR][MAX_LINES][MAX_LINE_LENGTH] =
//			{ { { '\0' } } };
//	uint32_t flash_address = get_sector_address(sector);
//	char trashLine[MAX_LINE_LENGTH];
//	memset(trashLine, 0xFF, sizeof(trashLine));
//	char nullLine[MAX_LINE_LENGTH];
//	memset(nullLine, 0x00, sizeof(nullLine));
//	uint8_t num_protocols = get_num_protocols_in_sector(sector);
//
//	//copy all protocols in sector to local buffer
//	for (i = 0; i < num_protocols; i++) {
//		for (j = 0; j < MAX_LINES; j++) {
//			//get the address for the current line in the current protocol
//			uint32_t temp_address = flash_address + i * PROTOCOL_SIZE
//					+ j * MAX_LINE_LENGTH;
//			//read lines from memory into buffer until you reach garbage
//			read_from_flash(protocolStorage[i][j], temp_address); //only updates protocolStorage if its a valid line from memory
//		}
//	}
//
//	//write existing protocols except the one to be deleted back to flash
//	erase_sector(sector); //erase sector before writing
//	HAL_FLASH_Unlock(); // Unlock flash for writing. note: the erase_sector() unlocks and locks itself
//	for (i = 0; i < num_protocols; i++) {
//		for (j = 0; j < MAX_LINES; j++) {
//			//if first character of a line is null from qr scanner or trash from flash, ignore it
//			uint32_t temp_address = flash_address + i * PROTOCOL_SIZE
//					+ j * MAX_LINE_LENGTH;
//			if (i != offset) {
//				//printf("len: %d and data: %s", strlen(protocolStorage[i][j]), protocolStorage[i][j]);
//				write_to_flash(protocolStorage[i][j], temp_address);
//			}
//
//		}
//	}
//	HAL_FLASH_Lock();  // Unlock flash for writing
//}

void SendStopMotorsMessage(void) {
	//printf("S\n");
	HAL_UART_Transmit(&huart2, (uint8_t*) "S", 1, HAL_MAX_DELAY);
	rx_byte = 0; //otherwise when you run a new protocol is has old info
}

// Function to write a uint32_t number to flash memory
void write_number_to_flash(uint32_t flash_address, uint32_t number) {
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_address, number);
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

/*** Touchscreen Interrupt Handler ****/
//t_irq_pin is default high. it gets driven low while the screen is touched.
//the interrupt is triggered on falling edge.
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == T_IRQ_Pin) {

		uint32_t current_time = HAL_GetTick(); // ms since startup

		//perform some debouncing for interrupt pin
		if ((current_time - last_interrupt_time) > DEBOUNCE_DELAY_MS) {
			last_interrupt_time = current_time;

			//handle the actual interrupt below
			uint8_t currentTouchedState = HAL_GPIO_ReadPin(T_IRQ_GPIO_Port,
			T_IRQ_Pin);
			//printf("%d\n", currentTouchedState)
			if (XPT2046_TouchPressed() && !touchFlag) {
				touchFlag = 1;
			}
		}
	}
	if (GPIO_Pin == OTG_FS_OC_Pin) {
		//HAL_GPIO_WritePin(OTG_FS_PSO_GPIO_Port, OTG_FS_PSO_Pin, GPIO_PIN_SET);
	}
}

// This is called every time the timer overflows (i.e., when the interrupt triggers)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2) {
		// Your code here — for example:
		if (protocolTimer > 0) {
			protocolTimer--;
		}
		if (currentPage == PAGE_PROGRESS) {
			lcdSetCursor(10, 170);
			DrawCountdownTime();
			//DrawProgressPage("Fill", rx_byte, rx_data);
		}

	}
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
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
