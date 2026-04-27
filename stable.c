/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bmi270.h"
#include <stdio.h>
#include <string.h> // Added for safety with strings

#define BMI270_CS_Pin GPIO_PIN_1
#define BMI270_CS_GPIO_Port GPIOB
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */
extern const uint8_t bmi270_config_file[];
#define BMI270_CHIP_ID_ADDR 0x80  // 0x00 | 0x80 for Read
#define BMI270_CMD_REG      0x7E
#define BMI270_PWR_CONF     0x7C
#define BMI270_PWR_CTRL     0x7D
#define BMI270_INIT_CTRL    0x59
#define BMI270_INIT_ADDR_0  0x5B
#define BMI270_INIT_ADDR_1  0x5C
#define BMI270_INIT_DATA    0x5E
#define BMI270_INTERNAL_STAT 0x21

uint8_t chipID = 0;
uint8_t sensor_data[12]; // To hold Accel and Gyro raw bytes
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */
void BMI270_WriteReg(uint8_t reg, uint8_t data);
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

PUTCHAR_PROTOTYPE
{
  extern UART_HandleTypeDef huart3; // Use huart3 for Nucleo USB
  HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int8_t user_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    uint8_t tx_addr = reg_addr | 0x80; // Set bit 7 for Read
    uint8_t rx_buf[len + 1];           // Buffer for dummy + data
    memset(rx_buf, 0, len + 1);

    HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_RESET);

    // 1. Send the address
    if(HAL_SPI_Transmit(&hspi1, &tx_addr, 1, 100) != HAL_OK) {
        HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_SET);
        return -1;
    }

    // 2. Receive Dummy Byte + Actual Data
    if(HAL_SPI_Receive(&hspi1, rx_buf, len + 1, 100) != HAL_OK) {
        HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_SET);
        return -1;
    }

    HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_SET);

    // 3. Data starts at index 1 (rx_buf[0] is the dummy/empty byte)
    for(uint32_t i = 0; i < len; i++) {
        reg_data[i] = rx_buf[i + 1];
    }

    return 0;
}


