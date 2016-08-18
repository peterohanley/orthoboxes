#ifndef _TIMER_H_
#define _TIMER_H_
//TODO remove include guards and relocate these header inclusions
	#include <avr/io.h>
	#include <avr/interrupt.h>
	#include "ModuleSystem.h"
	#include <stdbool.h>

	typedef uint32_t ms_time_t;
	typedef uint64_t TIME_t;

	ms_time_t millis(void);
	TIME_t host_millis(void);
	
	MODULE_TASK(timer);
	MODULE_INIT(timer);
	PROX_HANDLER(timer);
	ACT_HANDLER(timer);
	INPUT_REQUESTEE(timer);
	//void setup_timer(void);
	void set_time_oset(TIME_t);
	
	void time_to_wire(TIME_t t, uint8_t w[]);
	TIME_t time_from_wire(const uint8_t w[]);
#endif