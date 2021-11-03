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
 * \brief		LTE Communication and state handler for Siedlungsentwässerung
 * 
 * \copyright 	SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 * \author		Matthias Ludwig (ludw)
 * \version		1.0
 */

#include <stdio.h>
#include <string.h>

#include <zephyr.h>
#include <net/coap.h>
#include <net/socket.h>
#include <modem/lte_lc.h>

#include "lte_communication.h"
#include "radar_communication.h"
#include "fsm.h"

// ------------------------------------------------------- static variables ---

// -------------------------------------------- private function prototypes ---

// ------------------------------------------------------- public functions ---

void main(void)
{
	//int64_t next_msg_time = send_interval_ms;
	//int err, received;

	printk("Siedlungsentwaesserung started\n");

	fsm_init();
	uart_init();
	gpio_init();
	
	if (lte_init() != 0)
	{
		printk("LTE Connection could not be initialiesed");
		return;
	}

	//next_msg_time = k_uptime_get();

	while (1)
	{
		k_sleep(Z_TIMEOUT_MS(20));
		stateMachine();

		//if (k_uptime_get() >= next_msg_time)
		//{
		//	if (send_data(acces_token) != 0)
		//	{
		//		printk("Failed to send GET request, exit...\n");
		//		break;
		//	}
//
		//	next_msg_time += send_interval_ms;
		//}
//
		//int64_t remaining = next_msg_time - k_uptime_get();
//
		//if (remaining < 0)
		//{
		//	remaining = 0;
		//}
//
		//err = lte_wait(remaining);
		//if (err < 0)
		//{
		//	if (err == -EAGAIN)
		//	{
		//		continue;
		//	}
//
		//	printk("Poll error, exit...\n");
		//	break;
		//}

		//received = recv(sock, coap_buf, sizeof(coap_buf), MSG_DONTWAIT);
		//if (received < 0) {
		//	if (errno == EAGAIN || errno == EWOULDBLOCK) {
		//		printk("socket EAGAIN\n");
		//		continue;
		//	} else {
		//		printk("Socket error, exit...\n");
		//		break;
		//	}
		//}
		//
		//if (received == 0) {
		//	printk("Empty datagram\n");
		//	continue;
		//}
		//
		//err = client_handle_get_response(coap_buf, received);
		//if (err < 0) {
		//	printk("Invalid response, exit...\n");
		//	break;
		//}
	}

	lte_close();
}
