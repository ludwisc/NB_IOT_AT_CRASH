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
 * \brief	Handles the LTE communication with the CoAP protocol
 * 
 * \author	Matthias Ludwig (ludw)
 * \version	1.0
 */
#include <stdio.h>
#include <net/coap.h>
#include <net/socket.h>
#include <modem/lte_lc.h>
#include <modem/at_cmd.h>
#include <random/rand32.h>
#include <date_time.h>
#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif

#include "lte_communication.h"
#include "measure.h"

#define APP_COAP_SEND_INTERVAL_MS 10000
#define APP_COAP_MAX_MSG_LEN 1280
#define APP_COAP_VERSION 1

// ------------------------------------------------------- static variables ---

static int sock;
static struct pollfd fds;
static struct sockaddr_storage server;
static uint16_t next_token;

static uint8_t coap_buf[APP_COAP_MAX_MSG_LEN];

static const char get_signal_power_command[] = "AT+CESQ";

// -------------------------------------------- private function prototypes ---

static int server_resolve(void);
static int client_init(void);
static int client_handle_get_response(uint8_t *buf, int received);
static int client_get_send(void);
static int client_post_send(uint8_t *payload, const uint8_t *acces_token_str);
static void modem_configure(void);
static int wait(int timeout);
// ------------------------------------------------------- public functions ---

int lte_init()
{
    int err;

    modem_configure();

    err = server_resolve();
    if (err != 0)
    {
        printk("Failed to resolve server name\n");
        return err;
    }

    err = client_init();
    if (err != 0)
    {
        printk("Failed to initialize CoAP client\n");
        return err;
    }

    err = lte_lc_psm_req(true);
    if (err != 0)
    {
        printk("Failed to enable power save mode\n");
        return err;
    }

    err = lte_lc_edrx_req(true);
    if (err != 0)
    {
        printk("Failed to enable eDRX mode\n");
        return err;
    }
    date_time_update_async(NULL);
    return 0;
}

void lte_close(void)
{
    (void)close(sock);
}

int lte_wait(const uint32_t timeout)
{
    return wait(timeout);
}

int send_data(uint8_t *payload, const uint8_t *acces_token_str)
{
    int err;
    err = client_post_send(payload, acces_token_str);

    return err;
}

uint64_t get_network_time(void)
{
    int64_t unix_time = 0;
    int err = 0;
    err = date_time_now(&unix_time);
    if (err != 0)
    {
        printk("not date/time");
        unix_time = 0;
    }
    return (uint64_t)unix_time;
}

void get_signal_power(uint8_t *buffer)
{
    enum at_cmd_state state;
    char temp[50];
    int16_t signalPower = 0;
    printk("Before AT\n");
    at_cmd_write(get_signal_power_command, temp, sizeof(temp), &state);
    printk("After AT\n");
    
    //if (state == AT_CMD_OK)
    //{
    //    for (uint8_t u = 0; u < 3; u++)
    //    {
    //        buffer[u] = temp[u + 25];
    //    }
    //}
    //
    //signalPower = atoi(buffer);
    signalPower = 141;
    sprintf(buffer, "%d", signalPower);
    printk("Signal-power: %sdBm\n", buffer);
}

#if defined(CONFIG_NRF_MODEM_LIB)

/**
 * @brief Recoverable modem library error.
*/
void nrf_modem_recoverable_error_handler(uint32_t err)
{
    printk("Modem library recoverable error: %u\n", (unsigned int)err);
}

#endif /* defined(CONFIG_NRF_MODEM_LIB) */

// ------------------------------------------------------ private functions ---

/**
 * @brief Resolves the configured hostname.
 */
