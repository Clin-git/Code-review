#include "main.h"
#include "Picocap_app.h"
#include "adc_app.h"
#include "dac_bsp.h"
#include "iwdg_bsp.h"
#include "ModBusRtu.h"
#include "In_Flash_app.h"

/* 系统时钟配置 */
void SystemClock_Config(void);


int main(void)
{
    uint32_t                    FilterResult;           //滤波后的结果
    DataFilterParam             FilterParam;            //数据滤波需要的参数结构体
    PCap_DataConvert_Param      DataConvert_Param;      //PCap做数据转换需要的参数结构体
    PCap_DataConvert_Out_Param  DataConvert_Out;        //PCap转换后输出数据的结构体
    ADC_TemperParam_TypeDef     ADC_TemperParam;        //ADC温度处理需要的参数结构体
    ADC_TemperOut_TypeDef       ADC_TemperOut;          //ADC温度处理输出数据的结构体
    ModBusBaseParam_TypeDef     ModBusBaseParam;        //ModBus处理的基本参数结构
    ModBus_Device_Param         ModBus_Device;          //ModBus管理设备的参数结构体

    ModBus_Device.ADC_TemperParam         = &ADC_TemperParam;
    ModBus_Device.ADC_TemperOut           = &ADC_TemperOut;
    ModBus_Device.DataFilter              = &FilterParam;
    ModBus_Device.PCap_DataConvert        = &DataConvert_Param; 
    ModBus_Device.PCap_DataConvert_Out    = &DataConvert_Out;
    
    HAL_Init();                                         //HAL库初始化
    SystemClock_Config();                               //系统时钟配置

    Check_Device_Param();                               //检查内部设备数据    
    DataFilterParam_Init(&FilterParam, DATA_BUF_MAX);   //滤波参数初始化
    DataConvertParam_Init(&DataConvert_Param);          //PCap数据转换参数初始化
    Sensor_ADC_TemperParam_Init(&ADC_TemperParam);      //ADC温度处理参数初始化
    PCap_Init();                                        //PCap初始化    
    Sensor_ADC_Init();                                  //ADC初始化
    BSP_DAC_Init();                                     //DAC初始化
    BSP_IWDG_Init();                                    //独立看门狗初始化
    ModBus_Init(&ModBusBaseParam);                      //ModBus初始化(包括串口初始化)
    
    while (1)
    {
        //看门狗喂狗
        BSP_IWDG_Refresh();
        //ModBus处理
        ModbusHandle(&ModBusBaseParam, &ModBus_Device);
        //读取PCap数据并判断返回状态,成功状态则进行滤波和数据转换
        if(Sensor_PCap_GetResult(RESULT_REG1_ADDR, 
                                &ModBus_Device.PCap_DataConvert_Out->PCap_ResultValue) == OP_SUCCESS)
        {
            //数据滤波并判断是否成功
            if(Sensor_DataFilter(&FilterParam, 
                        ModBus_Device.PCap_DataConvert_Out->PCap_ResultValue, &FilterResult) == OP_SUCCESS)
            {
                //数值转换
                Sensor_PCap_DataConvert(&DataConvert_Param, 
                                        FilterResult, 
                                        &DataConvert_Out);
                //DA转换,使用通道2
                BSP_DAC_Convert(DataConvert_Out.PCapDA_OutValue, DA_CHANNEL_2);
            }
        }
        //ADC处理
        Sensor_ADC_Data_Handler();
        //判断ADC是否被更新
        if(Sensor_ADC_Get_Updata_Flag() == UPDATA_OK)
        {
            //获取AD转换出的温度值
            ADC_TemperOut.TemperInAir = Sensor_ADC_Get_TemperData();
            //清除ADC更新标志
            Sensor_ADC_Clean_Updata_Flag();
        }
        //传感器事件处理,一般为自动上传
        SensorEvent(&ModBusBaseParam, &ModBus_Device);
    }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_HSI14
                              |RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI14State = RCC_HSI14_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI14CalibrationValue = 16;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV1;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

