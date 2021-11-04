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
#include "fsm.h"

// ------------------------------------------------------- static variables ---

// -------------------------------------------- private function prototypes ---

// ------------------------------------------------------- public functions ---

void main(void)
{
	printk("Siedlungsentwaesserung started\n");

	fsm_init();
	
	if (lte_init() != 0)
	{
		printk("LTE Connection could not be initialiesed");
		return;
	}

	while (1)
	{
		k_sleep(Z_TIMEOUT_MS(20));
		stateMachine();
	}

	lte_close();
}
