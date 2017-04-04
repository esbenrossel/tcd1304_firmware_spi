#include "main.h"


extern __IO uint32_t SH_period;
extern __IO uint32_t ICG_period;

/* The timer prescalers are derived from the APB1 clock. */
int apb1_freq;



void get_Timer_clocks(void)
{
	/* Get the apb-prescalers */
	int apb1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> 10;

	/* APBx prescaler table */
	int apb2x[8] = {1,0,0,0,2,4,8,16};

	/* Calculate the timer clocks */
	apb1_freq = SystemCoreClock / (apb2x[apb1]*(apb2x[apb1]==1) + apb2x[apb1]/2*(apb2x[apb1]!=1));
}



/* fM is served by TIM3 on PB1 */
void TIM_CCD_fM_conf(void)
{
	TIM_TimeBaseInitTypeDef	TIM_TimeBaseStructure;
	TIM_OCInitTypeDef		TIM_OCInitStructure;
	GPIO_InitTypeDef    	GPIO_InitStructure;

	/* Clock TIM3 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/*	GPIOB Configuration: TIM3 CH4 (PB1) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	/*	Connect TIM pins to AF2 */
 	GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_TIM3);

	/*	TIM3 Time base configuration 
	Prescaler is 1 so timer clock is apb1_freq / 1 
	Period is apb1_freq / CCD_fm, so timer frequency is CCD_fm in Hz
	(ie. apb1_freq/[apb1_freq/CCD_fm]) */
	TIM_TimeBaseStructure.TIM_Prescaler = 1 - 1;	
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = apb1_freq / CCD_fm - 1;	
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	/*	TIM3 PWM1 Mode configuration: Channel1
	Pulse is apb1_freq / (2*CCD_fm), so the duty cycle is 50% */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = apb1_freq / (2*CCD_fm);
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;

	TIM_OC4Init(TIM3, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM3, ENABLE);

	/*	TIM3 enable counter */
	TIM_Cmd(TIM3, ENABLE);
}


/* ADC is paced by TIM4 (optional output on PB9) */
void TIM_ADC_conf(void)
{
	TIM_TimeBaseInitTypeDef	TIM_TimeBaseStructure;
	TIM_OCInitTypeDef		TIM_OCInitStructure;

	/* Clock TIM3 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

	/*	GPIOB Configuration: TIM4 CH4 (PB9)
	optional - comment out */
	GPIO_InitTypeDef    	GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
  	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

 	GPIO_PinAFConfig(GPIOB, GPIO_PinSource9, GPIO_AF_TIM4);

	/*	TIM4 Time base configuration */
	/*  Prescaler is 1 so timer clock is apb1_freq / 1 
	Period is 4*apb1_freq / CCD_fm, so timer frequency is CCD_fm/4 in Hz
	(ie. apb1_freq/[apb1_freq/CCD_fm]) */
	TIM_TimeBaseStructure.TIM_Prescaler = 1 - 1;	
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = 4 * apb1_freq / CCD_fm - 1;	
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	/*	Pulse is apb1_freq / (2*CCD_fm), so the duty cycle is 12.5% */
	/*	TIM4 PWM1: Channel4 */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = apb1_freq / (2*CCD_fm);
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

	TIM_OC4Init(TIM4, &TIM_OCInitStructure);
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM4, ENABLE);

	/*	TIM4 is enabled/disabled by IRQs */
	//TIM_Cmd(TIM4, ENABLE);
}



/* ---------------------------------------------------------------------------

    TIM2 (channel 3) on PB10 runs the SH-pulse.
    TIM5 (channel 2) on PA1 runs the ICG-pulse

 The following is based on the timing diagrams in the datasheet for TCD1304.

  1) SH must go high with a delay (t2) of between 100 and 1000 ns after ICG goes low.
  2) SH must stay high for (t3) a minium of 1000 ns.
  3) ICG must go high with a delay (t1) of minimum 1000 ns after SH goes low.
  4) ICG must go high when fM is high.

  To help achieve these requirements both timer clocks are set to the period length of fM.
  Furthermore the period of TIM2 should be a multiple of the period of TIM5.

  ####################################################################
  Don't forget that fm, SH and ICG are inverted by the TC74HC04 before
  reaching the CCD. 

 Set TIM_OCInitStructure.TIM_OCPolarity:
     High for TIM3 (fM) and TIM2 (SH)
     Low for TIM5 (ICG)
  when a 74HC04 is NOT used.

  Set TIM_OCInitStructure.TIM_OCPolarity:
     Low for TIM3 (fM) and TIM2 (SH)
     High for TIM5 (ICG)
  when a 74HC04 IS in use.

  ####################################################################


  Notes on the SH and ICG pulses:
     The timing diagrams in the datasheet for TCD1304 can be a little confusing, and
     it doesn't get better that there are two of them.

     The ICG pulse marks the start of each read, so even if it's named 'Integration
     Clear Gate', it seems to function more as a shift gate. Since we want to be able
     to complete a full read of the entire CCD before starting a new read, and a pixel
	 is clocked out for every 4 fM cycles, the period of ICG must be at least:
        3694 * 4 = 14776
	 because the timer controlling ICG runs with a clock speed identical to fM

	 As the SH and ICG pulses must conform to the four criteria above, so it's
     advisable to confine the ICG periods to full multiples of the SH period.

     The SH pulse defines the integration time, and according to the datasheet this
     must be >10 us, so the SH period must be at least:
        10 us * 2.0 MHz = 20
     The upper limit of the SH period is given by the ICG period, as each ICG pulse
     must coincide with an SH pulse. It seems that it's the SH signal that acts as 
     the electronic shutter and not the ICG signal.

	 At each ICG pulse TIM5 creates an interrupt which will start TIM4 that drives
	 the ADC+DMA.


     ###############################################################
     In the following code I advise to only change the values of 
     TIM_Period (and TIM_OCPolarity if you follow the schematic in
     the datasheet and put a 74HC04 between your GPIOs and the CCD).
     ###############################################################

  --------------------------------------------------------------------------- */   


