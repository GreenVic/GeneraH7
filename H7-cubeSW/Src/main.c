/* USER CODE BEGIN Header */
//
// Genera Eurorack H7 board
// by Snyderphonics
//
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "i2c.h"
#include "quadspi.h"
#include "rng.h"
#include "sai.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "audiostream.h"
#include "leaf.h"

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

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

//these are by rough testing measurement, to even out the control of the RGB led so each color is equal in strength
uint16_t redScaled[17] = {0, 6550, 6660, 6750, 6840, 7000, 7080, 7170, 7250, 7350, 7550, 7900, 8200, 8500, 9300, 9900, 11000};
uint16_t greenScaled[17] = {0, 8, 9, 10, 11, 13, 16, 22, 30, 40, 60, 110, 180, 300, 400, 700, 900};
uint16_t blueScaled[17] = {0, 8, 9, 10, 11, 14, 17, 22, 25, 30, 40, 50, 70, 90, 150, 250, 300};

//the typedef for assigning jack functions
typedef enum
{
	DIGITAL_INPUT = 0,
	DIGITAL_OUTPUT,
	ANALOG_INPUT,
	ANALOG_OUTPUT,
	AUDIO_INPUT,
	AUDIO_OUTPUT
}jackModeType;


#define NUM_ADC_CHANNELS 12
uint16_t myADC[NUM_ADC_CHANNELS] __ATTR_RAM_D2;
int counter = 0;
int internalcounter = 0;
int DAC1_active = 0;
int DAC2_active = 0;

