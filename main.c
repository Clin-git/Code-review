#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h"
/************************************************
 ALIENTEK ������STM32H7������ ʵ��1
 �����ʵ��-HAL��汾
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/

int main(void)
{
	Cache_Enable();                 //��L1-Cache
	HAL_Init();				        //��ʼ��HAL��
	Stm32_Clock_Init(160,5,2,4);    //����ʱ��,400Mhz 
	delay_init(400);				//��ʱ��ʼ��
	uart_init(115200);				//���ڳ�ʼ��
	LED_Init();						//��ʼ��LED
	while(1)
	{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_RESET);	//PB0��0
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_RESET); //PB1��0
		delay_ms(500);
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_0,GPIO_PIN_SET);	//PB0��1
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_1,GPIO_PIN_SET);	//PB1��1
		delay_ms(500);
	}
}

//ʹ�üĴ�������ʵ�������
/*int main(void)
{ 
    Cache_Enable();                 //��L1-Cache
    HAL_Init();				        //��ʼ��HAL��
	Stm32_Clock_Init(160,5,2,2,2);  //����ʱ��,400Mhz 
	delay_init(400);				//��ʱ��ʼ��
	uart_init(115200);				//���ڳ�ʼ��
	LED_Init();						//��ʼ��LED
	while(1)
	{
      GPIOB->BSRRL=GPIO_PIN_0;      //LED0��
	  GPIOB->BSRRH=GPIO_PIN_1;    	//LED1��
	  delay_ms(500);               	//��ʱ500ms
      GPIOB->BSRRH=GPIO_PIN_0;  	//LED0��
	  GPIOB->BSRRL=GPIO_PIN_1;    	//LED1��
	  delay_ms(500);               	//��ʱ500ms
	 }
 }	*/

