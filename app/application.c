#include <application.h>

#include "fatfs/ff.h"
//#include "fatfs_spi.h"

#include "hxcmod.h"
//#include "data_cartoon_dreams_n_fantasies_mod.h"

// MOD data
extern const unsigned char mod_data[39424];

// sound output buffer
#define SAMPLE_BUFFER_SIZE (1024*6)
msample dmasoundbuffer[SAMPLE_BUFFER_SIZE] __attribute__ ((aligned (4)));

uint32_t buffer_playhead = 0;
bool load_first_half_flag = false;
bool load_second_half_flag = false;

bool pause = false;

// LED instance
bc_led_t led;

// Button instance
bc_button_t button;

FATFS FatFs;

//modcontext modctx;

// Button event callback
void button_event_handler(bc_button_t *self, bc_button_event_t event, void *event_param)
{
    // Log button event
    bc_log_info("APP: Button event: %i", event);

    // Check event source
    if (event == BC_BUTTON_EVENT_PRESS)
    {
        pause = !pause;
    }
}

const uint8_t sine_wave[] = {
  0x80, 0x86, 0x8C, 0x93,
  0x99, 0x9F, 0xA5, 0xAB,
  0xB1, 0xB6, 0xBC, 0xC1,
  0xC7, 0xCC, 0xD1, 0xD5,
  0xDA, 0xDE, 0xE2, 0xE6,
  0xEA, 0xED, 0xF0, 0xF3,
  0xF5, 0xF8, 0xFA, 0xFB,
  0xFD, 0xFE, 0xFE, 0xFF,
  0xFF, 0xFF, 0xFE, 0xFE,
  0xFD, 0xFB, 0xFA, 0xF8,
  0xF5, 0xF3, 0xF0, 0xED,
  0xEA, 0xE6, 0xE2, 0xDE,
  0xDA, 0xD5, 0xD1, 0xCC,
  0xC7, 0xC1, 0xBC, 0xB6,
  0xB1, 0xAB, 0xA5, 0x9F,
  0x99, 0x93, 0x8C, 0x86,
  0x80, 0x7A, 0x74, 0x6D,
  0x67, 0x61, 0x5B, 0x55,
  0x4F, 0x4A, 0x44, 0x3F,
  0x39, 0x34, 0x2F, 0x2B,
  0x26, 0x22, 0x1E, 0x1A,
  0x16, 0x13, 0x10, 0x0D,
  0x0B, 0x08, 0x06, 0x05,
  0x03, 0x02, 0x02, 0x01,
  0x01, 0x01, 0x02, 0x02,
  0x03, 0x05, 0x06, 0x08,
  0x0B, 0x0D, 0x10, 0x13,
  0x16, 0x1A, 0x1E, 0x22,
  0x26, 0x2B, 0x2F, 0x34,
  0x39, 0x3F, 0x44, 0x4A,
  0x4F, 0x55, 0x5B, 0x61,
  0x67, 0x6D, 0x74, 0x7A
};



void irq_TIM3_handler(void *param)
{
    (void) param;
    TIM3->SR = ~TIM_DIER_UIE;

    if (pause)
    {
        return;
    }
/*
    static uint8_t interrupt_counter = 0;

    interrupt_counter++;

    if (interrupt_counter != 5)
    {
        //return;
    }

    interrupt_counter = 0;

    static uint16_t sine_items = sizeof(sine_wave);
    static uint16_t sample = 0;

    TIM3->CCR2 = sine_wave[sample];

    sample += 10;

    if (sample >= sine_items)
    {
        sample = 0;
    }*/

    uint8_t sample = dmasoundbuffer[buffer_playhead];
    TIM3->CCR2 = sample;

    buffer_playhead++;


    if (buffer_playhead == SAMPLE_BUFFER_SIZE/2)
    {
        load_first_half_flag = true;
    }

    if (buffer_playhead >= SAMPLE_BUFFER_SIZE)
    {
        buffer_playhead = 0;
        load_second_half_flag = true;
    }
}


