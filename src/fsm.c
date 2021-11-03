
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

#include "radar_communication.h"
#include "lte_communication.h"
#include "fsm.h"
#include "measure.h"

#include <zephyr.h>

#define COAP_SEND_INTERVAL_MS_DEFAULT K_SECONDS(60)
#define MEASURE_INTERVAL_MS_DEFAULT K_SECONDS(10)
#define TIMEOUT_TIME_UART K_MSEC(1300)
#define QUEUE_LENGTH 10
#define SENSOR_ID 0

//fsm
typedef enum
{
    IDLE_STATE,
    GET_SPEED_STATE,
    GET_DISTANCE_STATE,
} state_t;

// ------------------------------------------------------- static variables ---

const uint8_t acces_token[] = "ANKKtru6meb6lMSxqntw";

//Timer
static struct k_timer measure_timer;
static struct k_timer send_timer;
static struct k_timer startup_timer;
static struct k_timer timeout_timer;

static bool startup_flag = 1;
static bool send_timer_flag = 0;

static event_t fsm_event = NO_EVENT;
static state_t fsm_state = IDLE_STATE;
static state_t next_fsm_state = IDLE_STATE;
static event_t event_queue[QUEUE_LENGTH] = {NO_EVENT};
static uint16_t queue_head = 0, queue_tail = 0;

static uint8_t timeoutcounter = 0;

// -------------------------------------------- private function prototypes ---

static void idle_state_event_handler(void);
static void get_distance_state_handler(void);
static void get_speed_state_handler(void);
static void timer_init(void);
static event_t get_next_event(void);

// ------------------------------------------------------- public functions ---

void fsm_init(void)
{
    timer_init();
}

int8_t new_event(event_t event)
{
    static bool queue_full = false;

    if ((queue_tail == queue_head) & queue_full)
    {
        return -1;
    }
    else
    {
        queue_full = false;
    }

    event_queue[queue_tail] = event;
    queue_tail = (queue_tail + 1) % QUEUE_LENGTH;

    if (queue_tail == queue_head)
    {
        queue_full = true;
    }
    printk("event: %d\n", event);
    return 0;
}

void stateMachine(void)
{
    uint8_t result_buffer[1000];

    fsm_state = next_fsm_state;
    fsm_event = get_next_event();
    

    switch (fsm_state)
    {
    case IDLE_STATE:
        if (fsm_event != NO_EVENT)
        {
            printk("event_Idle_state: %d, send_timer_flag: %u\n", fsm_event, send_timer_flag);
        }
        if (send_timer_flag)
        {
            get_results(result_buffer);
            send_data(result_buffer, acces_token);
            send_timer_flag = 0;
        }
        idle_state_event_handler();
        break;

    case GET_SPEED_STATE:
        printk("event_Speed_state: %d\n", fsm_event);
        rec_speed();
        get_speed_state_handler();
        break;

    case GET_DISTANCE_STATE:
        printk("event_Distance_state: %d\n", fsm_event);
        rec_distance();
        get_distance_state_handler();
        break;

    default:
        break;
    }
}

// ------------------------------------------------------ private functions ---

static event_t get_next_event(void)
{
    event_t next_event;
    if (event_queue[queue_head] != NO_EVENT)
    {
        next_event = event_queue[queue_head];
        event_queue[queue_head] = NO_EVENT;
        queue_head = (queue_head + 1) % QUEUE_LENGTH;
    }
    else
    {
        next_event = NO_EVENT;
    }
    return next_event;
}

static void idle_state_event_handler(void)
{
    uint64_t unix_time;
    switch (fsm_event)
    {
    case MEASURE_TIMER_EVENT:
        printk("measuretimert_event\n");
        new_datapoint();
        printk("new_datapoint\n");
        unix_time = get_network_time();
        new_time(unix_time);
        printk("get_network_time\n");
        start_distance_measurement();
        printk("start_distance_measurement\n");
        k_timeout_t timeout;
        timeout.ticks = 0;
        k_timer_start(&timeout_timer, TIMEOUT_TIME_UART, timeout);
        printk("k_timer_start\n");
        timeoutcounter = 0;
        next_fsm_state = GET_DISTANCE_STATE;
        printk("end\n");
        break;

    default:
        break;
    }
}

