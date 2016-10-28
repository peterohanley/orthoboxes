#include <stdint.h>
#include "WireConversions.h"

union uint32_byteview {
	uint32_t val;
	uint8_t u8s[4];
};
void uint32_to_wire(uint32_t v, uint8_t w[])
{
	union uint32_byteview ubv;
	ubv.val = v;
	for (int i = 0; i < 4; i++) {
		w[i] = ubv.u8s[3-i];
	}
}
void uint16_to_wire(uint16_t v, uint8_t w[])
{
	w[0] = v>>8;
	w[1] = v&0xff;
}
union float_byteview {
	float val;
	uint8_t u8s[4];
};
void float_to_wire(float f, uint8_t w[])
{
	union float_byteview fbv;
	fbv.val = f;
	for (int i = 0; i < 4; i++) {
		w[i] = fbv.u8s[i];
	}
}
float float_from_wire(const uint8_t w[])
{
	union float_byteview fbv;
	for (int i = 0; i < 4; i++) {
		fbv.u8s[i] = w[i];
	}
	return fbv.val;
}