void RGB_LED_setColor(uint8_t R, uint8_t G, uint8_t B);
void configure_Jack(uint8_t jackNumber, jackModeType jackMode);
void Invalid_Configuration(void);
void CV_DAC_Output(uint8_t DACnum, uint16_t value);

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
void MPU_Conf(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  MPU_Conf();

  SystemInit();
  /* USER CODE END 1 */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  // this code sets the processor to treat denormal numbers (very tiny floats) as zero to improve performance.
  uint32_t tempFPURegisterVal = __get_FPSCR();
  tempFPURegisterVal |= (1<<24); // set the FTZ (flush-to-zero) bit in the FPU control register
  __set_FPSCR(tempFPURegisterVal);

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_I2C2_Init();
  MX_QUADSPI_Init();
  MX_RNG_Init();
  MX_SAI1_Init();
  MX_SPI4_Init();
  MX_TIM3_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */

  //set up the timers that control PWM dimming on the LEDs
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);

  //and set the PWM values for those LEDs
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);


  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);



	if (HAL_ADC_Start_DMA(&hadc1,(uint32_t*)&myADC, NUM_ADC_CHANNELS) != HAL_OK)
	{
		Error_Handler();

	}
	audioInit(&hi2c2, &hsai_BlockA1, &hsai_BlockB1, myADC);
	

	  //look at the configure_Jack function for notes on how to set the physical jumpers for each setting
	  configure_Jack(1, ANALOG_INPUT); //jack 1 can be DIGITAL_INPUT, DIGITAL_OUTPUT, or ANALOG_INPUT (CV in)
	     // for Digital or Analog Input ---  jumpers on I/O board channel 5 set to input
	  	 // for Digital Output --- jumpers on I/O board channel 5 set to output

	  configure_Jack(2, ANALOG_INPUT); //jack 2 can be DIGITAL_INPUT, DIGITAL_OUTPUT, or ANALOG_INPUT (CV in)
	     // for Digital or Analog Input ---  jumpers on I/O board channel 6 set to input
	  	 // for Digital Output --- jumpers on I/O board channel 6 set to output

	  configure_Jack(3, ANALOG_OUTPUT); //jack 3 can be DIGITAL_INPUT, DIGITAL_OUTPUT, ANALOG_INPUT (CV in), or ANALOG_OUTPUT (CV out)
	     // for Digital or Analog Input ---  jumpers on I/O board channel 7 set to input, and jumpers A and B to 3
	  	 // for Digital Output --- jumpers on I/O board channel 7 set to output, and jumpers A and B to 3
	     // for Analog Output --- jumpers A and B to 1

	  configure_Jack(4, ANALOG_OUTPUT); //jack 4 can be DIGITAL_INPUT, DIGITAL_OUTPUT, ANALOG_INPUT (CV in), or ANALOG_OUTPUT (CV out)
	     // for Digital or Analog Input ---  jumpers on I/O board channel 8 set to input, and jumpers C and D to 3
	  	 // for Digital Output --- jumpers on I/O board channel 8 set to output, and jumpers C and D to 3
	     // for Analog Output --- jumpers C and D to 1

	  configure_Jack(5, AUDIO_INPUT); //jack 5 can be DIGITAL_INPUT, or AUDIO_INPUT
	  	  // for Digital Input --- set jumper E to 3
	      // for Audio Input --- set jumper E to 1

	  configure_Jack(6, AUDIO_INPUT); //jack 6 can be DIGITAL_INPUT, or AUDIO_INPUT
  	  	  // for Digital Input --- set jumper F to 3
      	  // for Audio Input --- set jumper F to 1

	  configure_Jack(7, AUDIO_OUTPUT); //jack 7 can be DIGITAL_OUTPUT, or AUDIO_OUTPUT
  	  	  // for Digital Output --- set jumper G to 3
	  	  // for Audio Output --- set jumper G to 1

	  configure_Jack(8, AUDIO_OUTPUT); //jack 8 can be DIGITAL_OUTPUT, or AUDIO_OUTPUT
  	  	  // for Digital Output --- set jumper H to 3
      	  // for Audio Output --- set jumper H to 1

	  //comment the following jack configurations in and configure them if you are using a Genera version with 12 knobs and 6 jacks instead of an 8knob/8jack version

	  //configure_Jack(9, DIGITAL_OUTPUT); //jack 9 can be DIGITAL_INPUT, DIGITAL_OUTPUT, or ANALOG_INPUT (CV in)  -- analog input takes over the input for knob 5
		 //for Digital Input --- set "I/O board 1" to input (both jumpers), and set jumper I to 3
		 //for Digital Output --- set "I/O board 1" to output (both jumpers), and set jumper I to 3
		 //for Analog Input --- set "I/O board 1" to input (both jumpers), and set jumper I to 1 and jumper K to 1 (takes away input from knob 5)

	  //configure_Jack(10, DIGITAL_INPUT); //jack 10 can be DIGITAL_INPUT, DIGITAL_OUTPUT, or ANALOG_INPUT (CV in)  -- analog input takes over the input for knob 6
	  	  //for Digital Input --- set "I/O board 2" to input (both jumpers), and set jumper J to 3
	  	  //for Digital Output --- set "I/O board 2" to output (both jumpers), and set jumper J to 3
	  	  //for Analog Input --- set "I/O board 2" to input (both jumpers), and set jumper J to 1 and jumper L to 1 (takes away input from knob 6)

	  //configure_Jack(11, ANALOG_INPUT); //jack 11 can be DIGITAL_INPUT, DIGITAL_OUTPUT, or ANALOG_INPUT (CV in)
	  	  //for Digital and Analog Input --- set "I/O board 3" to input (both jumpers), and set jumper M to 1
	  	  //for Digital Output --- set "I/O board 3" to output (both jumpers), and set jumper M to 1

	  //configure_Jack(12, ANALOG_INPUT); //jack 12 can be DIGITAL_INPUT, DIGITAL_OUTPUT, or ANALOG_INPUT (CV in)
  	  	  //for Digital and Analog Input --- set "I/O board 4" to input (both jumpers), set jumper N to 1, and jumper O to 3
  	  	  //for Digital Output --- set "I/O board 4" to output (both jumpers), set jumper N to 1, and jumper O to 3


	  if (DAC1_active == 1)
	  {
		  HAL_DAC_Start(&hdac1, DAC_CHANNEL_1);
	  }
	  if (DAC2_active == 1)
	  {
		  HAL_DAC_Start(&hdac1, DAC_CHANNEL_2);
	  }

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  //RGB_LED_setColor(255,255,255);
	  if (counter > 200000)
	  {
		  RGB_LED_setColor(internalcounter % 256, internalcounter % 256, internalcounter % 256);
		  //__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, internalcounter % 8000);

		  CV_DAC_Output(1, internalcounter % 2048);
		  CV_DAC_Output(2, internalcounter % 2048);

		 internalcounter++;

		  counter = 0;
	  }
	  counter++;
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /**Supply configuration update enable 
  */
  MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);
  /**Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while ((PWR->D3CR & (PWR_D3CR_VOSRDY)) != PWR_D3CR_VOSRDY) 
  {
    
  }
  /**Macro to configure the PLL clock source 
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSE);
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI
                              |RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 128;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RNG|RCC_PERIPHCLK_SPI4
                              |RCC_PERIPHCLK_SAI1|RCC_PERIPHCLK_I2C2
                              |RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_QSPI
                              |RCC_PERIPHCLK_CKPER;
  PeriphClkInitStruct.PLL2.PLL2M = 25;
  PeriphClkInitStruct.PLL2.PLL2N = 344;
  PeriphClkInitStruct.PLL2.PLL2P = 7;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_0;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_D1HCLK;
  PeriphClkInitStruct.CkperClockSelection = RCC_CLKPSOURCE_HSI;
  PeriphClkInitStruct.Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLL2;
  PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
  PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_HSI48;
  PeriphClkInitStruct.I2c123ClockSelection = RCC_I2C123CLKSOURCE_D2PCLK1;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_CLKP;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// Returns random floating point value [0.0,1.0)
float randomNumber(void) {
	
	uint32_t rand;
	HAL_RNG_GenerateRandomNumber(&hrng, &rand);
	float num = (((float)(rand >> 16))- 32768.f) * INV_TWO_TO_15;
	return num;
}

void CV_DAC_Output(uint8_t DACnum, uint16_t value)  //takes numbers from 0-4096 (if R119 and R121 are replaced with 22k instead of 47k, otherwise only 0-2048)
//note that DAC1 comes out jack 4 and DAC2 comes out jack 3 (I know it makes no sense) - JS
{
	if (DACnum == 1)
	{
		HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_1, DAC_ALIGN_12B_R, value);

	}
	else if (DACnum == 2)
	{
		HAL_DAC_SetValue(&hdac1, DAC_CHANNEL_2, DAC_ALIGN_12B_R, value);
	}
}


void RGB_LED_setColor(uint8_t Red, uint8_t Green, uint8_t Blue) //inputs between 0-255
{
	float floatyPoint;
	uint8_t intPart;
	float fractPart;
	float endValue;

	floatyPoint = ((float)Red) * 0.0625f;
	intPart = (uint8_t)floatyPoint;
	fractPart = floatyPoint - ((float)intPart);
	endValue = (redScaled[intPart] * (1.0f - fractPart)) + (redScaled[intPart + 1] * (fractPart));
	__HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, (uint16_t) endValue);

	floatyPoint = ((float)Green) * 0.0625f;
	intPart = (uint8_t)floatyPoint;
	fractPart = floatyPoint - ((float)intPart);
	endValue = (greenScaled[intPart] * (1.0f - fractPart)) + (greenScaled[intPart + 1] * (fractPart));
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint16_t) endValue);

	floatyPoint = ((float)Blue) * 0.0625f;
	intPart = (uint8_t)floatyPoint;
	fractPart = floatyPoint - ((float)intPart);
	endValue = (blueScaled[intPart] * (1.0f - fractPart)) + (blueScaled[intPart + 1] * (fractPart));
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint16_t) endValue);

}

void configure_Jack(uint8_t jackNumber, jackModeType jackMode)
{
	 GPIO_InitTypeDef GPIO_InitStruct;

	if (jackNumber == 1)
	{

		if (jackMode == DIGITAL_INPUT) //be sure to set the jumpers on the IO board for"I/O 5" both to input
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_1;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

		else if (jackMode == DIGITAL_OUTPUT) //be sure to set the jumpers on the IO board for"I/O 5" both to output
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_1;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

		else if (jackMode == ANALOG_INPUT) //be sure to set the jumpers on the IO board for"I/O 5" both to input
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 2)
	{

		if (jackMode == DIGITAL_INPUT) //be sure to set the jumpers on the IO board for"I/O 6" both to input
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_0;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

		else if (jackMode == DIGITAL_OUTPUT) //be sure to set the jumpers on the IO board for"I/O 6" both to output
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_0;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

		else if (jackMode == ANALOG_INPUT) //be sure to set the jumpers on the IO board for"I/O 6" both to input
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 3)
	{

		if (jackMode == DIGITAL_INPUT) //be sure to set the jumpers on the IO board for"I/O 7" both to input
			//also put jumper A on 3 and jumper B on 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_5;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

		else if (jackMode == DIGITAL_OUTPUT) //be sure to set the jumpers on the IO board for"I/O 7" both to output
			//also put jumper A on 3 and jumper B on 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_5;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

		else if (jackMode == ANALOG_INPUT) //be sure to set the jumpers on the IO board for"I/O 7" both to input
			//also put jumper A on 3 and jumper B on 3
		{
			//do nothing, already configured
		}

		else if (jackMode == ANALOG_OUTPUT) //put jumper A on 1 and jumper B on 1
		{
			//keep pin in analog mode (no pull up or down) but initialize it with the DAC
			MX_DAC1_Init(2);
			GPIO_InitStruct.Pin = GPIO_PIN_4;
			GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
			GPIO_InitStruct.Pull = GPIO_NOPULL;
			HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
			DAC2_active = 1;
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 4)
	{

		if (jackMode == DIGITAL_INPUT) //be sure to set the jumpers on the IO board for"I/O 8" both to input
			//also put jumper C on 3 and jumper D on 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_4;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

		else if (jackMode == DIGITAL_OUTPUT) //be sure to set the jumpers on the IO board for"I/O 8" both to output
			//also put jumper C on 3 and jumper D on 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_4;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		}

		else if (jackMode == ANALOG_INPUT) //be sure to set the jumpers on the IO board for"I/O 8" both to input
			//also put jumper C on 3 and jumper D on 3
		{
			//do nothing, already configured
		}

		else if (jackMode == ANALOG_OUTPUT) // put jumper C on 1 and jumper D on 1
		{
			//keep pin in analog mode (no pull up or down) but initialize it with the DAC

			MX_DAC1_Init(1);
			GPIO_InitStruct.Pin = GPIO_PIN_4;
			GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
			GPIO_InitStruct.Pull = GPIO_NOPULL;
			HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
			DAC1_active = 1;
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 5)
	{

		if (jackMode == DIGITAL_INPUT) //set jumper E to 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_12;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
		}

		else if (jackMode == AUDIO_INPUT) //set jumper E to 1
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 6)
	{

		if (jackMode == DIGITAL_INPUT) //set jumper F to 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_13;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
		}

		else if (jackMode == AUDIO_INPUT) //set jumper F to 1
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 7)
	{

		if (jackMode == DIGITAL_OUTPUT) //set jumper G to 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_14;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
		}

		else if (jackMode == AUDIO_OUTPUT) //set jumper G to 1
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 8)
	{

		if (jackMode == DIGITAL_OUTPUT) //set jumper H to 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_15;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
		}

		else if (jackMode == AUDIO_OUTPUT) //set jumper H to 1
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 9)
	{

		if (jackMode == DIGITAL_INPUT) //set "I/O board 1" to input (both jumpers), and set jumper I to 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_11;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
		}

		else if (jackMode == DIGITAL_OUTPUT) //set "I/O board 1" to output (both jumpers), and set jumper I to 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_11;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
		}

		else if (jackMode == ANALOG_INPUT) //set "I/O board 1" to input (both jumpers), and set jumper I to 1 and jumper K to 1 (takes away input from knob 5)
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 10)
	{

		if (jackMode == DIGITAL_INPUT) //set "I/O board 2" to input (both jumpers), and set jumper J to 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_12;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
		}

		else if (jackMode == DIGITAL_OUTPUT) //set "I/O board 2" to output (both jumpers), and set jumper J to 3
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_12;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
		}

		else if (jackMode == ANALOG_INPUT) //set "I/O board 2" to input (both jumpers), and set jumper J to 1 and jumper L to 1 (takes away input from knob 6)
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 11)
	{

		if (jackMode == DIGITAL_INPUT) //set "I/O board 3" to input (both jumpers), and set jumper M to 1
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_12;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
		}

		else if (jackMode == DIGITAL_OUTPUT) //set "I/O board 3" to output (both jumpers), and set jumper M to 1
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_12;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
		}

		else if (jackMode == ANALOG_INPUT) //set "I/O board 3" to input (both jumpers), and set jumper M to 1
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}

	else if (jackNumber == 12)
	{

		if (jackMode == DIGITAL_INPUT) //set "I/O board 4" to input (both jumpers), set jumper O to 3, and jumper N to 1
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_12;
			  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
			  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
			  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
		}

		else if (jackMode == DIGITAL_OUTPUT) //set "I/O board 4" to output (both jumpers), set jumper O to 3, and jumper N to 1
		{
			  GPIO_InitStruct.Pin = GPIO_PIN_12;
			  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
			  GPIO_InitStruct.Pull = GPIO_NOPULL;
			  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
			  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
		}

		else if (jackMode == ANALOG_INPUT) //set "I/O board 4" to input (both jumpers), set jumper O to 3, and jumper N to 1
		{
			//do nothing, already configured
		}

		else
		{
			Invalid_Configuration();
		}
	}
}

void Invalid_Configuration(void)
{
	//just an error function so that you can put a breakpoint here while debugging to check for invalid jack configurations
	;
}

void MPU_Conf(void)
{
	//code from Keshikan https://github.com/keshikan/STM32H7_DMA_sample
  //Thanks, Keshikan! This solves the issues with accessing the SRAM in the D2 area properly. -JS
	
	MPU_Region_InitTypeDef MPU_InitStruct;

	  HAL_MPU_Disable();

	  MPU_InitStruct.Enable = MPU_REGION_ENABLE;

	  //D2 Domain�SRAM1
	  MPU_InitStruct.BaseAddress = 0x30000000;
	  // Increased region size to 256k. In Keshikan's code, this was 512 bytes (that's all that application needed).
	  // Each audio buffer takes up the frame size * 8 (16 bits makes it *2 and stereo makes it *2 and double buffering makes it *2)
	  // So a buffer size for read/write of 4096 would take up 64k = 4096*8 * 2 (read and write).
	  // I increased that to 256k so that there would be room for the ADC knob inputs and other peripherals that might require DMA access.
	  // we have a total of 256k in SRAM1 (128k, 0x30000000-0x30020000) and SRAM2 (128k, 0x30020000-0x3004000) of D2 domain. 
	  // There is an SRAM3 in D2 domain as well (32k, 0x30040000-0x3004800) that is currently not mapped by the MPU (memory protection unit) controller.
	  
	  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;

	  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;

	  //AN4838
	  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
	  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;

	  //Shared Device
//	  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
//	  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
//	  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
//	  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;


	  MPU_InitStruct.Number = MPU_REGION_NUMBER0;

	  MPU_InitStruct.SubRegionDisable = 0x00;


	  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;


	  HAL_MPU_ConfigRegion(&MPU_InitStruct);


	  //now set up D3 domain RAM

	  MPU_InitStruct.Enable = MPU_REGION_ENABLE;

	 	  //D2 Domain�SRAM1
	 	  MPU_InitStruct.BaseAddress = 0x38000000;


	 	  MPU_InitStruct.Size = MPU_REGION_SIZE_64KB;

	 	  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;

	 	  //AN4838
	 	  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL1;
	 	  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
	 	  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
	 	  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;

	 	  //Shared Device
	 //	  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	 //	  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	 //	  MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
	 //	  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;


	 	  MPU_InitStruct.Number = MPU_REGION_NUMBER1;

	 	  MPU_InitStruct.SubRegionDisable = 0x00;


	 	  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;


	 	  HAL_MPU_ConfigRegion(&MPU_InitStruct);


	  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
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
  while(1) 
  {

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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
