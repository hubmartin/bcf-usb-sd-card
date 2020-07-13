<a href="https://www.hardwario.com/"><img src="https://www.hardwario.com/ci/assets/hw-logo.svg" width="200" alt="HARDWARIO Logo" align="right"></a>

# Core Module FatFs microSD card & MOD audio player example

## FatFs

This code is using FatFs library to access microSD card over SPI. This is initial rough prototype.

This code could use CoreCard project which is a PCB with microSD and audio jack board. See also the [CoreCard repository](https://github.com/hubmartin/CoreCard).

FatFs example shows writing to the card. You can configure which SPI to use by `#define` in `stm32l0.c` file:

- `USE_PINHEADER_SPI` this is using SPI and CS signals on the Core Module pin headers which are shared with other SPI modules like LCD Module or Ethernet module. This way you couldn't use LCD or other modules.
- `USE_RADIO_SPI` use this define when the `CoreCard` PCB is soldered instead of SPIRIT1 radio module. This way the SPI is dedicated to microSD card only and can take advantage of maximal data bandwith and tehre is no collision with the second SPI port on pin headers.

## Audio out

Audio is using PWM with `TIM3` connected to GPIO0 pin on place where SPIRIT1 radio is usually soldered, this code needs CoreCard PCB (see other repo) which is using RC filter and audio jack to create analog output.

I hope that soon this will be also possible to configure and you could use other PWM output of TIM3 or DAC. Right now it is fixed to TIM3_CH2 GPIO0 on SPIRIT1 radio footprint.

The audio example is using https://github.com/jfdelnero/HxCModPlayer MOD player which plays the MOD file which is embedded as an array in the C firmware.

The RAM is really almost full using the HxCModPlayer decoder. It can help if you lower the maximum number of notes or other features of the library.
The audio player also almost fully utilizes the MCU even when the PLL is turned on. This can be lowered when you set lower `PLAYER_SAMPLE_RATE`. When you half the bitrate, you also need to increment multiplier in the `TIM3->PSC` prescaller so the timer runs at half the speed.

```
TIM3->PSC = resolution_us * 1 - 1;
#define PLAYER_SAMPLE_RATE 16000
```

```
TIM3->PSC = resolution_us * 2 - 1;
#define PLAYER_SAMPLE_RATE 8000
```

At 8kHz the MCU is utilized around 50%.


## Images

![](images/IMG_20200713_211840.jpg)

![](images/IMG_20200713_211856.jpg)

## License

This project is licensed under the [MIT License](https://opensource.org/licenses/MIT/) - see the [LICENSE](LICENSE) file for details.

---

Made with &#x2764;&nbsp; by [**HARDWARIO s.r.o.**](https://www.hardwario.com/) in the heart of Europe.
