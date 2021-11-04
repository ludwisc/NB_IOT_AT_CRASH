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
#include <modem/at_notif.h>
#include <random/rand32.h>
#include <date_time.h>
#if defined(CONFIG_LWM2M_CARRIER)
#include <lwm2m_carrier.h>
#endif

#include "lte_communication.h"

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
static int client_post_send(uint8_t *payload, const uint8_t *acces_token_str);
static void modem_configure(void);
// ------------------------------------------------------- public functions ---

int lte_init()
{
    int err;

    err = at_cmd_init();
	if (err) {
		printk("Failed to initialize AT commands, err %d\n", err);
		return err;
	}

	err = at_notif_init();
	if (err) {
		printk("Failed to initialize AT notifications, err %d\n", err);
		return err;
	}
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


int send_data(uint8_t *payload, const uint8_t *acces_token_str)
{
    int err;
    err = client_post_send(payload, acces_token_str);

    return err;
}

void get_signal_power(uint8_t *buffer)
{
    enum at_cmd_state state;
    char temp[50];
    uint8_t *ptr;
    const uint8_t delim[] = ",";
    int16_t signalPower = 0;
    printk("Befor AT\n");
    at_cmd_write(get_signal_power_command, temp, sizeof(temp), &state);
    printk("After AT\n");

    if (state == AT_CMD_OK)
    {
        // rsrp is 5th element of the string
        ptr = strtok(temp, delim);
        for (uint8_t u = 0; u < 5; u++)
        {
            ptr = strtok(NULL, delim);
        }
        signalPower = atoi(ptr);
        signalPower -= 140;
    }

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

    err = send(sock, request.data, request.offset, 0);
    if (err < 0)
    {
        printk("Failed to send CoAP request, %d\n", errno);
        return -errno;
    }
    printk("CoAP request sent: token 0x%04x\n", next_token);

    return 0;
}


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
        printk("Waiting for carrier registration...\n");
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