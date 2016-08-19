#include <avr/io.h>
#include "Timer.h"
#include "led.h"
#include "box.h"
#include "adc.h"
#include "WireConversions.h"
#include <avr/eeprom.h>
#define UNUSED(x) (void)x

//FIXME write a generic debouncer and replace all current debouncers with it
uint32_t status;
uint8_t tool_was_in_slot;
#define BOX_TYPE_EEPROM_ADDR ((uint8_t*)13)
#define BOX_KNOWN_TYPES 2
void
set_box_type(uint8_t x)
{
	eeprom_update_byte(BOX_TYPE_EEPROM_ADDR, x);
}
uint8_t
get_box_type(void)
{
	return eeprom_read_byte(BOX_TYPE_EEPROM_ADDR);
}
uint8_t
determine_box_type(void)
{
	// read eeprom location for box type. If 0, autodetermine and set. If nonzero, be that. If it's a value above the known types, act as if it was zero.
	//this was tested with a pokey and peggy and worked for both
	uint8_t v = eeprom_read_byte(BOX_TYPE_EEPROM_ADDR);
	if (v && v <= BOX_KNOWN_TYPES) return v;
	
	uint16_t val = 0;
	val += adc_values[DROP_ERROR_LINE];
	uint16_t ref = 512*1;
	if (val < ref) {
		//all low: peggy
		v = BOX_TYPE_PEGGY;
	} else {
		//all high: pokey
		v = BOX_TYPE_POKEY;
	}
	eeprom_write_byte(BOX_TYPE_EEPROM_ADDR, v);
	return v;
}

// explicitly initializing to 0 to indicate that it's unlimited by default
ms_time_t timeout = 0;
void
buzzer_init(void)
{
	//set up timer and pwm
	//page 164 and following
	
	TCCR4A = (1<<PWM4B) | (1<<COM4B0);
	TCCR4B = (1<<CS42)|(1<<CS41); /* set prescaler to 1/32 */
	TCCR4D &= ~0x03;
}
inline void
buzzer_set_duty_cycle(uint8_t x)
{
	//assign to pwm compare register
	OCR4B = x;
}
void
buzzer_on(void) {buzzer_set_duty_cycle(0xf0);}
void
buzzer_off(void) {buzzer_set_duty_cycle(0);}

struct led buzzer_as_led = {.on = buzzer_on, .off = buzzer_off};

//FIXME fix this mess
#define AS_USE_LEDS(dir, name, led, port, ddr, num) led(dir, name, port, ddr, num)

#define AS_LED_ON_DECL(name, port, ddr, num) void name ## _on(void);

#define __notled__(dir, name, port, ddr, num)
#define __led__(dir, name, port, ddr, num) AS_LED_ON_DECL(name, port, ddr, num)
//make declarations
BOX_HARDWARE_TABLE(AS_USE_LEDS);
#undef __notled__
#undef __led__

#define AS_LED_OFF_DECL(name, port, ddr, num) void name ## _off(void);
#define __notled__(dir, name, port, ddr, num)
#define __led__(dir, name, port, ddr, num) AS_LED_OFF_DECL(name, port, ddr, num)
// make declarations
BOX_HARDWARE_TABLE(AS_USE_LEDS);
#undef __notled__
#undef __led__

#define AS_LED_ON_FUN(name, port, ddr, num) void name ## _on(void) \
  {port |= (1<<num);}
#define __notled__(dir, name, port, ddr, num)
#define __led__(dir, name, port, ddr, num) AS_LED_ON_FUN(name, port, ddr, num)
// make declarations
  BOX_HARDWARE_TABLE(AS_USE_LEDS);
#undef __notled__
#undef __led__
  
#define AS_LED_OFF_FUN(name, port, ddr, num) void name ## _off(void) \
  {port &= ~(1<<num);}
#define __notled__(dir, name, port, ddr, num)
#define __led__(dir, name, port, ddr, num) AS_LED_OFF_FUN(name, port, ddr, num) 
// make declarations
  BOX_HARDWARE_TABLE(AS_USE_LEDS);
#undef __notled__
#undef __led__
  
//not needed, done all at once
//#define AS_ENABLE_OUTPUT(name, port, ddr, num) ddr |= (1<<num);
  

#define __notled__(dir, name, port, ddr, num)
#define __led__(dir, name, port, ddr, num) {.on = name ## _on, .off = name ## _off},
#define BOX_LED_NUM 2
struct led box_leds[BOX_LED_NUM] = {BOX_HARDWARE_TABLE(AS_USE_LEDS)};

