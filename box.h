#define BOX_HARDWARE_TABLE(_) \
  _(in, tool_jack, __notled__, PORTB, DDRB, PB5)\
  _(out, buzzer, __notled__, PORTB, DDRB, PB6)\
  _(out, panel_led, __led__, PORTB, DDRB, PB7)\
  _(out, debug_led, __led__, PORTD, DDRD, PD5)\
  _(in, tool_holder, __notled__, PORTD, DDRD, PD4)\
  _(in, wall_error, __notled__, PORTD, DDRD, PD6)

#define DROP_ERROR_LINE 10

#define BOX_TYPE_PEGGY 1
#define BOX_TYPE_POKEY 2
uint8_t box_type;

#define BAD_TOOL_STATUS_F 0x01
uint32_t status;
void
set_box_type(uint8_t x);
uint8_t
get_box_type(void);
uint8_t
determine_box_type(void);
struct led *error_led;

ms_time_t timeout;

#define TOOL_ERROR_LINE 9
#define TOOL_HOLDER_LINE 8
#define TOOL_CONNECTED_LINE 12
typedef enum {
	WALL_ERROR_OK = 0,
	WALL_ERROR_WRONG = 1,
	NO_TOOL = 2,
	TOOL_IN_AND_OK = 3
} tool_state;
tool_state cur_tool;
int
tool_in_slot(void);

//how long must an error be to count as an error against the student?
uint16_t wall_error_timeout;
void
handle_wall_errors(void);
void
reset_wall_errors(void);

void
box_init(void);
void
buzzer_init(void);
void
buzzer_on(void);
void
buzzer_off(void);
#define MIN_BUZZER_LENGTH 250
struct led buzzer_as_led;


void
box_flash_handler(void);
void
box_test_leds(void);
void
box_tick(void);

uint8_t lms_said_to_start;
uint8_t lms_said_to_end;

/* reporting ringbuffers */
//These could perhaps be unified into one but that might use more space. Having lots will certainly work, so I will try it first.
//to unify it would be a ring buffer of (start ptr, len, report id) and then a byte buffer
//one advantage of seperate buffers is sending is prioritized and space is reserved

#define LOC_START 0
#define LOC_END (-1)
#define LOC_TIMEOUT (-2)
#define LOC_READY 42

#define WALL_ERROR_BUFFER_SIZE 3
#define DROP_ERROR_BUFFER_SIZE 2
#define POKE_BUFFER_SIZE 3
#define TOOL_BUFFER_SIZE 3
#define EVENT_BUFFER_SIZE 3

#define RINGBUFFER_INNARDS int first_empty; unsigned int occupancy; int first_real;
struct wall_error_buffer {
	uint64_t stamps[WALL_ERROR_BUFFER_SIZE];
	uint32_t durs[WALL_ERROR_BUFFER_SIZE];
	RINGBUFFER_INNARDS;
};
struct drop_error_buffer {
	uint64_t stamps[DROP_ERROR_BUFFER_SIZE];
	RINGBUFFER_INNARDS;
};
struct poke_buffer {
	uint64_t stamps[POKE_BUFFER_SIZE];
	uint8_t locs[POKE_BUFFER_SIZE];
	RINGBUFFER_INNARDS;
};
struct tool_buffer {
	uint64_t stamps[TOOL_BUFFER_SIZE];
	uint8_t newsts[TOOL_BUFFER_SIZE];
	RINGBUFFER_INNARDS;
};
struct event_buffer {
	uint64_t stamps[EVENT_BUFFER_SIZE];
	uint8_t typs[EVENT_BUFFER_SIZE];
	RINGBUFFER_INNARDS;
};

struct wall_error_buffer werrbuf;
struct drop_error_buffer derrbuf;
struct poke_buffer pokebuf;
struct tool_buffer toolbuf;
struct event_buffer evtbuf;

void new_wall_error(struct wall_error_buffer *we, uint64_t, ms_time_t);
void new_drop_error(struct drop_error_buffer *de, uint64_t);
void new_poke(struct poke_buffer *pb, uint64_t, uint8_t);
void new_tool(struct tool_buffer *tb, uint64_t, uint8_t);
void new_event(struct event_buffer *eb, uint64_t, uint8_t);
int extract_wall_error(struct wall_error_buffer *eb, uint8_t *buf, int buflen);
int extract_drop_error(struct drop_error_buffer *eb, uint8_t *buf, int buflen);
int extract_poke(struct poke_buffer *eb, uint8_t *buf, int buflen);
int extract_tool(struct tool_buffer *eb, uint8_t *buf, int buflen);
int extract_event(struct event_buffer *eb, uint8_t *buf, int buflen);
