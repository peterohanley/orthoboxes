#include <stdint.h>
#include <avr/io.h>
uint16_t adc_values[13];

uint16_t adc_read(int pin) {
	uint8_t lo, hi;
	uint8_t pinlo = pin & 0x07;
	
	if (pin & 0x08) { //FIXME optimize away the if
		ADCSRB |= (1<<MUX5);
	} else {
		ADCSRB &= ~(1<<MUX5);
	}
	
	//ADMUX &= 0xf8; //clear low pins
	//ADMUX |= pinlo;
	ADMUX = (ADMUX & 0xe0) | pinlo; //clear low pins
	
	ADCSRA |= (1<<ADSC); //get an adc value
	while (ADCSRA & (1<<ADSC)); //Wait for it to do the adc
	lo = ADCL;
	hi = ADCH;
	return (hi << 8) | lo;
}

void adc_task(void) {
	int i;
	for (i = 0; i < 13; i++) {
		adc_values[i] = adc_read(i);
	}
}