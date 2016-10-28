#include <avr/io.h>
#include "Timer.h"
#include "box.h"
#include "pokey.h"
#include "adc.h"
#include "led.h"

LED_TABLE(AS_LED_OFF_FUN);
LED_TABLE(AS_LED_ON_FUN);

#define FOREACH_POKEY_LED(key) for (struct led *key=leds;key<&leds[LED_COUNT];key++)

#define AS_LED_ARRAY_INIT(name, port, ddr, num) {.on = name ## _on, .off = name ## _off},
struct led leds[LED_COUNT] = {LED_TABLE(AS_LED_ARRAY_INIT)};

inline int
int_buffer_full(struct int_buffer *ib)
{
	return !(ib->occupancy < MAX_LEDS_ON_AT_ONCE);
}
void
int_buffer_put(struct int_buffer *ib, int v)
{
	if (int_buffer_full(ib)) return;
	
	ib->buf[ib->first_empty] = v;
	ib->occupancy++;
	ib->first_empty++;
	ib->first_empty %= MAX_LEDS_ON_AT_ONCE;
}

int
int_buffer_extract(struct int_buffer *ib)
{
	if (!ib->occupancy) return -1;
	int res = ib->buf[ib->first_real];
	ib->occupancy--;
	ib->first_real++;
	ib->first_real %= MAX_LEDS_ON_AT_ONCE;
	return res;
}
struct int_buffer ledbuf;//initializing to zero fulfills invariants
void
led_on(int led)
{
	//Make a ring buffer of interior leds that are on. The function to turn on an interior led checks if the buffer is full. If it is the least recently turned on is turned off and removed, then the new one is turned on and added
	
	//check that argument is valid
	if (led > LED_COUNT) return;
	
	struct led *l = &leds[led];
	
	//check if ringbuffer is full
	if (int_buffer_full(&ledbuf)) {
		//drop first in, turn it off
		int ix = int_buffer_extract(&ledbuf);
		leds[ix].off();
	}
	l->on();
	//add l to ringbuffer
	int_buffer_put(&ledbuf, led);
		
}

//all buttons are on the mux and at 1-10 so making a table of them does not make a lot of sense
uint8_t button_values[BUTTON_COUNT];

void
read_buttons(void)
{
	PORTB &= ~(1<<PB0);
	for (int i = 0; i < BUTTON_COUNT;i++) {
		set_muxer(i+1);
		button_values[i] = !!(PINB & (1<<PB4));
	}
}

//need some buttons to be connected to leds

//sensors
#define ADC_MUX_LINE 11

void
set_muxer(uint8_t val)
{
	//set arg of muxer to val, wait however many nanoseconds (possibly no need to wait)
	int ix0, ix1, ix2, ix3;
	ix0 = !!(val&1);
	ix1 = !!(val&2);
	ix2 = !!(val&4);
	ix3 = !!(val&8);
	PORTD &= ~((1<<PD0)|(1<<PD1)|(1<<PD2)|(1<<PD3));
	if (ix0) PORTD |= (1<<PD0);
	if (ix1) PORTD |= (1<<PD1);
	if (ix2) PORTD |= (1<<PD2);
	if (ix3) PORTD |= (1<<PD3);
	
	//no waiting necessary
	
}

void
muxer_init()
{
	//set the one pin high (low?) to enable adc muxer
	DDRB |= (1<<PB0);
	PORTB &= ~(1<<PB0);
	DDRB &= ~(1<<PB4);
	DDRD |= (1<<PD0)|(1<<PD1)|(1<<PD2)|(1<<PD3);;
	
}

void
pokey_flash_handler(void)
{
	ms_time_t now = millis();
	FOREACH_POKEY_LED(l) do_flashing(l, now);
}

void
pokey_test_leds(void)
{
	FOREACH_POKEY_LED(l) start_flashing(l, 5, 1000, 1000);
	//this turns on all leds, don't leave on
}

#define FOREACH_TARGET(key) for (struct target *key=targets;key<&targets[TARGET_COUNT];key++)

#define IOTA10(_) _(0) _(1) _(2) _(3) _(4) _(5) _(6) _(7) _(8) _(9)
#define AS_TARGETS(n) {.led = &leds[n], .button_ix = n, .loc = n},
struct target targets[TARGET_COUNT] = {
	IOTA10(AS_TARGETS)
};

#define X(x) x,
uint8_t target_order[TARGET_COUNT] = {IOTA10(X)};
#undef X

ms_time_t last_err_time, game_start_time;
int order_chosen, game_started;
int target_in_play;
int game_end_type;
uint8_t game_going;

#define END_VICTORY 1
#define END_FAILURE 2
void check_pieces_init(void)
{
	game_start_time = 0;
	//timeout isn't this state's variable
	order_chosen = 0;
	game_started = 0;
	target_in_play = 0;
	game_end_type = 0;
	lms_said_to_end = 0;
	game_going = 0;
	FOREACH_TARGET(t) t->led->off();
}
void check_pieces(void)
{

	//game happens here
	while (!game_started) {
		game_start_time = millis();
		new_event(&evtbuf, host_millis(), LOC_START);
		game_started = 1;
		reset_wall_errors();
	}
	
	while (!order_chosen) {
		// this was causing the crashes fisher_yates(organ_order, SENSOR_NUM);
		target_in_play = 0;
		order_chosen = 1;
	}
	
	//record error durations and send as messages
	//handle error buzzer
    //check tool for error
	handle_wall_errors();
	
	if ((timeout != 0 && (millis() - game_start_time) > timeout) || lms_said_to_end) {
		new_event(&evtbuf, host_millis(), LOC_TIMEOUT);
		//TODO change next state or set a variable or something to indicate the timeout
		game_end_type = END_FAILURE;
	}
	
	//among other things, update o->down
	if (target_in_play < 10)
    FOREACH_TARGET(t) {
		if (t != &targets[target_order[target_in_play]]) continue;
		//top front middle is impossible
	    if (!button_values[t->button_ix] || target_order[target_in_play] == 2) {
			t->led->off();
			new_poke(&pokebuf, host_millis(), t->loc);
			//if last target, play happy sound
			if (target_in_play == 9)
				start_flashing(&buzzer_as_led, 10, 50, 50);
			//update which organ is the current organ
			target_in_play++;
			if (target_in_play == 10) break; // Don't go through again if we're done. This fixes the issue where top back light would turn on.
		} else {
			t->led->on();
		}
    }
}

void
pokey_init(void)
{
	LED_TABLE(AS_ENABLE_OUTPUT);
	muxer_init();
}

void
pokey_loop(void)
{
	if (lms_said_to_start && !tool_in_slot()) {
		check_pieces_init();
		buzzer_off();
		game_going = 1;
		lms_said_to_start = 0;
		target_in_play = 0;
	}
	if (game_going)
		check_pieces();
	if (target_in_play >= TARGET_COUNT && tool_in_slot()) {
		game_going = 0;
		buzzer_off();
		error_led->off();
	}
}
