CC      = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy

PROJECT_NAME = F401_FW_TCD1304

PROJECT_SRC = src
STM_SRC = ../Libraries/STM32F4xx_StdPeriph_Driver/src

vpath %.c $(PROJECT_SRC)
vpath %.c $(STM_SRC)

SRCS = main.c

SRCS += Device/startup_stm32f401xe.s

SRCS += stm32f4xx_it.c
SRCS += system_stm32f4xx.c
SRCS += timer_conf.c
SRCS += SPI_conf.c
SRCS += ADC_conf.c
#SRCS += newlib_stubs.c

EXT_SRCS = misc.c
EXT_SRCS += stm32f4xx_adc.c
#EXT_SRCS += stm32f4xx_can.c
#EXT_SRCS += stm32f4xx_cec.c
#EXT_SRCS += stm32f4xx_crc.c
#EXT_SRCS += stm32f4xx_cryp_aes.c
#EXT_SRCS += stm32f4xx_cryp.c
#EXT_SRCS += stm32f4xx_cryp_des.c
#EXT_SRCS += stm32f4xx_cryp_tdes.c
#EXT_SRCS += stm32f4xx_dac.c
#EXT_SRCS += stm32f4xx_dbgmcu.c
#EXT_SRCS += stm32f4xx_dcmi.c
#EXT_SRCS += stm32f4xx_dma2d.c
EXT_SRCS += stm32f4xx_dma.c
#EXT_SRCS += stm32f4xx_exti.c
#EXT_SRCS += stm32f4xx_flash.c
#EXT_SRCS += stm32f4xx_flash_ramfunc.c
#EXT_SRCS += stm32f4xx_fmc.c
#EXT_SRCS += stm32f4xx_fmpi2c.c
#EXT_SRCS += stm32f4xx_fsmc.c
EXT_SRCS += stm32f4xx_gpio.c
#EXT_SRCS += stm32f4xx_hash.c
#EXT_SRCS += stm32f4xx_hash_md5.c
#EXT_SRCS += stm32f4xx_hash_sha1.c
#EXT_SRCS += stm32f4xx_i2c.c
#EXT_SRCS += stm32f4xx_iwdg.c
#EXT_SRCS += stm32f4xx_ltdc.c
#EXT_SRCS += stm32f4xx_pwr.c
#EXT_SRCS += stm32f4xx_qspi.c
EXT_SRCS += stm32f4xx_rcc.c
#EXT_SRCS += stm32f4xx_rng.c
#EXT_SRCS += stm32f4xx_rtc.c
#EXT_SRCS += stm32f4xx_sai.c
#EXT_SRCS += stm32f4xx_sdio.c
#EXT_SRCS += stm32f4xx_spdifrx.c
EXT_SRCS += stm32f4xx_spi.c
#EXT_SRCS += stm32f4xx_syscfg.c
EXT_SRCS += stm32f4xx_tim.c
#EXT_SRCS += stm32f4xx_usart.c
#EXT_SRCS += stm32f4xx_wwdg.c

EXT_OBJ = $(EXT_SRCS:.c=.o)

INC_DIRS  = inc/
INC_DIRS += ../Libraries/STM32F4xx_StdPeriph_Driver/inc
INC_DIRS += ../Libraries/CMSIS/Include
INC_DIRS += ../Libraries/CMSIS/Device/ST/STM32F4xx/Include


INCLUDE = $(addprefix -I,$(INC_DIRS))

DEFS = -DSTM32F401xE -DUSE_STM324xG_EVAL -DSTM32F401xx -DUSE_STDPERIPH_DRIVER

CFLAGS  = -ggdb -O0
CFLAGS += -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork -Wl,--gc-sections

WFLAGS += -Wall -Wextra -Warray-bounds -Wno-unused-parameter

LFLAGS = -TDevice/gcc.ld

.PHONY: all
all: $(PROJECT_NAME)

.PHONY: $(PROJECT_NAME)
$(PROJECT_NAME): $(PROJECT_NAME).elf

$(PROJECT_NAME).elf: $(SRCS) $(EXT_OBJ)
	$(CC) $(INCLUDE) $(DEFS) $(CFLAGS) $(WFLAGS) $(LFLAGS) $^ -o $@
	$(OBJCOPY) -O ihex $(PROJECT_NAME).elf   $(PROJECT_NAME).hex
	$(OBJCOPY) -O binary $(PROJECT_NAME).elf $(PROJECT_NAME).bin

%.o: %.c
	$(CC) -c -o $@ $(INCLUDE) $(DEFS) $(CFLAGS) $^

clean:
	rm -f *.o $(PROJECT_NAME).elf $(PROJECT_NAME).hex $(PROJECT_NAME).bin