static int server_resolve(void)
{
    int err;
    struct addrinfo *result;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM};
    char ipv4_addr[NET_IPV4_ADDR_LEN];

    err = getaddrinfo(CONFIG_COAP_SERVER_HOSTNAME, NULL, &hints, &result);
    if (err != 0)
    {
        printk("ERROR: getaddrinfo failed %d\n", err);
        return -EIO;
    }

    if (result == NULL)
    {
        printk("ERROR: Address not found\n");
        return -ENOENT;
    }

    /* IPv4 Address. */
    struct sockaddr_in *server4 = ((struct sockaddr_in *)&server);

    server4->sin_addr.s_addr =
        ((struct sockaddr_in *)result->ai_addr)->sin_addr.s_addr;
    server4->sin_family = AF_INET;
    server4->sin_port = htons(CONFIG_COAP_SERVER_PORT);

    inet_ntop(AF_INET, &server4->sin_addr.s_addr, ipv4_addr,
              sizeof(ipv4_addr));
    printk("IPv4 Address found %s\n", ipv4_addr);

    /* Free the address. */
    freeaddrinfo(result);

    return 0;
}

/**
 * @brief Initialize the CoAP client 
 */
static int client_init(void)
{
    int err;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0)
    {
        printk("Failed to create CoAP socket: %d.\n", errno);
        return -errno;
    }

    err = connect(sock, (struct sockaddr *)&server,
                  sizeof(struct sockaddr_in));
    if (err < 0)
    {
        printk("Connect failed : %d\n", errno);
        return -errno;
    }

    /* Initialize FDS, for poll. */
    fds.fd = sock;
    fds.events = POLLIN;

    /* Randomize token. */
    next_token = sys_rand32_get();

    return 0;
}

/**@brief Handles responses from the remote CoAP server. */
static int client_handle_get_response(uint8_t *buf, int received)
{
    int err;
    struct coap_packet reply;
    const uint8_t *payload;
    uint16_t payload_len;
    uint8_t token[8];
    uint16_t token_len;
    uint8_t temp_buf[16];

    err = coap_packet_parse(&reply, buf, received, NULL, 0);
    if (err < 0)
    {
        printk("Malformed response received: %d\n", err);
        return err;
    }

    payload = coap_packet_get_payload(&reply, &payload_len);
    token_len = coap_header_get_token(&reply, token);

    if ((token_len != sizeof(next_token)) &&
        (memcmp(&next_token, token, sizeof(next_token)) != 0))
    {
        printk("Invalid token received: 0x%02x%02x\n",
               token[1], token[0]);
        return 0;
    }

    if (payload_len > 0)
    {
        snprintf(temp_buf, MAX(payload_len, sizeof(temp_buf)), "%s", payload);
    }
    else
    {
        strcpy(temp_buf, "EMPTY");
    }

    printk("CoAP response: code: 0x%x, token 0x%02x%02x, payload: %s\n",
           coap_header_get_code(&reply), token[1], token[0], temp_buf);

    return 0;
}

/**@brief Send CoAP GET request. */
static int client_get_send(void)
{
    int err;
    struct coap_packet request;

    next_token++;

    err = coap_packet_init(&request, coap_buf, sizeof(coap_buf),
                           APP_COAP_VERSION, COAP_TYPE_NON_CON,
                           sizeof(next_token), (uint8_t *)&next_token,
                           COAP_METHOD_GET, coap_next_id());
    if (err < 0)
    {
        printk("Failed to create CoAP request, %d\n", err);
        return err;
    }

    /*err = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
					(uint8_t *)CONFIG_COAP_RESOURCE,
					strlen(CONFIG_COAP_RESOURCE));*/
    if (err < 0)
    {
        printk("Failed to encode CoAP option, %d\n", err);
        return err;
    }

    err = send(sock, request.data, request.offset, 0);
    if (err < 0)
    {
        printk("Failed to send CoAP request, %d\n", errno);
        return -errno;
    }

    printk("CoAP request sent: token 0x%04x\n", next_token);

    return 0;
}