struct led *error_led = &box_leds[0];

#define FOREACH_BOX_LED(key) \
  for (struct led *key=box_leds;key<&box_leds[BOX_LED_NUM];key++)

void
box_flash_handler(void)
{
	ms_time_t now = millis();
	FOREACH_BOX_LED(l) do_flashing(l, now);
	do_flashing(&buzzer_as_led, now);
}

void
box_test_leds(void)
{
	FOREACH_BOX_LED(l) start_flashing(l,-1,250,500);
}

#define out(name, ddr, num) ddr |= (1<<num);
#define in(name, ddr, num) ddr &= ~(1<<num);
#define AS_DIRECTION_SETUP(dir, name, led, port, ddr, num) dir(name,ddr,num);
void
box_init(void)
{
	BOX_HARDWARE_TABLE(AS_DIRECTION_SETUP);
	tool_was_in_slot = tool_in_slot();
	/*
	in, tool_connected?, PORTB, PB5
	out, buzzer, PORTB, PB6
	out, panel_led, PORTB, PB7
	in, tool_holder, PORTD, PD4
	in, wall_error, PORTD, PD6
	*/
}

int error;
uint8_t error_end_recorded;
ms_time_t last_err_time, err_early_end_time;
TIME_t host_err_time;
void
handle_wall_errors(void)
{
	// TODO this should work in a different way, so that it waits for the current error to end, then waits the delay time. If no new error happens in that time, send the old one, otherwise add the time to this one
	// TODO currently there is some fancy buzzer management code but it can be avoided with start_flashing(&buzzer_as_led, 1, BUZZER_LENGTH, 0);
	// if it is short and no other one happens, provide the short duration. This means that this can't use the debouncing code
    if (cur_tool == WALL_ERROR_OK) {
		if (!error) {
			error = 1;
			last_err_time = millis();
			host_err_time = host_millis();
			buzzer_on();
			error_led->on();
		}
		error_end_recorded = 0;
    }
	if (error && cur_tool != WALL_ERROR_OK && !error_end_recorded) {
		err_early_end_time = millis();
		error_end_recorded = 1;
	}
    if (error
			&& ((millis() - last_err_time) > MIN_BUZZER_LENGTH)
			&& cur_tool != WALL_ERROR_OK) {
		if (!error_end_recorded) //I don't think this will ever execute
			err_early_end_time = millis();
		buzzer_off();
		error_led->off();
		ms_time_t elapsed = err_early_end_time - last_err_time;
		error = 0;
		error_end_recorded = 0;
		//store error report in buffer
		new_wall_error(&werrbuf, host_err_time, elapsed);
	}
}
void
reset_wall_errors(void)
{
	error = 0;
}

int
tool_in_slot(void)
{
	//return if the tool is in
	return (cur_tool == NO_TOOL) ? 2 : adc_values[TOOL_HOLDER_LINE] > 512;
}
tool_state
classify_tool(int tool_error, int tool_jack)
{
	tool_error = tool_error < 512;
	tool_jack = tool_jack > 512;
	tool_state err;
	if (tool_error) {
		if (tool_jack) {
			err = WALL_ERROR_OK;
		} else {
			err = WALL_ERROR_WRONG; // hardware manufacturing error
			//notify home base, happens in box_tick
		}
	} else {
		if (tool_jack) {
			err = TOOL_IN_AND_OK;
		} else {
			err = NO_TOOL; // user error
		}
	}
	
	return err;
}

#define TOOL_DELAY 200
#define TOOL_STATE_FOOTPRINT 3
void
box_tick(void)
{
	cur_tool = classify_tool(
		adc_values[TOOL_ERROR_LINE], adc_values[TOOL_CONNECTED_LINE]
	);
	
	if (cur_tool == WALL_ERROR_WRONG) {
		//set part of status, thereby notifying lms
		status |= BAD_TOOL_STATUS_F;
	} else {
		status &= ~BAD_TOOL_STATUS_F;
	}
	
	//TODO debounce properly
	static uint8_t msg;
	static uint8_t pending;
	static TIME_t host_started;
	static ms_time_t started;
	static uint8_t last_msg_sent = -1;
	uint8_t tool_in = tool_in_slot();
	if (tool_in != tool_was_in_slot && tool_in != last_msg_sent) {
		pending = 1;
		msg = tool_was_in_slot = tool_in;
		started = millis();
		host_started = host_millis();
	}
	
	if (pending && (millis()-started) > TOOL_DELAY) {
		pending = 0;
		status &= ~((uint32_t)TOOL_STATE_FOOTPRINT << 1);
		status |= msg<<1;
		new_tool(&toolbuf, host_started, last_msg_sent = msg);
	}
	//new_tool(&toolbuf, host_millis(), tool_was_in_slot=tool_in);
}

