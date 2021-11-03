#ifndef MEASURE_H_
#define MEASURE_H_

#include <stdio.h>

void new_time(uint64_t time);
void new_datapoint(void);
void new_distance(uint16_t distance);
void new_speed(uint16_t speed);

void get_results(uint8_t *results);
uint32_t get_last_distance(void);
uint32_t get_last_speed(void);

#endif /* MEASURE_H_ */