static int client_post_send(uint8_t *payload, const uint8_t *acces_token_str)
{
    int err;
    struct coap_packet request;

    uint8_t uri_path[50] = "";
    strcat(uri_path, CONFIG_COAP_URI_PATH_API);
    strcat(uri_path, acces_token_str);
    strcat(uri_path, CONFIG_COAP_TELEMETRY);

    next_token++;

    err = coap_packet_init(&request, coap_buf, sizeof(coap_buf),
                           APP_COAP_VERSION, COAP_TYPE_NON_CON,
                           sizeof(next_token), (uint8_t *)&next_token,
                           COAP_METHOD_POST, coap_next_id());
    if (err < 0)
    {
        printk("Failed to create CoAP request, %d\n", err);
        return err;
    }

    err = coap_packet_append_option(&request, COAP_OPTION_URI_PATH,
                                    (uint8_t *)uri_path,
                                    strlen(uri_path));

    if (err < 0)
    {
        printk("Failed to encode CoAP option, %d\n", err);
        return err;
    }

    err = coap_packet_append_payload_marker(&request);
    if (err < 0)
    {
        printk("Failed to add payload marker, %d\n", err);
        return err;
    }

    coap_packet_append_payload(&request, payload, strlen(payload));

    printk(request.data);
    printk("\n");
    err = send(sock, request.data, request.offset, 0);
    if (err < 0)
    {
        printk("Failed to send CoAP request, %d\n", errno);
        return -errno;
    }
    printk("CoAP request sent: token 0x%04x\n", next_token);

    return 0;
}

#if defined(CONFIG_LWM2M_CARRIER)
K_SEM_DEFINE(carrier_registered, 0, 1);

void lwm2m_carrier_event_handler(const lwm2m_carrier_event_t *event)
{
    switch (event->type)
    {
    case LWM2M_CARRIER_EVENT_BSDLIB_INIT:
        printk("LWM2M_CARRIER_EVENT_BSDLIB_INIT\n");
        break;
    case LWM2M_CARRIER_EVENT_CONNECT:
        printk("LWM2M_CARRIER_EVENT_CONNECT\n");
        break;
    case LWM2M_CARRIER_EVENT_DISCONNECT:
        printk("LWM2M_CARRIER_EVENT_DISCONNECT\n");
        break;
    case LWM2M_CARRIER_EVENT_READY:
        printk("LWM2M_CARRIER_EVENT_READY\n");
        k_sem_give(&carrier_registered);
        break;
    case LWM2M_CARRIER_EVENT_FOTA_START:
        printk("LWM2M_CARRIER_EVENT_FOTA_START\n");
        break;
    case LWM2M_CARRIER_EVENT_REBOOT:
        printk("LWM2M_CARRIER_EVENT_REBOOT\n");
        break;
    }
}
#endif /* defined(CONFIG_LWM2M_CARRIER) */

/**@brief Configures modem to provide LTE link. Blocks until link is
 * successfully established.
 */
static void modem_configure(void)
{
#if defined(CONFIG_LTE_LINK_CONTROL)
    if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT))
    {
        /* Do nothing, modem is already turned on
		 * and connected.
		 */
    }
    else
    {
#if defined(CONFIG_LWM2M_CARRIER)
        /* Wait for the LWM2M_CARRIER to configure the modem and
		 * start the connection.
		 */
        printk("Waitng for carrier registration...\n");
        k_sem_take(&carrier_registered, K_FOREVER);
        printk("Registered!\n");
#else  /* defined(CONFIG_LWM2M_CARRIER) */
        int err;

        printk("LTE Link Connecting ...\n");
        err = lte_lc_init_and_connect();
        __ASSERT(err == 0, "LTE link could not be established.");
        printk("LTE Link Connected!\n");
#endif /* defined(CONFIG_LWM2M_CARRIER) */
    }
#endif /* defined(CONFIG_LTE_LINK_CONTROL) */
}

/* Returns 0 if data is available.
 * Returns -EAGAIN if timeout occured and there is no data.
 * Returns other, negative error code in case of poll error.
 */
static int wait(int timeout)
{
    int ret = poll(&fds, 1, timeout);

    if (ret < 0)
    {
        printk("poll error: %d\n", errno);
        return -errno;
    }

    if (ret == 0)
    {
        /* Timeout. */
        return -EAGAIN;
    }

    if ((fds.revents & POLLERR) == POLLERR)
    {
        printk("wait: POLLERR\n");
        return -EIO;
    }

    if ((fds.revents & POLLNVAL) == POLLNVAL)
    {
        printk("wait: POLLNVAL\n");
        return -EBADF;
    }

    if ((fds.revents & POLLIN) != POLLIN)
    {
        return -EAGAIN;
    }

    return 0;
}