void TIM_ICG_SH_conf(void)
{
	GPIO_InitTypeDef 		GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef	TIM_TimeBaseStructure;
	TIM_OCInitTypeDef		TIM_OCInitStructure;
//	NVIC_InitTypeDef		NVIC_InitStructure;

	/* 	TIM2 & TIM5 clock enable */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	/* GPIO Configuration: TIM2 CH3 (PB10) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(GPIOB, &GPIO_InitStructure); 

	/* 	Connect TIM2 pins to AF1 */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_TIM2);

	/* GPIO Configuration: TIM5 CH2 (PA1) */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
	GPIO_Init(GPIOA, &GPIO_InitStructure); 

	/* 	Connect TIM5 pins to AF2 */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource1, GPIO_AF_TIM5);

	/* 	Time base configuration */
	/* 	The prescaler is set to period of TIM3 (apb1_freq / CCD_fm - 1),
		so TIM2 and TIM5's clocks are equal to the period of TIM3.
		Some numbers to consider for the period of the timers:	
			ICG (TIM5): t_read = Period / CCD_fm MHz
			SH (TIM2): t_int = Period / CCD_fm MHz  
		eg. integration time is:
			t_int = 20 / 2 MHz = 10 µs */

	/* Common for TIM2 & TIM5 */
	TIM_TimeBaseStructure.TIM_Prescaler = apb1_freq / CCD_fm - 1;			
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

	/* TIM5 config */
	TIM_TimeBaseStructure.TIM_Period = ICG_period - 1;			
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);

	/* TIM2 config */
	TIM_TimeBaseStructure.TIM_Period = SH_period - 1;	
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/*	Set parameters for TIM5 PWM1 Mode configuration: Channel2 */
	/* 	The duty cycle should be 5 µs for ICG, so the pulse is:
		pulse = 5 µs * CCD_fm MHz
	Of course this is only accurate when CCD_fM in MHz is an integer.
	Change the Polarity to High if 74HC04 is in place */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = (5 * CCD_fm) / 1000000;						 
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;  
	TIM_OC2Init(TIM5, &TIM_OCInitStructure);

	TIM_OC1PreloadConfig(TIM5, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM5, ENABLE);

	/*	Set parameters for TIM2 PWM1 Mode configuration: Channel3 */
	/* 	The duty cycle should be 2 µs for SH, so the pulse is:
		pulse = 2 µs * CCD_fm MHz
	Of course this is only accurate when CCD_fM in MHz is an integer.
	Change the Polarity to Low if 74HC04 is in place */
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = (2 * CCD_fm) / 1000000;						 
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC3Init(TIM2, &TIM_OCInitStructure);

	TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
	TIM_ARRPreloadConfig(TIM2, ENABLE);


	/*	Clear TIM5 update pending flags */
	TIM_ClearFlag(TIM5, TIM_FLAG_Update);

	/*	Enable the TIM5 Interrupt. Interrupt routine is located in stm32f4xx_it.c */
	TIM_ITConfig(TIM5, TIM_IT_Update, ENABLE);

/* 	TIM5 enable counter */
	TIM_Cmd(TIM5, ENABLE);

/* 	TIM2 enable counter */
	TIM_Cmd(TIM2, ENABLE);

/*	Set counters close to expiration, as the integration times may be very long. 
	(For example: with an ICG-period of 300s we'd have to wait 600s for two ICG-
 	pulses if we don't cut the first one short.)
	The SH-period is slightly delayed to comply with the CCD's timing requirements. */
	TIM5->CNT = ICG_period - ICG_delay;
	TIM2->CNT = SH_period - SH_delay;
	TIM3->CNT = fm_delay;
}


/*	For continous reading of the CCD consider the following:
 	 The data from the CCD is stored in the TxBuffer for the UART, which in best case
	 is 642 ms to complete the transmission. Taking this into account and to allow for
	 some overhead in transmission time the minimum ICG-period becomes:
		700 ms * 2.0 MHz = 1400000
	 with fM = 2.0 MHz. 

	The above is not yet cut in stone. Minimum is still 14776..
*/


