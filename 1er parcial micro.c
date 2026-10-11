//***********************************************************//
/*           Microcontroladores                              */
/*  Nombre:    Henyel Mota                                   */
/*  Matricula:  2020-10735                                   */
/*  Seccion:   Lunes                                         */
/*  Practica:  GPIO                                          */
/*  Fecha:     21/9/2026                                     */
//***********************************************************//

#include <stdint.h>

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"


#define SW2_GPIO       GPIO1
#define SW2_PIN        7U


#define SPEED_GPIO     GPIO0
#define SPEED_PIN      23U


volatile uint32_t milliseconds = 0U;


void SysTick_Handler(void)
{
    milliseconds++;
}


static void turn_off_all(void)
{
    GPIO_PortSet(
        BOARD_LED_RED_GPIO,
        1U << BOARD_LED_RED_GPIO_PIN
    );

    GPIO_PortSet(
        BOARD_LED_GREEN_GPIO,
        1U << BOARD_LED_GREEN_GPIO_PIN
    );

    GPIO_PortSet(
        BOARD_LED_BLUE_GPIO,
        1U << BOARD_LED_BLUE_GPIO_PIN
    );
}


static void turn_on_color(uint8_t color)
{
    turn_off_all();

    switch (color)
    {
        case 0: /* Rojo */
            GPIO_PortClear(
                BOARD_LED_RED_GPIO,
                1U << BOARD_LED_RED_GPIO_PIN
            );
            break;

        case 1: /* Verde */
            GPIO_PortClear(
                BOARD_LED_GREEN_GPIO,
                1U << BOARD_LED_GREEN_GPIO_PIN
            );
            break;

        case 2: /* Azul */
            GPIO_PortClear(
                BOARD_LED_BLUE_GPIO,
                1U << BOARD_LED_BLUE_GPIO_PIN
            );
            break;

        case 3: /* Amarillo */
            GPIO_PortClear(
                BOARD_LED_RED_GPIO,
                1U << BOARD_LED_RED_GPIO_PIN
            );

            GPIO_PortClear(
                BOARD_LED_GREEN_GPIO,
                1U << BOARD_LED_GREEN_GPIO_PIN
            );
            break;

        case 4: /* Cian */
            GPIO_PortClear(
                BOARD_LED_GREEN_GPIO,
                1U << BOARD_LED_GREEN_GPIO_PIN
            );

            GPIO_PortClear(
                BOARD_LED_BLUE_GPIO,
                1U << BOARD_LED_BLUE_GPIO_PIN
            );
            break;

        case 5: /* Magenta */
            GPIO_PortClear(
                BOARD_LED_RED_GPIO,
                1U << BOARD_LED_RED_GPIO_PIN
            );

            GPIO_PortClear(
                BOARD_LED_BLUE_GPIO,
                1U << BOARD_LED_BLUE_GPIO_PIN
            );
            break;

        case 6: /* Blanco */
            GPIO_PortClear(
                BOARD_LED_RED_GPIO,
                1U << BOARD_LED_RED_GPIO_PIN
            );

            GPIO_PortClear(
                BOARD_LED_GREEN_GPIO,
                1U << BOARD_LED_GREEN_GPIO_PIN
            );

            GPIO_PortClear(
                BOARD_LED_BLUE_GPIO,
                1U << BOARD_LED_BLUE_GPIO_PIN
            );
            break;

        default:
            break;
    }
}

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();

    CLOCK_EnableClock(kCLOCK_GateGPIO0);
    CLOCK_EnableClock(kCLOCK_GateGPIO1);
    CLOCK_EnableClock(kCLOCK_GateGPIO3);


    gpio_pin_config_t led_config =
    {
        kGPIO_DigitalOutput,
        1U
    };

    gpio_pin_config_t button_config =
    {
        kGPIO_DigitalInput,
        0U
    };


    GPIO_PinInit(
        BOARD_LED_RED_GPIO,
        BOARD_LED_RED_GPIO_PIN,
        &led_config
    );

    GPIO_PinInit(
        BOARD_LED_GREEN_GPIO,
        BOARD_LED_GREEN_GPIO_PIN,
        &led_config
    );

    GPIO_PinInit(
        BOARD_LED_BLUE_GPIO,
        BOARD_LED_BLUE_GPIO_PIN,
        &led_config
    );


    GPIO_PinInit(SW2_GPIO, SW2_PIN, &button_config);
    GPIO_PinInit(SPEED_GPIO, SPEED_PIN, &button_config);


    SysTick_Config(SystemCoreClock / 1000U);

    uint8_t color = 0U;
    uint8_t speed = 0U;
    uint8_t led_on = 1U;

    uint32_t last_blink = 0U;
    uint32_t last_sw2_press = 0U;
    uint32_t last_speed_press = 0U;

    uint32_t last_sw2_state = 1U;
    uint32_t last_speed_state = 1U;

    /*
     * Velocidades:
     * 1000 ms = lenta
     * 500 ms  = media
     * 200 ms  = rápida
     */
    const uint32_t blink_times[3] =
    {
        1000U,
        500U,
        200U
    };

    turn_on_color(color);

    while (1)
    {
        uint32_t current_time = milliseconds;

        uint32_t sw2_state =
            GPIO_PinRead(SW2_GPIO, SW2_PIN);

        uint32_t speed_button_state =
            GPIO_PinRead(SPEED_GPIO, SPEED_PIN);


        if ((last_sw2_state == 1U) &&
            (sw2_state == 0U) &&
            ((current_time - last_sw2_press) > 50U))
        {
            color++;

            if (color >= 7U)
            {
                color = 0U;
            }

            led_on = 1U;
            turn_on_color(color);

            last_blink = current_time;
            last_sw2_press = current_time;
        }


        if ((last_speed_state == 1U) &&
            (speed_button_state == 0U) &&
            ((current_time - last_speed_press) > 50U))
        {
            speed++;

            if (speed >= 3U)
            {
                speed = 0U;
            }

            last_speed_press = current_time;
        }


        if ((current_time - last_blink) >= blink_times[speed])
        {
            last_blink = current_time;

            if (led_on == 1U)
            {
                turn_off_all();
                led_on = 0U;
            }
            else
            {
                turn_on_color(color);
                led_on = 1U;
            }
        }

        last_sw2_state = sw2_state;
        last_speed_state = speed_button_state;
    }
}