// Application initialization function which is called once after boot
void application_init(void)
{
    // Initialize logging
    bc_log_init(BC_LOG_LEVEL_DUMP, BC_LOG_TIMESTAMP_ABS);

    // Initialize LED
    bc_led_init(&led, BC_GPIO_LED, false, 0);
    bc_led_pulse(&led, 1000);

    bc_gpio_init(BC_GPIO_P9);
    bc_gpio_set_mode(BC_GPIO_P9, BC_GPIO_MODE_OUTPUT);
    bc_gpio_set_output(BC_GPIO_P9, false);

    // Initialize button
    bc_button_init(&button, BC_GPIO_BUTTON, BC_GPIO_PULL_DOWN, 0);
    bc_button_set_event_handler(&button, button_event_handler, NULL);

    bc_system_pll_enable();
/*
    uint32_t resolution_us = 4;
    uint32_t period_cycles = 255;

    // PA7 PWM TIM3_CH2
    // Enable TIM3 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Errata workaround
    RCC->APB1ENR;

    // Disable counter if it is running
    TIM3->CR1 &= ~TIM_CR1_CEN;

    // Set prescaler to 5 * 32 (5 microseconds resolution)
    TIM3->PSC = resolution_us * 1 - 1;
    TIM3->ARR = period_cycles - 1;

    // CH2 - is on PB5 - RADIO_MOSI
    TIM3->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
    TIM3->CCER |= TIM_CCER_CC2E;

    TIM3->CCR2 = 180; // z 255 //bc_pwm_set(BC_PWM_P6, 180);

    // Alternate function
    GPIOA->AFR[0] |= 2 << GPIO_AFRL_AFRL7_Pos;
    // MODER alternate
    GPIOA->MODER &= ~GPIO_MODER_MODE7_Msk;
    GPIOA->MODER |= GPIO_MODER_MODE7_1;


    // Enable TIM3 interrupts
    TIM3->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM3_IRQn);

    bc_timer_set_irq_handler(TIM3, irq_TIM3_handler, NULL);

    TIM3->CR1 |= TIM_CR1_CEN;

    int ret = 0;

    #define PLAYER_SAMPLE_RATE 16000 //32000

    ret = hxcmod_init(&modctx);
    hxcmod_setcfg( &modctx, PLAYER_SAMPLE_RATE, 0, 0);

    ret = hxcmod_load( &modctx, (void*)&mod_data, sizeof(mod_data) );


    bc_log_debug("ret %d", ret);
*/


    uint8_t ret = f_mount(&FatFs, "0:", 1);

    bc_log_debug("ret fmount: %d", ret);

    FIL fp;
    ret = f_open(&fp, "text.txt", FA_CREATE_ALWAYS | FA_WRITE);
    bc_log_debug("ret f_open: %d", ret);

    char text[] = "Ahoj svete z CoreCard!";
    uint32_t written = 0;
    ret = f_write(&fp, text, strlen(text), &written);
    bc_log_debug("ret f_write: %d", ret);

    bc_log_debug("written: %d", written);

    ret = f_close(&fp);
    bc_log_debug("ret f_close: %d", ret);

}

// Application task function (optional) which is called peridically if scheduled
void application_task(void)
{
    /*
    if(load_first_half_flag)
    {
        bc_led_set_mode(&led, BC_LED_MODE_ON);
        bc_gpio_set_output(BC_GPIO_P9, true);
        hxcmod_fillbuffer( &modctx, (msample*)&dmasoundbuffer, SAMPLE_BUFFER_SIZE/2, NULL );
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
        bc_gpio_set_output(BC_GPIO_P9, false);
        load_first_half_flag = false;
    }

    if(load_second_half_flag)
    {
        bc_led_set_mode(&led, BC_LED_MODE_ON);
        bc_gpio_set_output(BC_GPIO_P9, true);
        hxcmod_fillbuffer( &modctx, (msample*)&dmasoundbuffer[SAMPLE_BUFFER_SIZE/2], SAMPLE_BUFFER_SIZE/2, NULL );
        bc_led_set_mode(&led, BC_LED_MODE_OFF);
        bc_gpio_set_output(BC_GPIO_P9, false);
        load_second_half_flag = false;
    }

    bc_scheduler_plan_current_from_now(0);*/
}


DWORD get_fattime (void)
{
    return	  0;
}
