/**
 *           ______   _    ___        __          ___ ____   ____ 
 *          |__  / | | |  / \ \      / /         |_ _/ ___| / ___|
 *            / /| |_| | / _ \ \ /\ / /   _____   | |\___ \| |    
 *           / /_|  _  |/ ___ \ V  V /   |_____|  | | ___) | |___ 
 *          /____|_| |_/_/   \_\_/\_/            |___|____/ \____|
 * 
 *                Zurich University of Applied Sciences
 *         Institute of Signal Processing and Wireless Communications
 * 
 * ----------------------------------------------------------------------------
 * 
 * \brief	Handles the UART communication to the Acconeer Radar Boards
 * 
 * \author	Matthias Ludwig (ludw)
 * \version	1.0
 */

#include "radar_communication.h"
#include "measure.h"
#include "fsm.h"

#include <zephyr.h>
#include <string.h>
#include <stdlib.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>

// ------------------------------------------------------- static variables ---

//UART
static const struct device *uart1;
static const struct device *uart3;
static uint8_t uart_buf[6];
static uint16_t rx_distance;
static uint16_t rx_speed;
static bool data_received = 0;

//GPIO
static const struct device *gpio_dev;

// -------------------------------------------- private function prototypes ---

static void send_to_speed_measurement(uint32_t speed);
static void send_to_distance_measurement(uint32_t distance);
static void send_uart1(uint8_t *str);
static void send_uart3(uint8_t *str);
static void uart_cb(const struct device *dev, void *user_data);
static void uart_rx_handler(uint8_t buffer);

// ------------------------------------------------------- public functions ---

void uart_init(void)
{
    uart1 = device_get_binding("UART_1");
    uart3 = device_get_binding("UART_3");

    struct uart_config uart_settings;
    uart_settings.baudrate = 9600;
    uart_settings.data_bits = UART_CFG_DATA_BITS_8;
    uart_settings.stop_bits = UART_CFG_STOP_BITS_1;
    uart_settings.parity = UART_CFG_PARITY_NONE;
    uart_settings.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;
    uart_configure(uart1, &uart_settings);
    uart_irq_callback_user_data_set(uart1, uart_cb, NULL);
    //nrf_uarte_enable(uart1);  // TODO check if its usefull
    uart_irq_rx_enable(uart1);

    uart_configure(uart3, &uart_settings);
    uart_irq_callback_user_data_set(uart3, uart_cb, NULL);
    //nrf_uarte_enable(uart3);  // TODO check if its usefull
    uart_irq_rx_enable(uart3);
}

void gpio_init(void)
{
    gpio_dev = device_get_binding("GPIO_0");

    gpio_pin_configure(gpio_dev, 31, GPIO_OUTPUT);
    gpio_pin_configure(gpio_dev, 30, GPIO_OUTPUT);
}

void start_distance_measurement(void)
{
    // wake up radar board
    gpio_pin_set(gpio_dev, 31, true);
    k_sleep(Z_TIMEOUT_MS(2));
    gpio_pin_set(gpio_dev, 31, false);

    k_sleep(Z_TIMEOUT_MS(350));  // TODO Optimize // wait until Radar is powered up
    //send_to_distance_measurement(9999); // DEBUG
    data_received = 1; // DEBUG
}

void start_speed_measurement(uint32_t distance)
{
    // wake up radar board
    gpio_pin_set(gpio_dev, 30, true);
    k_sleep(Z_TIMEOUT_MS(2));
    gpio_pin_set(gpio_dev, 30, false);

    k_sleep(Z_TIMEOUT_MS(350));  // TODO Optimize // wait until Radar is powered up
    //send_to_speed_measurement(distance); // DEBUG
    data_received = 1; // DEBUG
}

void rec_distance(void)
{
    if (data_received)
    {
        rx_distance = 200; // DEBUG
        new_distance(rx_distance);
        printk("rec_distance: %d\n", rx_distance);
        rx_distance = 0;
        data_received = false;
        new_event(DISTANCE_REC_EVENT);
    }
}

void rec_speed(void)
{
    if (data_received)
    {
        rx_speed = 10; // DEBUG
        new_speed(rx_speed);
        rx_speed = 0;
        data_received = 0;
        new_event(SPEED_REC_EVENT);
    }
}

// ------------------------------------------------------ private functions ---

/*send string via uart1*/
static void send_uart1(uint8_t *str)
{
    uint32_t len = strlen(str);
    while (len--)
    {
        uart_poll_out(uart1, *str++);
    }

    k_sleep(Z_TIMEOUT_MS(5));
}

/*send string via uart3*/
static void send_uart3(uint8_t *str)
{
    uint32_t len = strlen(str);
    while (len--)
    {
        uart_poll_out(uart3, *str++);
    }

    k_sleep(Z_TIMEOUT_MS(5));
}

static void send_to_distance_measurement(uint32_t distance)
{
    uint8_t tx_buffer[8];

    sprintf(tx_buffer, "D%df", distance);
    send_uart1(tx_buffer);
}

static void send_to_speed_measurement(uint32_t speed)
{
    uint8_t tx_buffer[8];

    sprintf(tx_buffer, "S%df", speed);
    send_uart3(tx_buffer);
}

/*callback function from uart interrupt read uart string*/
static void uart_cb(const struct device *dev, void *user_data)
{
    uart_irq_update(dev);
    if (uart_irq_rx_ready(dev))
    {
        while (uart_fifo_read(dev, uart_buf, sizeof(uart_buf)))
        {
            uart_rx_handler(uart_buf[0]);
        }
    }
}

static void uart_rx_handler(uint8_t buffer)
{
    static uint8_t i = 0;
    static uint8_t rx_buffer[10];
    static bool distance_flag = 0;
    static bool speed_flag = 0;

    switch (buffer)
    {
    case 'S':
        speed_flag = 1;
        i = 0;
        break;
    case 'D':
        distance_flag = 1;
        i = 0;
        break;
    case 'f':
        rx_buffer[i] = '\0';
        if (distance_flag)
        {
            rx_distance = atoi(rx_buffer);
            distance_flag = 0;
        }
        else if (speed_flag)
        {
            rx_speed = atoi(rx_buffer);
            speed_flag = 0;
        }
        data_received = 1;
        break;
    default:
        rx_buffer[i] = buffer;
        i++;
    }
}