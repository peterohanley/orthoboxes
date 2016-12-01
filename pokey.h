// name, port, ddr, number
#define LED_TABLE(_)   _(targetled1, PORTF, DDRF, PF0)\
  _(targetled2, PORTF, DDRF, PF1)\
  _(targetled3, PORTF, DDRF, PF4)\
  _(targetled4, PORTF, DDRF, PF5)\
  _(targetled5, PORTF, DDRF, PF6)\
  _(targetled6, PORTF, DDRF, PF7)\
  _(targetled7, PORTE, DDRE, PE2)\
  _(targetled8, PORTE, DDRE, PE6)\
  _(targetled9, PORTC, DDRC, PC6)\
  _(targetled10, PORTC, DDRC, PC7)



#define AS_LED_ON_DECL(name, port, ddr, num) void name ## _on(void);
#define AS_LED_OFF_DECL(name, port, ddr, num) void name ## _off(void);
#define AS_LED_ON_FUN(name, port, ddr, num) void name ## _on(void) \
  {port |= (1<<num);}
#define AS_LED_OFF_FUN(name, port, ddr, num) void name ## _off(void) \
  {port &= ~(1<<num);}
#define AS_ENABLE_OUTPUT(name, port, ddr, num) ddr |= (1<<num);

#define LED_COUNT 10
#define TARGET_COUNT 10
#define MAX_LEDS_ON_AT_ONCE 5

LED_TABLE(AS_LED_OFF_DECL);
LED_TABLE(AS_LED_ON_DECL);

struct target {
	struct led *led;
	int button_ix;
	int loc;
};

uint8_t shuffle_order;
uint8_t target_order[10];
struct int_buffer {
	int buf[MAX_LEDS_ON_AT_ONCE];
	int first_empty;
	unsigned int occupancy;
	int first_real;
};

void
led_on(int led);
void
pokey_init(void);
void
pokey_loop(void);

#define BUTTON_COUNT 10

uint8_t button_values[BUTTON_COUNT];
void
set_muxer(uint8_t val);
void
muxer_init(void);
void
read_buttons(void);

void
pokey_flash_handler(void);
void
pokey_test_leds(void);
