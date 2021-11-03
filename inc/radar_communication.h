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

#ifndef UART_COMMUNICATION_H_
#define UART_COMMUNICATION_H_

#include <stdio.h>
#include <stdbool.h>

// --------------------------------------------------------- public structs ---

// ------------------------------------------------------- public functions ---
void gpio_init(void);
void uart_init(void);

void start_distance_measurement(void);
void start_speed_measurement(uint32_t distance);
bool check_data_received(void);
void rec_distance(void);
void rec_speed(void);



#endif /* UART_COMMUNICATION_H_ */
