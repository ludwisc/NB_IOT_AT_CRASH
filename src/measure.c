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
 * \brief	Storing and converting measurements to json
 * 
 * \author	Matthias Ludwig (ludw)
 * \version	1.0
 */

#include "measure.h"
#include "lte_communication.h"

#include <zephyr.h>
#include <string.h>
#include <nrf9160.h>

// ------------------------------------------------------- static variables ---

typedef struct
{
    uint64_t time;
    uint16_t speed;
    uint16_t distance;
} measure_results;

static measure_results result_buffer[50];
static uint16_t measure_data_counter = 0;

// -------------------------------------------- private function prototypes ---
/**
 * \brief	convert measured data to a json-string
 * \param[inout]	buffer	pointer to a buffer for holding the string
 * \param[in]	flag_rsrp	flag to ad the received signal power
 * \return	nothing
 */
static void structToJSON(uint8_t *buffer, bool flag_rsrp);

/**
 * \brief	prints a uint64_t in a char array
 * \param[inout]	buffer	pointer to a buffer with min length of 21
 * \param[in]	number	number to be printed
 * \return	nothing
 */
void sprintf_uint64(uint8_t *buffer, uint64_t number);

// ------------------------------------------------------- public functions ---

uint32_t get_last_distance(void)
{
    return result_buffer[measure_data_counter].distance;
}

uint32_t get_last_speed(void)
{
    return result_buffer[measure_data_counter].speed;
}

void new_time(uint64_t time)
{
    result_buffer[measure_data_counter].time = time;
    //strcpy(result_buffer[measure_data_counter].time, time);
}

void new_datapoint(void)
{
    measure_data_counter++;
}

void new_distance(uint16_t distance)
{
    result_buffer[measure_data_counter].distance = distance;
}

void new_speed(uint16_t speed)
{
    result_buffer[measure_data_counter].speed = speed;
}

void get_results(uint8_t *results)
{
    structToJSON(results, 1);
    measure_data_counter = 0;
}

// ------------------------------------------------------ private functions ---

static void structToJSON(uint8_t *buffer, bool flag_rsrp)
{
    uint8_t temp[21];
    uint8_t sig_pwr[21];
    uint8_t jsonString[1500];

    jsonString[0] = '\0';
    printk("before get sig pwr\n");
    get_signal_power(sig_pwr);
    printk("after get sig pwr\n");
    strcat(jsonString, "[");
    for (uint16_t m = 1; m <= measure_data_counter; m++)
    {
        strcat(jsonString, "{'ts':");
        sprintf_uint64(temp, result_buffer[m].time);
        strcat(jsonString, temp);
        strcat(jsonString, ",'values': {");
        strcat(jsonString, "'distance': ");
        sprintf(temp, "%d", result_buffer[m].distance);
        strcat(jsonString, temp);
        strcat(jsonString, ",'speed':");
        sprintf(temp, "%d", result_buffer[m].speed);
        strcat(jsonString, temp);

        strcat(jsonString, ",RSRP: ");
        strcat(jsonString, sig_pwr);
        
        strcat(jsonString, "}},");
    }
    jsonString[strlen(jsonString) - 1] = '\0';  // removes last ","
    strcat(jsonString, "]");

    //strcat(jsonString, "{datapoints: ");
    //sprintf(temp, "%d", measure_data_counter);
    //strcat(jsonString, temp);
//
    //if (flag_rsrp)
    //{
    //    get_signal_power(temp);
    //    strcat(jsonString, ",RSRP: ");
    //    strcat(jsonString, temp);
    //}
//
    //if (measure_data_counter)
    //{
    //    strcat(jsonString, ",data: [");
//
    //    for (uint16_t m = 1; m <= measure_data_counter; m++)
    //    {
    //        strcat(jsonString, "{'ts':");
    //        sprintf_uint64(temp, result_buffer[m].time);
    //        strcat(jsonString, temp);
//
    //        strcat(jsonString, ",'distance': ");
    //        sprintf(temp, "%d", result_buffer[m].distance);
    //        strcat(jsonString, temp);
//
    //        strcat(jsonString, ",'speed':");
    //        sprintf(temp, "%d", result_buffer[m].speed);
    //        strcat(jsonString, temp);
//
    //        
    //        strcat(jsonString, "},");
    //    }
    //    jsonString[strlen(jsonString) - 1] = '\0';  // removes last ","
    //    strcat(jsonString, "]}");
    //}
    //else
    //{
    //    strcat(jsonString, "}");
    //}
    printk("%s\n", jsonString);

    strcpy(buffer, jsonString);
}


void sprintf_uint64(uint8_t *buffer, uint64_t number)
{
    uint8_t temp_buffer[21];
    uint8_t i = 0;
    uint8_t str_length = 0;

    do
    {
        temp_buffer[i] = number % 10 + 48;  // convert number to char in reverse order
        number /= 10;
        i++;
    }while(number>0);

    str_length = i;

    for(i=0;i<str_length; i++)
    {
        buffer[i] = temp_buffer[(str_length-1)-i];  // correct order
    }

    buffer[str_length] = '\0';
}