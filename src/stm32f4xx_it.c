#include "main.h"

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

extern __IO uint32_t SH_period;
extern __IO uint32_t ICG_period;
extern __IO uint16_t aTxBuffer[CCDSize];
extern __IO uint8_t aRxBuffer[2*CCDSize];

extern __IO uint8_t change_exposure_flag;
extern __IO uint8_t transmit_data_flag;
extern __IO uint8_t pc_ready_flag;
extern __IO uint8_t pulse_counter;
extern __IO uint8_t CCD_flushed;




/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{

}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f40xx.s/startup_stm32f427x.s).                         */
/******************************************************************************/

/*	This interrupt is envoked when SPI-transmission is completed */
void DMA1_Stream4_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_Stream4, DMA_IT_TCIF4)) 
	{
		/* Clear DMA Stream Transfer Complete interrupt */
		DMA_ClearITPendingBit(DMA1_Stream4, DMA_IT_TCIF4);  

		/* Sort aRxBuffer into nRxBuffer */	
		//sort_aRxBuffer(); //this is a safeguard specific for UART

		/* Check the key before doing anything */
	 	if ((aRxBuffer[0]==69)&&(aRxBuffer[1]==82))
		{ 
			/* reset the key */
			aRxBuffer[0] = 0;
			aRxBuffer[1] = 0;

			/* set flags for main-loop */
			change_exposure_flag = 1;
			transmit_data_flag = 0;
			pc_ready_flag = aRxBuffer[10];
		}
	}
//GPIOA->ODR ^= GPIO_Pin_5;
}


/* This interrupt is envoked when the ADC has finished reading the CCD */
void DMA2_Stream0_IRQHandler(void)
{
	/* Test for DMA Stream Transfer Complete interrupt */
	if(DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0))
	{
		/* Clear DMA Stream Transfer Complete interrupt pending bit */
		DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);  
    
		/* Stop TIM4 and thus the ADC */
		TIM4->CR1 &= (uint16_t)~TIM_CR1_CEN;

		/* Set the transmit_data_flag */
		transmit_data_flag = 1;
	
//		GPIOA->ODR ^= GPIO_Pin_5;
	}
}


/* 	This interrupt is envoked when the ICG-pulse is sent,
	ie. when the CCD starts to output */
void TIM5_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM5, TIM_IT_Update))
	{	
		/* Clear TIM5 update interrupt */
		TIM_ClearITPendingBit(TIM5, TIM_IT_Update);
		if (pulse_counter == 6)
		{
			/* Restart TIM4 as this gets the ADC running again */
			TIM4->CR1 |= TIM_CR1_CEN;
		}
		else if (pulse_counter == 3)
		{
			CCD_flushed = 1;
		}
		pulse_counter++;
		/* prevent overflow */
		if (pulse_counter > 10)
			pulse_counter = 10;

		/* Flash the led to the beat of ICG */
		GPIOA->ODR ^= GPIO_Pin_5;
	}

}


/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