void new_wall_error(struct wall_error_buffer *b, uint64_t stamp, ms_time_t dur)
{
	if (b->occupancy >= WALL_ERROR_BUFFER_SIZE) return;
	b->stamps[b->first_empty] = stamp;
	b->durs[b->first_empty] = dur;
	b->occupancy++;
	b->first_empty++;
	b->first_empty %= WALL_ERROR_BUFFER_SIZE;
}
void new_drop_error(struct drop_error_buffer *b, uint64_t stamp)
{
	if (b->occupancy >= DROP_ERROR_BUFFER_SIZE) return;
	b->stamps[b->first_empty] = stamp;
	b->occupancy++;
	b->first_empty++;
	b->first_empty %= DROP_ERROR_BUFFER_SIZE;
}
void new_poke(struct poke_buffer *b, uint64_t stamp, uint8_t loc)
{
	if (b->occupancy >= POKE_BUFFER_SIZE) return;
	b->stamps[b->first_empty] = stamp;
	b->locs[b->first_empty] = loc;
	b->occupancy++;
	b->first_empty++;
	b->first_empty %= POKE_BUFFER_SIZE;
}
void new_tool(struct tool_buffer *b, uint64_t stamp, uint8_t newst)
{
	if (b->occupancy >= TOOL_BUFFER_SIZE) return;
	b->stamps[b->first_empty] = stamp;
	b->newsts[b->first_empty] = newst;
	b->occupancy++;
	b->first_empty++;
	b->first_empty %= TOOL_BUFFER_SIZE;
}
void new_event(struct event_buffer *eb, uint64_t stamp, uint8_t typ)
{
	if (eb->occupancy >= EVENT_BUFFER_SIZE)
	eb->stamps[eb->first_empty] = stamp;
	eb->typs[eb->first_empty] = typ;
	eb->occupancy++;
	eb->first_empty++;
	eb->first_empty %= EVENT_BUFFER_SIZE;
}

int extract_wall_error(struct wall_error_buffer *eb, uint8_t *buf, int buflen)
{
	if (buflen < (8 + 4) || !eb->occupancy) return -1;
	
	time_to_wire(eb->stamps[eb->first_real], buf);
	uint32_to_wire(eb->durs[eb->first_real], &buf[8]);
	eb->occupancy--;
	eb->first_real++;
	eb->first_real %= WALL_ERROR_BUFFER_SIZE;
	return 0;
}
int extract_drop_error(struct drop_error_buffer *eb, uint8_t *buf, int buflen)
{
	if (buflen < 8 || !eb->occupancy) return -1;
	
	time_to_wire(eb->stamps[eb->first_real], buf);
	eb->occupancy--;
	eb->first_real++;
	eb->first_real %= DROP_ERROR_BUFFER_SIZE;
	return 0;
}
int extract_poke(struct poke_buffer *eb, uint8_t *buf, int buflen)
{
	if (buflen < (8 + 1) || !eb->occupancy) return -1;
	
	time_to_wire(eb->stamps[eb->first_real], buf);
	buf[8] = eb->locs[eb->first_real];
	eb->occupancy--;
	eb->first_real++;
	eb->first_real %= POKE_BUFFER_SIZE;
	return 0;
}
int extract_tool(struct tool_buffer *eb, uint8_t *buf, int buflen)
{
	if (buflen < (8 + 1) || !eb->occupancy) return -1;
	
	time_to_wire(eb->stamps[eb->first_real], buf);
	buf[8] = eb->newsts[eb->first_real];
	eb->occupancy--;
	eb->first_real++;
	eb->first_real %= TOOL_BUFFER_SIZE;
	return 0;
}
int extract_event(struct event_buffer *eb, unsigned char *buf, int buflen)
{
	if (buflen < (8 + 1) || !eb->occupancy) return -1;
	
	time_to_wire(eb->stamps[eb->first_real], buf);
	buf[8] = eb->typs[eb->first_real];
	eb->occupancy--;
	eb->first_real++;
	eb->first_real %= EVENT_BUFFER_SIZE;
	return 0;
}
