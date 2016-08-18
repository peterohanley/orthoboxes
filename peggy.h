//name, port, num, adc line
#define PEG_COUNT 6
#define PEG_TABLE(_) _(peg1, PORTF, DDRF, PF0, 0, 0)\
  _(peg2, PORTF, DDRF, PF1, 1, 1)\
  _(peg3, PORTF, DDRF, PF4, 4, 2)\
  _(peg4, PORTF, DDRF, PF5, 5, 3)\
  _(peg5, PORTF, DDRF, PF6, 6, 4)\
  _(peg6, PORTF, DDRF, PF7, 7, 5)
#define DROP_THRESH 0x003E

#define PEG_STATE_CAPPED 1
#define PEG_STATE_CLEAR 3
#define PEG_STATE_CAPPING 2
#define PEG_STATE_CLEARING 4
struct peg {
	int adc_ix;
	uint8_t loc;
	ms_time_t state_start;
	unsigned int thresh;
	uint8_t state;
};

struct peg pegs[PEG_COUNT];

void
peg_tick(struct peg *);

void
peggy_hardware(void);
void
peggy_stm_loop(void);
int
peggy_cur_state(void);

void
write_peggy_thresholds(void);
void
read_peggy_thresholds(void);
