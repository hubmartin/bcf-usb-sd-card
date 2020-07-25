#include <application.h>

#include "fatfs/ff.h"
#include "hxcmod.h"


//#define MOD_PLAYER
#define FAT_PLAYER

FIL fp;

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

#ifdef MOD_PLAYER
modcontext modctx;

// MOD data
extern const unsigned char mod_data[39424];
#endif

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


void irq_TIM3_handler(void *param)
{
    (void) param;
    TIM3->SR = ~TIM_DIER_UIE;

    if (pause)
    {
        return;
    }

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

uint8_t f_read_multiple(FIL* fp, uint8_t* buff, UINT btr,UINT* br)
{
    uint8_t ret = 0;
    uint32_t bytes_read;
    uint32_t bytes_read_total = 0;

    while(btr)
    {
        uint32_t len = (btr > 512) ? 512 : btr;
        ret = f_read(fp, (void*)buff, len, (UINT*)&bytes_read);
        if (ret != FR_OK)
        {
            return ret;
        }
        btr -= len;
        bytes_read_total += bytes_read;
        buff += len;
    }

    *br = bytes_read_total;
    return ret;
}

void audio_task(void *param)
{
    uint32_t bytes_read;
    uint8_t ret;

    if (load_first_half_flag || load_second_half_flag)
    {
        //bc_log_debug(".");
    }


    if(load_first_half_flag)
    {
        bc_led_set_mode(&led, BC_LED_MODE_ON);
        bc_gpio_set_output(BC_GPIO_P9, true);

        #ifdef MOD_PLAYER
        hxcmod_fillbuffer( &modctx, (msample*)&dmasoundbuffer, SAMPLE_BUFFER_SIZE/2, NULL );
        #endif

        #ifdef FAT_PLAYER
        ret = f_read_multiple(&fp, (msample*)&dmasoundbuffer, SAMPLE_BUFFER_SIZE/2, (UINT*)&bytes_read);
        bc_log_debug("ret f_read: %d", ret);
        #endif

        bc_led_set_mode(&led, BC_LED_MODE_OFF);
        bc_gpio_set_output(BC_GPIO_P9, false);
        load_first_half_flag = false;
    }

    if(load_second_half_flag)
    {
        bc_led_set_mode(&led, BC_LED_MODE_ON);
        bc_gpio_set_output(BC_GPIO_P9, true);

        #ifdef MOD_PLAYER
        hxcmod_fillbuffer( &modctx, (msample*)&dmasoundbuffer[SAMPLE_BUFFER_SIZE/2], SAMPLE_BUFFER_SIZE/2, NULL );
        #endif

        #ifdef FAT_PLAYER
        ret = f_read_multiple(&fp, (msample*)&dmasoundbuffer[SAMPLE_BUFFER_SIZE/2], SAMPLE_BUFFER_SIZE/2, (UINT*)&bytes_read);
        bc_log_debug("ret f_read: %d", ret);
        #endif

        bc_led_set_mode(&led, BC_LED_MODE_OFF);
        bc_gpio_set_output(BC_GPIO_P9, false);
        load_second_half_flag = false;
    }



    bc_scheduler_plan_current_from_now(0);
}

void audio_pwm_init()
{
    uint32_t resolution_us = 4;
    uint32_t period_cycles = 255;

    // PA7 PWM TIM3_CH2, GPIO0 on SPIRIT1 header
    // Enable TIM3 clock
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    // Errata workaround
    RCC->APB1ENR;

    // Disable counter if it is running
    TIM3->CR1 &= ~TIM_CR1_CEN;

    // Set prescaler to 5 * 32 (5 microseconds resolution)
    TIM3->PSC = resolution_us * 2 - 1;
    TIM3->ARR = period_cycles - 1;

    // Timer3 CH2
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
}

void audio_example()
{

    audio_pwm_init();

    uint8_t ret = 0;

    #define PLAYER_SAMPLE_RATE 16000 //32000

    #ifdef MOD_PLAYER
    ret = hxcmod_init(&modctx);
    hxcmod_setcfg( &modctx, PLAYER_SAMPLE_RATE, 0, 0);
    ret = hxcmod_load( &modctx, (void*)&mod_data, sizeof(mod_data) );
    bc_log_debug("ret %d", ret);
    #endif

    #ifdef FAT_PLAYER
    ret = f_mount(&FatFs, "0:", 1);
    bc_log_debug("ret fmount: %d", ret);

    ret = f_open(&fp, "hwdev.raw", FA_READ);
    bc_log_debug("ret f_open: %d", ret);

/*
    uint32_t bytes_read = 0;
    //SAMPLE_BUFFER_SIZE/2 = 3072
    for (int i = 0; i < 6; i++)
    {
        ret = f_read_multiple(&fp, (msample*)&dmasoundbuffer, SAMPLE_BUFFER_SIZE/2, (UINT*)&bytes_read);
        bc_log_debug("ret f_read: %d, bytes_read %d", ret, bytes_read);
    }*/

    #endif

    bc_scheduler_register(audio_task, NULL, 0);
}

void fat_example()
{
    uint8_t ret = f_mount(&FatFs, "0:", 1);

    bc_log_debug("ret fmount: %d", ret);

    /*
    FIL fp;

    ret = f_open(&fp, "text.txt", FA_CREATE_ALWAYS | FA_WRITE);
    bc_log_debug("ret f_open: %d", ret);

    char text[] = "Ahoj svete z CoreCard!";
    uint32_t written = 0;
    ret = f_write(&fp, text, strlen(text), (UINT*)&written);
    bc_log_debug("ret f_write: %d", ret);

    bc_log_debug("written: %d", written);

    ret = f_close(&fp);
    bc_log_debug("ret f_close: %d", ret);*/
/*
    FIL fp;
    ret = f_open(&fp, "hwdev.raw", FA_READ);
    bc_log_debug("ret f_open: %d", ret);

    static char rx_buffer[600];
    uint32_t bytes_read = 0;
    ret = f_read(&fp, (msample*)&dmasoundbuffer, SAMPLE_BUFFER_SIZE/2, (UINT*)&bytes_read);
    bc_log_debug("ret f_read: %d", ret);

    ret = f_close(&fp);
    bc_log_debug("ret f_close: %d", ret);

    (void) rx_buffer[0];*/
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

    audio_example();

    //fat_example();
}

// Application task function (optional) which is called peridically if scheduled
void application_task(void)
{
    /*
    */
}


DWORD get_fattime (void)
{
    return	  0;
}
