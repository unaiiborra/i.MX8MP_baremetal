#include <kernel/panic.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>
#include <lib/string.h>

void strcopy(char *dst, char *src, uint64 max_size)
{
	for (uint64 i = 0; i < max_size - 1; i++) {
		dst[i] = src[i];

		if (src[i] == '\0') return;
	}

	dst[max_size - 1] = '\0';
}

char uint8_to_ascii_char(uint8 n)
{
	if (n > 9) {
		PANIC(
			"uint8_to_ascii_char: requires the input number to be between 0 "
			"and 9");
	}

	return (char)(n + '0');
}

static const uint8 STDINT_REPR_CHARS[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

char *stdint_to_ascii(STDINT_UNION n, STDINT_TYPES n_type, char *buf,
					  uint64 buf_len, STDINT_REPR repr)
{
	uint64 repr_len = 0;
	switch (repr) {
		case STDINT_REPR_DEC:
			break;
		case STDINT_REPR_HEX:
			repr_len = 2;
			break;
		case STDINT_REPR_BIN:
			repr_len = 2;
			break;
		case STDINT_REPR_OCT:
			repr_len = 1;
			break;
		default:
			PANIC("Unexpected value of repr");
	}

	bool is_signed;
	int64 signed_value = 0;
	uint64 unsigned_value = 0;

	switch (n_type) {
		case STDINT_INT8:
			signed_value = n.int8;
			is_signed = true;
			break;
		case STDINT_UINT8:
			unsigned_value = n.uint8;
			is_signed = false;
			break;
		case STDINT_INT16:
			signed_value = n.int16;
			is_signed = true;
			break;
		case STDINT_UINT16:
			unsigned_value = n.uint16;
			is_signed = false;
			break;
		case STDINT_INT32:
			signed_value = n.int32;
			is_signed = true;
			break;
		case STDINT_UINT32:
			unsigned_value = n.uint32;
			is_signed = false;
			break;
		case STDINT_INT64:
			signed_value = n.int64;
			is_signed = true;
			break;
		case STDINT_UINT64:
			unsigned_value = n.uint64;
			is_signed = false;
			break;
		default:
			PANIC("stdint_to_ascii: STDINT_TYPES case not supported");
	}

	uint64 value;
	bool negative = false;

	if (is_signed) {
		if (signed_value < 0) {
			negative = true;
			value = (uint64)-signed_value;
		} else {
			value = (uint64)signed_value;
		}
	} else {
		value = unsigned_value;
	}

	if (value == 0 && !negative) {
		if (buf_len < 2 + repr_len) goto panic;
		int64 i = 0;
		switch (repr) {
			case STDINT_REPR_DEC:
				break;
			case STDINT_REPR_HEX:
				buf[i++] = '0';
				buf[i++] = 'x';
				break;
			case STDINT_REPR_BIN:
				buf[i++] = '0';
				buf[i++] = 'b';
				break;
			case STDINT_REPR_OCT:
				buf[i++] = '0';
				break;
		}
		buf[i++] = '0';
		buf[i] = '\0';
		return buf;
	}

	uint64 i = 0;
	while (value > 0) {
		// repr enum values represent the base
		uint8 digit = value % repr;
		value /= repr;

		// + 1 for the \0 + negative (0 or 1) for '-'
		if (i + 1 + (uint64)negative + repr_len >= buf_len) goto panic;

		buf[i++] = STDINT_REPR_CHARS[digit];
	}

	switch (repr) {
		case STDINT_REPR_DEC:
			break;
		case STDINT_REPR_HEX:  // Values are reversed after
			buf[i++] = 'x';
			buf[i++] = '0';
			break;
		case STDINT_REPR_BIN:
			buf[i++] = 'b';
			buf[i++] = '0';
			break;
		case STDINT_REPR_OCT:
			buf[i++] = '0';
			break;
	}

	if (negative) buf[i++] = '-';
	buf[i] = '\0';

	// Reverse created string
	uint64 start = 0;
	uint64 end = i - 1;	 // i -1 to not include the '\0'
	while (start < end) {
		char tmp = buf[start];
		buf[start] = buf[end];
		buf[end] = tmp;
		start++;
		end--;
	}

	return buf;

panic:
	PANIC(
		"stdint_to_ascii: provided buffer is smaller than requied "
		"for provided stdint");

	return (char *)0x0;
}

/*
void test_stdint_to_ascii(int64 test_v, uint64 buf_size)
{
	char buf[buf_size];

	// ==========================
	// INT8
	// ==========================
	stdint_to_ascii((STDINT_UNION){.int8 = (int8)test_v}, STDINT_INT8, buf,
					buf_size, STDINT_REPR_DEC);
	UART_puts(UART_ID_2, "STDINT_REPR_DEC int8: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int8 = (int8)test_v}, STDINT_INT8, buf,
					buf_size, STDINT_REPR_HEX);
	UART_puts(UART_ID_2, "STDINT_REPR_HEX int8: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int8 = (int8)test_v}, STDINT_INT8, buf,
					buf_size, STDINT_REPR_BIN);
	UART_puts(UART_ID_2, "STDINT_REPR_BIN int8: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int8 = (int8)test_v}, STDINT_INT8, buf,
					buf_size, STDINT_REPR_OCT);
	UART_puts(UART_ID_2, "STDINT_REPR_OCT int8: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	// ==========================
	// UINT8
	// ==========================
	stdint_to_ascii((STDINT_UNION){.uint8 = (uint8)test_v}, STDINT_UINT8, buf,
					buf_size, STDINT_REPR_DEC);
	UART_puts(UART_ID_2, "STDINT_REPR_DEC uint8: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint8 = (uint8)test_v}, STDINT_UINT8, buf,
					buf_size, STDINT_REPR_HEX);
	UART_puts(UART_ID_2, "STDINT_REPR_HEX uint8: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint8 = (uint8)test_v}, STDINT_UINT8, buf,
					buf_size, STDINT_REPR_BIN);
	UART_puts(UART_ID_2, "STDINT_REPR_BIN uint8: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint8 = (uint8)test_v}, STDINT_UINT8, buf,
					buf_size, STDINT_REPR_OCT);
	UART_puts(UART_ID_2, "STDINT_REPR_OCT uint8: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	// ==========================
	// INT16
	// ==========================
	stdint_to_ascii((STDINT_UNION){.int16 = (int16)test_v}, STDINT_INT16, buf,
					buf_size, STDINT_REPR_DEC);
	UART_puts(UART_ID_2, "STDINT_REPR_DEC int16: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int16 = (int16)test_v}, STDINT_INT16, buf,
					buf_size, STDINT_REPR_HEX);
	UART_puts(UART_ID_2, "STDINT_REPR_HEX int16: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int16 = (int16)test_v}, STDINT_INT16, buf,
					buf_size, STDINT_REPR_BIN);
	UART_puts(UART_ID_2, "STDINT_REPR_BIN int16: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int16 = (int16)test_v}, STDINT_INT16, buf,
					buf_size, STDINT_REPR_OCT);
	UART_puts(UART_ID_2, "STDINT_REPR_OCT int16: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	// ==========================
	// UINT16
	// ==========================
	stdint_to_ascii((STDINT_UNION){.uint16 = (uint16)test_v}, STDINT_UINT16,
					buf, buf_size, STDINT_REPR_DEC);
	UART_puts(UART_ID_2, "STDINT_REPR_DEC uint16: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint16 = (uint16)test_v}, STDINT_UINT16,
					buf, buf_size, STDINT_REPR_HEX);
	UART_puts(UART_ID_2, "STDINT_REPR_HEX uint16: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint16 = (uint16)test_v}, STDINT_UINT16,
					buf, buf_size, STDINT_REPR_BIN);
	UART_puts(UART_ID_2, "STDINT_REPR_BIN uint16: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint16 = (uint16)test_v}, STDINT_UINT16,
					buf, buf_size, STDINT_REPR_OCT);
	UART_puts(UART_ID_2, "STDINT_REPR_OCT uint16: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	// ==========================
	// INT32
	// ==========================
	stdint_to_ascii((STDINT_UNION){.int32 = (int32)test_v}, STDINT_INT32, buf,
					buf_size, STDINT_REPR_DEC);
	UART_puts(UART_ID_2, "STDINT_REPR_DEC int32: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int32 = (int32)test_v}, STDINT_INT32, buf,
					buf_size, STDINT_REPR_HEX);
	UART_puts(UART_ID_2, "STDINT_REPR_HEX int32: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int32 = (int32)test_v}, STDINT_INT32, buf,
					buf_size, STDINT_REPR_BIN);
	UART_puts(UART_ID_2, "STDINT_REPR_BIN int32: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int32 = (int32)test_v}, STDINT_INT32, buf,
					buf_size, STDINT_REPR_OCT);
	UART_puts(UART_ID_2, "STDINT_REPR_OCT int32: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	// ==========================
	// UINT32
	// ==========================
	stdint_to_ascii((STDINT_UNION){.uint32 = (uint32)test_v}, STDINT_UINT32,
					buf, buf_size, STDINT_REPR_DEC);
	UART_puts(UART_ID_2, "STDINT_REPR_DEC uint32: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint32 = (uint32)test_v}, STDINT_UINT32,
					buf, buf_size, STDINT_REPR_HEX);
	UART_puts(UART_ID_2, "STDINT_REPR_HEX uint32: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint32 = (uint32)test_v}, STDINT_UINT32,
					buf, buf_size, STDINT_REPR_BIN);
	UART_puts(UART_ID_2, "STDINT_REPR_BIN uint32: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint32 = (uint32)test_v}, STDINT_UINT32,
					buf, buf_size, STDINT_REPR_OCT);
	UART_puts(UART_ID_2, "STDINT_REPR_OCT uint32: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	// ==========================
	// INT64
	// ==========================
	stdint_to_ascii((STDINT_UNION){.int64 = test_v}, STDINT_INT64, buf,
					buf_size, STDINT_REPR_DEC);
	UART_puts(UART_ID_2, "STDINT_REPR_DEC int64: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int64 = test_v}, STDINT_INT64, buf,
					buf_size, STDINT_REPR_HEX);
	UART_puts(UART_ID_2, "STDINT_REPR_HEX int64: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int64 = test_v}, STDINT_INT64, buf,
					buf_size, STDINT_REPR_BIN);
	UART_puts(UART_ID_2, "STDINT_REPR_BIN int64: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.int64 = test_v}, STDINT_INT64, buf,
					buf_size, STDINT_REPR_OCT);
	UART_puts(UART_ID_2, "STDINT_REPR_OCT int64: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	// ==========================
	// UINT64
	// ==========================
	stdint_to_ascii((STDINT_UNION){.uint64 = (uint64)test_v}, STDINT_UINT64,
					buf, buf_size, STDINT_REPR_DEC);
	UART_puts(UART_ID_2, "STDINT_REPR_DEC uint64: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint64 = (uint64)test_v}, STDINT_UINT64,
					buf, buf_size, STDINT_REPR_HEX);
	UART_puts(UART_ID_2, "STDINT_REPR_HEX uint64: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint64 = (uint64)test_v}, STDINT_UINT64,
					buf, buf_size, STDINT_REPR_BIN);
	UART_puts(UART_ID_2, "STDINT_REPR_BIN uint64: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");

	stdint_to_ascii((STDINT_UNION){.uint64 = (uint64)test_v}, STDINT_UINT64,
					buf, buf_size, STDINT_REPR_OCT);
	UART_puts(UART_ID_2, "STDINT_REPR_OCT uint64: ");
	UART_puts(UART_ID_2, buf);
	UART_puts(UART_ID_2, "\r\n");
}
*/