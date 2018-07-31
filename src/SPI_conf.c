/*-
 * Copyright (c) 2017 Esben Rossel
 * All rights reserved.
 *
 * Author: Esben Rossel <esbenrossel@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include "main.h"

extern __IO uint16_t aTxBuffer[CCDSize];
extern __IO uint8_t aRxBuffer[2*CCDSize];


void SPI2_conf(void)
{
/*	Setup of the SPI+DMA peripherals 
	SPI2 is configured as follows:

	SCLK on PB13
	MISO on PB14
	MOSI on PB15
	NSS is not needed and not configured

	DMA1 stream 4 and 3 handles the transmitted and received
	data respectively.

	Once this function is run communication has been set up
	and runs whenever the Raspberry Pi demands it.

	Furthermore a DMA interrupt is generated at the end of
	each transfer. Interrupt routines are located in stm32f4xx_it.c

	Notice that the Raspberry Pi's SPI can only handle 8b data
	but the ADC delivers 12b into a 16b array. This causes an
	endianness-like problem on the Raspberry Pi side. 
		eg. a uint16 like 0x0102 is received as 0x02 0x01 
	This is handled on the Raspberry side with 
		proper_data[i] = (unsigned int)Rx_data[i+1] << 8 | Rx_data[i];	*/

	GPIO_InitTypeDef	GPIO_InitStructure;
	DMA_InitTypeDef		DMA_InitStructure;
	SPI_InitTypeDef		SPI_InitStructure;

/*	Peripheral Clock Enable -------------------------------------------------*/
/* 	Enable the SPI clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
  
/* 	Enable GPIO clocks */
//	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); //already clocked in virtual_GND()
  
/*	Enable DMA clock */
  	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

  
/*	Connect SPI pins to AF5 */  
/*	NSS is not needed and therefore commented out */	
	//GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_SPI2);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_SPI2);    
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource15, GPIO_AF_SPI2);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;

/*	NSS SCK pin configuration */
/*	This is commented out because we don't need the slave select feature, 
	hence the NSS_Soft flag 25 lines from here. */
	//GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	//GPIO_Init(GPIOB, &GPIO_InitStructure);

/*	SPI SCK pin configuration */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
  
/*	SPI  MISO pin configuration */
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_14;
	GPIO_Init(GPIOB, &GPIO_InitStructure);  

/* 	SPI  MOSI pin configuration */
  	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_15;
  	GPIO_Init(GPIOB, &GPIO_InitStructure);
 
/*	SPI configuration -------------------------------------------------------*/
/*	Raspberry Pi must be master, and can only handle 8b data*/
	SPI_I2S_DeInit(SPI2);
	SPI_InitStructure.SPI_Mode = SPI_Mode_Slave;		
	SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; 
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; 
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;			
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; 
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_Init(SPI2, &SPI_InitStructure);
  
/*	DMA configuration -------------------------------------------------------*/
/*	Deinitialize DMA Streams */
	DMA_DeInit(DMA1_Stream4);
	DMA_DeInit(DMA1_Stream3);
  
/*	Configure DMA Initialization Structure */
	DMA_InitStructure.DMA_BufferSize = 2*CCDSize; 
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable ;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_1QuarterFull ;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single ;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_PeripheralBaseAddr =(uint32_t) (&(SPI2->DR)) ;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;

/*	Configure TX DMA */
	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral ;
	DMA_InitStructure.DMA_Memory0BaseAddr =(uint32_t)aTxBuffer ;
	DMA_Init(DMA1_Stream4, &DMA_InitStructure);

/*	Configure RX DMA */
	DMA_InitStructure.DMA_Channel = DMA_Channel_0 ;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory ;
	DMA_InitStructure.DMA_Memory0BaseAddr =(uint32_t)aRxBuffer ; 
	DMA_Init(DMA1_Stream3, &DMA_InitStructure);

/*	DMA1 interrupt configuration */
	DMA_ITConfig(DMA1_Stream4, DMA_IT_TC, ENABLE);



/*	Enable the SPI+DMA. From here on communication is controlled by
	the Raspberry Pi */

/*	Enable DMA SPI TX Stream */
	DMA_Cmd(DMA1_Stream4,ENABLE);		

/*	Enable DMA SPI RX Stream */
	DMA_Cmd(DMA1_Stream3,ENABLE);  
  
/*	Enable SPI DMA TX Requsts */
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Tx, ENABLE);

/*	Enable SPI DMA RX Requsts */
	SPI_I2S_DMACmd(SPI2, SPI_I2S_DMAReq_Rx, ENABLE);

/*	Enable the SPI peripheral */
	SPI_Cmd(SPI2, ENABLE);
}

