# tcd1304_firmware_spi
STM32F401RE firmware to drive and read the linear CCD TCD1304DG and TCD1304AP

Specifications:
  fM-frequency: 2.0 MHz [1]
  Integration time: 10 µs - 35 min [2]
  Communication through SPI [3]
  Frame-rate: up to 85 Hz [4]


Description:
  The firmware uses timers for all the driving pulses and triggering of the STM32F401RE's built-in ADC.
  Data transfers are handled using the MCU's two DMA-controllers. DMA2 takes care of the data coming from
  the ADC. DMA1 feeds the SPI-controller. Two different interrupts starts and stops the ADC-trigger.
  
  To compile, the Standard Peripherals Library from ST-Micro must be installed, and the HSE-value in 
    ../Libraries/CMSIS/Device/ST/STM32F4xx/Include/stm32f4xx.h
  set to the correct value, which will depend on the external clock source. On nucleo boards it's usually
  8.000 MHz (the x-tal on the ST-link).


[1] The fM-frequency is user configurable at compile time. With a systemcoreclock of 84 MHz, 
    and no prescaling of the timers the following frequencies are available simply by altering main.h:
        4.0 MHz, 3.5 MHz, 3.36 MHz, 3.0 MHz, 2.8 MHz, 2.625 MHz, 2.4 MHz, 2.1 MHz, 2.0 MHz, 
        1.75 MHz, 1.68 MHz, 1.50 MHz, 1.40 MHz, 1.3125 MHz, 1.2 MHz, 1.12 MHz, 1.05 MHz, 1.0 MHz,
        875 kHz, 840 kHz and 800 kHz
    Only the values 1.0 MHz and 1.4 MHz - 2.0 MHz are tested, however the ADC should be fast enough
    for even 4.0 MHz - though the sampling time should be reduced.
    
[2] The firmware uses a 32-bit timer to control integration time. The timer's clock is prescaled to
    run with the same frequency of the fM clock. This means the maximum integration time is dependent
    on the CCD's master clock, eg for a firmware with fM = 2.0 MHz:
      t_int(max) = (2^32 - 1) / fM = (2^32 - 1) / 2.0 MHz = 2147 s = 35 min
      
[3] The firmware is set up to use one of the STM32F401RE's SPI controllers in slave mode. With a Raspberry pi
    communication can be as fast as 16 Mbps using the rpi's SPI peripheral.
    
[4] The frame-rate is dependent on the integration time, the read-out time and the transmission time.
    The read-out time is: 
        t_read = 4*3696 / fM = 14776 / 2.0 MHz = 7.4 ms
    The transmission time is (minimum):
        t_tx = 16 bit*3694 / 16 Mbps = 3.7 ms
    With very short integration times (in the 10-100µs range) the max frame-rate is:
        frame-rate = 1 / (7.4 ms + 3.7 ms + 100 µs) = 89 Hz
    In reality the SPI transfer probably takes something like 4.2 ms.

goto to https://tcd1304.wordpress.com for details of how to compile
