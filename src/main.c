#include "main.h"

void virtual_GND(void);
void flush_CCD(void);
void NVIC_conf(void);


__IO uint32_t SH_period = 25;
__IO uint32_t ICG_period = 500000;
__IO uint16_t aTxBuffer[CCDSize];
__IO uint8_t aRxBuffer[2*CCDSize];

__IO uint8_t change_exposure_flag = 0;
__IO uint8_t transmit_data_flag = 0;
__IO uint8_t pc_ready_flag = 0;
__IO uint8_t pulse_counter = 0;
__IO uint8_t CCD_flushed = 0;



/* TIM Configuration
	TIM2/5 are 32 bit and will serve the ICG and SH pulses that may require very long periods.
	TIM3/4 are 16 bit and will serve the fM (master clock) and ADC clock. 

	TIM2 drives SH on PB10
	TIM3 drives fM on PB1 
	TIM4 paces the ADC
	TIM5 drives ICG on PA1

	ADC-In is on PC3 */

/* UART Configuration
	Tx on PA3
	Rx on PA2 */

/* Other GPIOs
	PA0, PB0, PB2 and PC2 are driven low
	PA5 (LED) is enabled (but driven low by default) */


int main(void)
{
	/* virtual_GND() enables GPIOA, GPIOB and GPIOC clocks */
	virtual_GND();
	NVIC_conf();

	/* Setup CCD_fM (TIM3) and ADC-timer (TIM4) */
	get_Timer_clocks();
	TIM_CCD_fM_conf();
	TIM_ADC_conf();

	/* Setup communication */
	//USART2_conf();
	SPI2_conf();

	/* Setup ADC + ADC-DMA */
	ADC1_conf();

	/* Setup ICG (TIM5) and SH (TIM2) */
	TIM_ICG_SH_conf();


	//flush_CCD();




	while(1)
	{
		if (change_exposure_flag == 1)
		{
			/* reset flag */
			change_exposure_flag = 0;

			flush_CCD();

			/* set new integration time */
			ICG_period = aRxBuffer[6]<<24|aRxBuffer[7]<<16|aRxBuffer[8]<<8|aRxBuffer[9];
			SH_period = aRxBuffer[2]<<24|aRxBuffer[3]<<16|aRxBuffer[4]<<8|aRxBuffer[5];

			/*	Disable ICG (TIM5) and SH (TIM2) before reconfiguring*/
			TIM_Cmd(TIM2, DISABLE);
			TIM_Cmd(TIM5, DISABLE);

			/* 	Reconfigure TIM2 and TIM5 */
			TIM_ICG_SH_conf();
		}
/*	Communication is controlled by the rpi, and the rpi must know how long to wait
	for data collection to finish. */
//		if ((transmit_data_flag == 1)&&(pc_ready_flag == 1))
//		{
//			/* reset flags */
//			transmit_data_flag = 0;
//			pc_ready_flag = 0;
//
//			/* Transmit data */
//			UART2_Tx_DMA();
//		}
	}

}



/* 	To keep noise-level on ADC-in down, the following GPIO's are
	set as output, driven low and physically connected to GND:
		PA0 and PC2 which are physically close to PC3 (ADC-in)
		PB0 and PB2 which are physically close to PB1 - the most busy GPIO (fM) */
void virtual_GND(void)
{
	GPIO_InitTypeDef    	GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;

	/* 	Clock the GPIOs */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);  
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	/* PA0 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

	/* PC2 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

	/* PB0 and PB2 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

	/* Setup LED (PA5) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}


/* Run this function prior to datacollection */
void flush_CCD()
{
	/* Set exposure very low */
	ICG_period = 15000;
	SH_period = 25;

	/*	Disable ICG (TIM5) and SH (TIM2) before reconfiguring*/
	TIM_Cmd(TIM2, DISABLE);
	TIM_Cmd(TIM5, DISABLE);

	/*	Reset flags and counters */
	CCD_flushed = 0;
	pulse_counter = 0;

	/* 	Reconfigure TIM2 and TIM5 */
	TIM_ICG_SH_conf();

	/*	Block until CCD is properly flushed */
	while(CCD_flushed == 0);
}


/* Configure interrupts */
void NVIC_conf(void)
{
	NVIC_InitTypeDef		NVIC_InitStructure;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	/* ICG (TIM5) IRQ */
	/* The TIM5 update interrupts starts TIM4 and ADC */
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* ADC-DMA IRQ */
	/* DMA1 Transfer complete interrupt stops TIM4 and ADC */
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* SPI-DMA IRQ */
	/* DMA1 Transfer complete interrupt checks incoming data */
	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Stream4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

}


