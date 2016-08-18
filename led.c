#include <stdlib.h>
#include <stdint.h>
#include "Timer.h"
#include "led.h"

void do_flashing(struct led *l, ms_time_t now)
{
	if (!l->currently_flashing) return;
	//if times == -1 flash forever
	ms_time_t elapsed = now - l->cur_flash_start;
	if (l->cur_flash_mode == 1) {
		if (elapsed > l->flash_on_dur) {
			l->off();
			l->cur_flash_mode = 0;
			//it is true that we could do some special compensation for elapsed time but it would only be noticeable if you were looking on an oscope.
			l->cur_flash_start = now;
		} //else nothing, things are fine
	} else {
		if (elapsed > l->flash_off_dur) {
			l->flashes_done++;
			if (l->times_to_flash == -1 || l->flashes_done < l->times_to_flash) {
				//flash more
				l->on();
				l->cur_flash_mode = 1;
				l->cur_flash_start = now;
			} else {
				//stop flashing
				l->currently_flashing = 0;
				//it's already off
			}
		}
	}
}

/*
void flash_handler(void)
{
	ms_time_t now = millis();
	FOREACH_LED(l) do_flashing(l, now); 
}
*/

void start_flashing(struct led *l, int times, int on_dur, int off_dur)
{
	l->times_to_flash = times;
	l->flash_on_dur = on_dur;
	l->flash_off_dur = off_dur;
	l->currently_flashing = 1;
	l->flashes_done = 0;
	l->cur_flash_mode = 1;
	l->cur_flash_start = millis();
	l->on();
}

void stop_flashing(struct led *l)
{
	l->currently_flashing = 0;
	l->off();
}