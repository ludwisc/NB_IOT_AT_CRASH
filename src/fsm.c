
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
 * \brief	State machine to handle messurement and sending
 * 
 * \author	Matthias Ludwig (acronym)
 * \version	1.0
 */

#include "lte_communication.h"
#include "fsm.h"

#include <zephyr.h>
#include <string.h>

#define COAP_SEND_INTERVAL_MS_DEFAULT K_SECONDS(10)

// ------------------------------------------------------- static variables ---

const uint8_t acces_token[] = "ANKKtru6meb6lMSxqntw";

//Timer
static struct k_timer send_timer;
static uint32_t i = 0;

static bool send_timer_flag = 0;

// -------------------------------------------- private function prototypes ---

static void timer_init(void);

// ------------------------------------------------------- public functions ---

void fsm_init(void)
{
    timer_init();
}

void stateMachine(void)
{
    uint8_t result_buffer[100];
    uint8_t sig_pwr[21];
    result_buffer[0] = '\0';

    if (send_timer_flag)
    {
        printk("%u\n",i);
        i++;

        get_signal_power(sig_pwr);
        strcat(result_buffer, "{'RSRP': ");
        strcat(result_buffer, sig_pwr);
        strcat(result_buffer, "}");

        send_data(result_buffer, acces_token);
        send_timer_flag = 0;
    }
}

// ------------------------------------------------------ private functions ---

static void send_timer_handler(struct k_timer *dummy)
{
    send_timer_flag = 1;
}

/*sets the timers for measurement and send time*/
static void timer_init(void)
{
    k_timer_init(&send_timer, send_timer_handler, NULL);
    k_timer_start(&send_timer, COAP_SEND_INTERVAL_MS_DEFAULT, COAP_SEND_INTERVAL_MS_DEFAULT);
}
