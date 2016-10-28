#include "Timer.h"
#include "led.h"
#include "box.h"
#include "adc.h"
#include "peggy.h"
#include <avr/eeprom.h>

//each peg keeps track of its own state from the states UP, DOWN, RISING, FALLING
//RISING means was stable DOWN, changed
//FALLING means was stable UP, then changed, hasn't stabilised

#define FOREACH_PEG(k) for (struct peg *k = &pegs[0];k < &pegs[6];k++)
#define FOREACH_LEFT_PEG(k) for (struct peg *k = &pegs[0];k < &pegs[3];k++)
#define FOREACH_RIGHT_PEG(k) for (struct peg *k = &pegs[3];k < &pegs[6];k++)
#define PEG_DELAY 1000

#define PEG_MESSAGE_CAPPED 1
#define PEG_MESSAGE_CLEAR 0

void
peg_tick(struct peg *p)
{
	// TODO if a peg is down, but then determined to be up, and the down message isn't sent yet or something, the up message will be discarded by the buffer and will not make it to the host.
	//determine that it isn't just thresholds or something
	//combined
	int covered = adc_values[p->adc_ix] < p->thresh;
	if (covered) {
		switch(p->state) {
		case PEG_STATE_CAPPED:
			//still capped, everything is great.
			break;
		case PEG_STATE_CAPPING:
			if ((millis() - p->state_start) > PEG_DELAY) {
				//new_peg(&pegbuf, host_millis(), p->loc, PEG_MESSAGE_CAPPED);
				//set values so that message will be sent elsewhere
				peg_stamps[p->loc] = host_millis();
				peg_msg_pending |= (1 << p->loc);
				peg_msg_state |= (1 << p->loc);
				p->state = PEG_STATE_CAPPED;
				p->state_start = millis();
			}
			break;
		case PEG_STATE_CLEARING:
		case PEG_STATE_CLEAR:
		default:
			p->state = PEG_STATE_CAPPING;
			p->state_start = millis();
		
		}
	} else {
		switch(p->state) {
		case PEG_STATE_CLEAR:
			//still clear, everything is great.
			break;
		case PEG_STATE_CLEARING:
			if ((millis() - p->state_start) > PEG_DELAY) {
				//new_peg(&pegbuf, host_millis(), p->loc, PEG_MESSAGE_CLEAR);
				peg_stamps[p->loc] = host_millis();
				peg_msg_pending |= (1 << p->loc);
				peg_msg_state &= ~(1 << p->loc);
				p->state = PEG_STATE_CLEAR;
				p->state_start = millis();
			}
			break;
		case PEG_STATE_CAPPING:
		case PEG_STATE_CAPPED:
		default:
			p->state = PEG_STATE_CLEARING;
			p->state_start = millis();
		
		}
	}
}
void
handle_pegs(void)
{
	FOREACH_PEG(p) peg_tick(p);
}

#define AS_DECLARE_HARDWARE(name, port, ddr, pin, num, loc) ddr &= ~(1<<pin);
int foo;
void
peggy_hardware(void)
{
	PEG_TABLE(AS_DECLARE_HARDWARE);
}

#define PEGGY_THRESHOLD 512
#define AS_PEGS(name, port, ddr, pin, num, locc) {.adc_ix = num, .thresh = PEGGY_THRESHOLD, .loc=locc},

struct peg pegs[PEG_COUNT] = {PEG_TABLE(AS_PEGS)};
uint16_t *peg_eep_start = 0;
void
read_peggy_thresholds(void)
{
	for (int i = 0; i < PEG_COUNT;i++)
		pegs[i].thresh = eeprom_read_word(peg_eep_start+i);
}

void
write_peggy_thresholds(void)
{
	for (int i = 0; i < PEG_COUNT;i++)
		eeprom_update_word(peg_eep_start+i, pegs[i].thresh);
}

void
self_calibrate_peggy_thresholds(void)
{
	// TODO self calibrate. not as easy as it sounds because we need to measure each sensor with both peg and open to get a good threshold (probably just the center of those two measurements) so this function needs to interact with a user. Fortunately it only needs to interact with a knowledgeable user.
	// A likely workflow: device determines all values for open pegs, then detects big changes when someone covers a peg. It makes a changing tone/rate of beeping to indicate that it is time to move to the next
}

int
play_stage_success(int x)
{
	if (x) start_flashing(&buzzer_as_led, 10, 50, 50);
	return x;
}

int buzzer_because_drop;
void
handle_drops(void)
{
	//buzz to notify of errors
	
	//for now do simple thing
	//*
	if (adc_values[DROP_ERROR_LINE] > DROP_THRESH) {
		buzzer_as_led.on();
		buzzer_because_drop = 1;
	} else {
		if (buzzer_because_drop) {
			buzzer_because_drop = 0;
			buzzer_as_led.off();
			//send drop error reason
			new_drop_error(&derrbuf, host_millis());
		}
	}
	//*/
}