void user_delay_us(uint32_t period, void *intf_ptr) {
    // Force a slower delay for the H7. 20ms becomes 30ms.
    HAL_Delay((period / 1000) + 10);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

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
  MX_SPI1_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
    uint8_t status = 0;

    // 1. Force SPI Mode (Toggle CSB)
    HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(5);

    // 2. READ SENSOR ID
    user_spi_read(0x00, &chipID, 1, NULL);

    // Bit-shift fix for the current clock mismatch
    if(chipID == 0x48) chipID = chipID >> 1;

    printf("BMI270 Chip ID Detected: 0x%02X\r\n", chipID);

    if(chipID != 0x24) {
        printf("Wrong Chip ID! Check SPI clock speed.\r\n");
    }

    // 3. SOFT RESET
    printf("Resetting sensor...\r\n");
    BMI270_WriteReg(BMI270_CMD_REG, 0xB6);
    HAL_Delay(100); // Increased delay for slower clock

    // 4. IMPORTANT: Re-select SPI mode after Reset
    HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(10);

    // 5. Prepare for Config (Disable advanced power save)
    BMI270_WriteReg(BMI270_PWR_CONF, 0x00);
    HAL_Delay(10);
    BMI270_WriteReg(BMI270_INIT_CTRL, 0x00);
    HAL_Delay(10);

    // 6. UPLOAD CONFIG (Ignore Receiver)
        printf("Starting Upload (Ignoring MISO)...\r\n");
        uint8_t init_addr = BMI270_INIT_DATA;

        // Change SPI to Transmit Only for the upload duration
        // Correct H7 way to set Simplex Transmit (ignores MISO)
        hspi1.Instance->CFG2 |= SPI_CFG2_COMM_0;

        HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit(&hspi1, &init_addr, 1, 100);
        HAL_SPI_Transmit(&hspi1, (uint8_t*)bmi270_config_file, 8192, 1000);
        HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_SET);

        // Switch back to 2-way communication
        // Restore Full-Duplex (2-way) communication
        hspi1.Instance->CFG2 &= ~SPI_CFG2_COMM;

        printf("Upload finished! Reconnect MISO now.\r\n");
        BMI270_WriteReg(BMI270_INIT_CTRL, 0x01); // Tell BMI270 to start firmware boot
        HAL_Delay(500); // Give it a long time to boot (0.5s)

    // 7. CHECK STATUS
    user_spi_read(0x21, &status, 1, NULL);
    if((status & 0x0F) == 0x01) {
        printf("BMI270 Initialization SUCCESS!\r\n");
    } else {
        printf("Initialization FAILED. Status: 0x%02X\r\n", status);
        // If 0x00, the 8KB file didn't load. Check SPI wires.
    }

    // 8. ENABLE SENSORS
    BMI270_WriteReg(BMI270_PWR_CTRL, 0x03);
    BMI270_WriteReg(BMI270_PWR_CONF, 0x02);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    /* Infinite loop */
      /* USER CODE BEGIN WHILE */
    // Enable Accel (bit 2) and Gyro (bit 1)
    // Register 0x7D is PWR_CTRL
    // 1. Set Accel Range to ±2g (Default, but good to force)
    // Register 0x41 (ACC_RANGE), Value 0x00 = ±2g
    BMI270_WriteReg(0x41, 0x00);

    // 2. Set Accel Config (ODR & Bandwidth)
    // Register 0x40 (ACC_CONF)
    // Value 0xA8: ODR = 100Hz, Normal Mode
    BMI270_WriteReg(0x40, 0xA8);

    // 3. Force the Power Control again to be sure
    // Register 0x7D (PWR_CTRL): bit 2=Accel, bit 1=Gyro, bit 3=Temp
    BMI270_WriteReg(0x7D, 0x0E);
    HAL_Delay(50);
      while (1)
      {
          // Read 12 bytes starting from Register 0x0C (Accel + Gyro data)
          if (user_spi_read(0x0C, sensor_data, 12, NULL) == 0) {

              // 1. Convert raw bytes to signed 16-bit integers
              int16_t acc_x = (int16_t)((sensor_data[1] << 8) | sensor_data[0]);
              int16_t acc_y = (int16_t)((sensor_data[3] << 8) | sensor_data[2]);
              int16_t acc_z = (int16_t)((sensor_data[5] << 8) | sensor_data[4]);

              // 2. PASTE THE CONVERSION CODE HERE:
              float g_x = (float)acc_x / 16384.0f;
              float g_y = (float)acc_y / 16384.0f;
              float g_z = (float)acc_z / 16384.0f;

              // 3. Update the printf to use the new float variables
              printf("ACC [g]: X=%.2f Y=%.2f Z=%.2f\r\n", g_x, g_y, g_z);
              // Gyro data starts at index 6
              int16_t gyr_x = (int16_t)((sensor_data[7] << 8) | sensor_data[6]);
              int16_t gyr_y = (int16_t)((sensor_data[9] << 8) | sensor_data[8]);
              int16_t gyr_z = (int16_t)((sensor_data[11] << 8) | sensor_data[10]);

              // Convert to Degrees per Second (dps)
              // Default range is ±2000 dps -> Sensitivity is 16.384 LSB/dps
              float dps_x = (float)gyr_x / 16.384f;
              float dps_y = (float)gyr_y / 16.384f;
              float dps_z = (float)gyr_z / 16.384f;

              printf("GYR [dps]: X=%.2f Y=%.2f Z=%.2f\r\n", dps_x, dps_y, dps_z);
          }

          HAL_Delay(100);
        /* USER CODE END WHILE */
      }
    /* USER CODE BEGIN 3 */
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 2;
  RCC_OscInitStruct.PLL.PLLN = 24;
  RCC_OscInitStruct.PLL.PLLP = 10;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 0x0;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
  hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
  hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
  hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
  hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
  hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
  hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
  hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : BMI270_CS_Pin */
  GPIO_InitStruct.Pin = BMI270_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BMI270_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void BMI270_WriteReg(uint8_t reg, uint8_t data) {
    HAL_GPIO_WritePin(BMI270_CS_GPIO_Port, BMI270_CS_Pin, GPIO_PIN_RESET); // CS LOW
    HAL_SPI_Transmit(&hspi1, &reg, 1, 100);              // Send Reg Address
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);             // Send Data
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);   // CS HIGH
}
/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
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
