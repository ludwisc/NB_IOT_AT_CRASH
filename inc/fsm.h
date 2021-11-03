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
#ifndef RADAR_FSM_H_
#define RADAR_FSM_H_

#include <stdio.h>


// --------------------------------------------------------- public structs ---
typedef enum{
    NO_EVENT,
    DISTANCE_REC_EVENT,
    SPEED_REC_EVENT,
    MEASURE_TIMER_EVENT,
    DATA_SENDED_EVENT,
    TIMEOUT_EVENT
} event_t;

// ------------------------------------------------------- public functions ---

int8_t new_event(event_t event);
void fsm_init(void);
void stateMachine(void);

#endif /* RADAR_FSM_H_ */