void
handle_errors(void)
{
	handle_drops();
	handle_wall_errors();
}
//state 1: move pegs to center
//state 2: move pegs to right
//state 3: move pegs from right to center
//state 4: move pegs from center to left
//each peg stores its running up/down state
int
left_to_center(void)
{
	handle_errors();
	handle_pegs();
	//if all left are empty: become center_to_right
	int all_empty = 1;
	FOREACH_LEFT_PEG(p) {
		int ready = p->state == PEG_STATE_CLEAR;
		
		all_empty = all_empty && ready;
	}
	return (all_empty);
}

int
center_to_right(void)
{
	handle_errors();
	handle_pegs();
	//if all right are full: become right_to_center
	int all_full = 1;
	FOREACH_RIGHT_PEG(p) {
		int ready = p->state == PEG_STATE_CAPPED;
		
		all_full = all_full && ready;
	}
	return play_stage_success(all_full);
}

int
right_to_center(void)
{
	handle_errors();
	handle_pegs();
	//if all right are empty: become center_to_left
	int all_empty = 1;
	FOREACH_RIGHT_PEG(p) {
		int ready = p->state == PEG_STATE_CLEAR;
		
		all_empty = all_empty && ready;
	}
	return (all_empty);
}

int
center_to_left(void)
{
	handle_errors();
	handle_pegs();
	//if all left are full: win
	int all_full = 1;
	FOREACH_LEFT_PEG(p) {
		int ready = p->state == PEG_STATE_CAPPED;
		
		all_full = all_full && ready;
	}
	return play_stage_success(all_full);
}

int
win_peggy(void)
{
	handle_errors();
	return tool_in_slot();
}

int
wait_to_start_act(void)
{
	//report if the pegs are on in the status
	// this doesn't work because the pegs won't settle into "capped" for a while and the status is requested quite early. It does put the appropriate values into status though
	handle_pegs();
	uint16_t peg_status = 0;
	
	//works >>1 <<8 is ugly though
	for (int i = 0; i < PEG_COUNT;i++,peg_status<<=1)
		if (pegs[PEG_COUNT-1-i].state == PEG_STATE_CAPPED)
			peg_status |= 1;
	
	peg_status >>= 1;
	peg_status <<= 8;
	
	status &= ~((uint32_t) 0x0000ff00);
	status |= peg_status;
	
	//TODO later: make sure all pegs are on the left
	
	//wait for tools to be removed
	if (lms_said_to_start && !tool_in_slot()) {
		lms_said_to_start = 0;
		//clear this status flag -- better than leaving it stale
		status &= ~((uint32_t) 0x0000ff00);
		return 1;
	}
	return 0;
}

struct peggy_state {
	struct peggy_state *next;
	int (*f)(void);
	int msg;
};

#define LEFT_TO_CENTER_MESSAGE 11
#define CENTER_TO_RIGHT_MESSAGE 12
#define RIGHT_TO_CENTER_MESSAGE 13
#define CENTER_TO_LEFT_MESSAGE 14
#define WIN_MESSAGE 15
#define WAIT_TO_START_MESSAGE 16

struct peggy_state wait_to_start;
struct peggy_state l2c;
struct peggy_state c2r;
struct peggy_state r2c;
struct peggy_state c2l;
struct peggy_state win;
struct peggy_state wait_to_start = {&l2c, wait_to_start_act, WAIT_TO_START_MESSAGE};
struct peggy_state l2c = {&c2r, left_to_center, LEFT_TO_CENTER_MESSAGE};
struct peggy_state c2r = {&r2c, center_to_right, CENTER_TO_RIGHT_MESSAGE};
struct peggy_state r2c = {&c2l, right_to_center, RIGHT_TO_CENTER_MESSAGE};
struct peggy_state c2l = {&win, center_to_left, CENTER_TO_LEFT_MESSAGE};
struct peggy_state win = {&wait_to_start, win_peggy, WIN_MESSAGE};

struct peggy_state *cur = &wait_to_start;
void
peggy_stm_loop(void)
{
	if (lms_said_to_start) {// restart on receipt of timestamp
		cur = &wait_to_start;
		// TODO Pokey beeps continuously if you reset while an error is occurring. Peggy stops, but resumes if you take the tool away and make a new error 
		buzzer_as_led.off();
	}
	int r = cur->f();
	if (r)
		cur = cur->next;
}

int
peggy_cur_state(void)
{
	return cur->msg;
}
