#include "Timer.h"
#include "Config/AppConfig.h"

/* will rollover once every ~50 days */
volatile ms_time_t cur_millis = 0;
TIME_t time_oset = 0;

MODULE_TASK(timer)
{
	return;
}
PROX_HANDLER(timer)
{
	UNUSED(Data);
	return;
}
ACT_HANDLER(timer)
{
	UNUSED(Data);
	return;
}
INPUT_REQUESTEE(timer)
{
	UNUSED(Data);
	return 0;
}

MODULE_INIT(timer)
{
	/*
		for one millisecond clock hits, we want K = F_CPU / (pres * 1000) - 1 to
		be an integer. Considering prescaler options:
		pres	count
		1		15999
		8		1999
		64		249
		256		61.5
		1024	14.625
	*/
	
	/* set timer 1 to CTC mode */
	TCCR0A |= (1<<WGM01);
	/* enable interrupts for */
	TIMSK0 |= (1<<OCIE0A);
	
	/* set CTC value to 1ms */
	OCR0A = 249; /* 249;*/
	
	/* set timer 1 to a prescaler of 64 */
	TCCR0B |= (1<<CS01)|(1<<CS00);
	
	
	
	//when timer hits 249 OCF1A is set. to clear it set it to 1
	/* 16000000 / 1024 = 250 ticks in a second, set overflow check to 249 */
	
}

ISR(TIMER0_COMPA_vect)
{
	cur_millis++;
}

ms_time_t millis(void)
{
	ms_time_t ms = cur_millis;
	return ms;
}

TIME_t host_millis(void) {
	return time_oset + cur_millis;
}

void set_time_oset(TIME_t t)
{
	ms_time_t cur = cur_millis;
	time_oset = t - cur;
}

union ui64_byteview {
	uint64_t u64_val;
	uint8_t u8s[8];
};
void time_to_wire(TIME_t ms, uint8_t w[])
{
	union ui64_byteview val;
	val.u64_val = ms;
	for (int i = 0; i < 8; i++) {
		w[i] = val.u8s[7-i];
	}
}

TIME_t time_from_wire(const uint8_t w[])
{
	/* little-endian */
	union ui64_byteview val;
	for (int i = 0; i < 8; i++) {
		val.u8s[i] = w[7-i];
	}
	
	return val.u64_val;
}