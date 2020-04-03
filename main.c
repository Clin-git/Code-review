#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h"
/************************************************
 ALIENTEK 阿波罗STM32H7开发板 实验1
 跑马灯实验-HAL库版本
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/

int main(void)
{
	Cache_Enable();                 //打开L1-Cache
	HAL_Init();				        //初始化HAL库
	Stm32_Clock_Init(160,5,2,4);    //设置时钟,400Mhz 
	delay_init(400);				//延时初始化
	uart_init(115200);				//串口初始化
	LED_Init();						//初始化LED
	while(1)
	{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);	//PB0置0
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET); //PB1置0
		delay_ms(500);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);	//PB0置1
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);	//PB1置1
		delay_ms(500);
	}
}

//使用寄存器操作实现跑马灯
/*int main(void)
{ 
    Cache_Enable();                 //打开L1-Cache
    HAL_Init();				        //初始化HAL库
	Stm32_Clock_Init(160,5,2,2,2);  //设置时钟,400Mhz 
	delay_init(400);				//延时初始化
	uart_init(115200);				//串口初始化
	LED_Init();						//初始化LED
	while(1)
	{
      GPIOB->BSRRL=GPIO_PIN_0;      //LED0亮
	  GPIOB->BSRRH=GPIO_PIN_1;    	//LED1灭
	  delay_ms(500);               	//延时500ms
      GPIOB->BSRRH=GPIO_PIN_0;  	//LED0灭
	  GPIOB->BSRRL=GPIO_PIN_1;    	//LED1亮
	  delay_ms(500);               	//延时500ms
	 }
 }	*/