static void get_distance_state_handler(void)
{

    switch (fsm_event)
    {
    case DISTANCE_REC_EVENT:
        k_timer_stop(&timeout_timer);
        if (get_last_distance())
        {
            start_speed_measurement(get_last_distance());
            k_timeout_t timeout;
            timeout.ticks = 0;
            k_timer_start(&timeout_timer, TIMEOUT_TIME_UART, timeout);
            timeoutcounter = 0;
            next_fsm_state = GET_SPEED_STATE;
        }
        else
        {
            new_speed(0);
            next_fsm_state = IDLE_STATE;
        }

        printk("distance recived event\n");

        break;
    case TIMEOUT_EVENT:
        printk("timeout_event_distance\n");
        if (timeoutcounter == 1)
        {
            new_speed(1);
            new_distance(1);
            k_timer_stop(&timeout_timer);
            timeoutcounter = 0;
            next_fsm_state = IDLE_STATE;
        }
        else
        {
            start_distance_measurement();
            k_timer_stop(&timeout_timer);
            k_timeout_t timeout;
            timeout.ticks = 0;
            k_timer_start(&timeout_timer, TIMEOUT_TIME_UART, timeout);
            next_fsm_state = GET_DISTANCE_STATE;
            timeoutcounter++;
        }

        break;

    default:
        if (k_timer_status_get(&timeout_timer) > 0)
        {
            /* timer has expired */
            //new_event(TIMEOUT_EVENT);
        }
        else if (k_timer_remaining_get(&timeout_timer) == 0)
        {
            /* timer was stopped (by someone else) before expiring */
            // new_event(TIMEOUT_EVENT);
        }
        else
        {
            /* timer is still running */
        }
        break;
    }
}

static void get_speed_state_handler(void)
{

    switch (fsm_event)
    {
    case SPEED_REC_EVENT:
        printk("speed_rec_event\n");
        k_timer_stop(&timeout_timer);
        next_fsm_state = IDLE_STATE;
        break;
    case TIMEOUT_EVENT:
        printk("timeout_event_speed\n");
        if (timeoutcounter == 1)
        {
            k_timer_stop(&timeout_timer);
            new_speed(1);
            timeoutcounter = 0;
            next_fsm_state = IDLE_STATE;
        }
        else
        {
            start_speed_measurement(get_last_distance());
            k_timer_stop(&timeout_timer);
            k_timeout_t timeout;
            timeout.ticks = 0;
            k_timer_start(&timeout_timer, TIMEOUT_TIME_UART, timeout);
            next_fsm_state = GET_SPEED_STATE;
            timeoutcounter++;
        }
        break;
    default:
        if (k_timer_status_get(&timeout_timer) > 0)
        {
            /* timer has expired */
            //new_event(TIMEOUT_EVENT);
        }
        else if (k_timer_remaining_get(&timeout_timer) == 0)
        {
            /* timer was stopped (by someone else) before expiring */
            // new_event(TIMEOUT_EVENT);
        }
        else
        {
            /* timer is still running */
        }
        break;
    }
}

static void timeout_timer_handler(struct k_timer *dummy)
{
    new_event(TIMEOUT_EVENT);
    printk("timeout_event\n");
}

static void measure_timer_handler(struct k_timer *dummy)
{
    new_event(MEASURE_TIMER_EVENT);
}

static void send_timer_handler(struct k_timer *dummy)
{
    send_timer_flag = 1;
}

static void startup_timer_handler(struct k_timer *dummy)
{
    //k_timer_stop(&send_timer);
    //k_timer_stop(&measure_timer);
    k_timer_stop(&startup_timer);
    k_timer_start(&send_timer, COAP_SEND_INTERVAL_MS_DEFAULT, COAP_SEND_INTERVAL_MS_DEFAULT);
    k_timer_start(&measure_timer, MEASURE_INTERVAL_MS_DEFAULT, MEASURE_INTERVAL_MS_DEFAULT);

    startup_flag = 0;
}

/*sets the timers for measurement and send time*/
static void timer_init(void)
{
    k_timer_init(&measure_timer, measure_timer_handler, NULL);
    //k_timer_start(&measure_timer, K_SECONDS(1), K_SECONDS(1));

    k_timer_init(&send_timer, send_timer_handler, NULL);
    //k_timer_start(&send_timer, K_SECONDS(1), K_SECONDS(1));

    k_timer_init(&startup_timer, startup_timer_handler, NULL);
    k_timeout_t timeout;
    timeout.ticks = 0;
    k_timer_start(&startup_timer, K_SECONDS(1), timeout);

    k_timer_init(&timeout_timer, timeout_timer_handler, NULL);
}
