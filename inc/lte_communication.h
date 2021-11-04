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

#ifndef _LTE_COMMUNICATION_H
#define _LTE_COMMUNICATION_H

#include <stdint.h>

// --------------------------------------------------------- public structs ---


// ------------------------------------------------------- public functions ---

int lte_init();
void lte_close(void);
int send_data(uint8_t *payload, const uint8_t *acces_token_str);

/**
 * \brief	returns receiving signal power in dbm
 * \param[out] buffer chararray for signal power in ascii
 */
void get_signal_power(uint8_t *buffer);

void nrf_modem_recoverable_error_handler(uint32_t err);


#endif /* _LTE_COMMUNICATION_H */

