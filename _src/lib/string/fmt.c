#include <kernel/panic.h>
#include <lib/stdarg.h>
#include <lib/stdbool.h>
#include <lib/stdint.h>
#include <lib/string.h>

char uint8_to_ascii_char(uint8 n)
{
	ASSERT(n < 9, "uint8_to_ascii_char: requires the input number to be between 0 "
	       "and 9");

	return (char)(n + '0');
}

static const uint8 STDINT_BASE_REPR_CHARS_[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

char * stdint_to_ascii(STDINT_UNION n, STDINT_TYPES n_type, char *buf, uint64 buf_len,
		       STDINT_BASE_REPR repr)
{
	uint64 repr_len = 0;

	switch (repr) {
	case STDINT_BASE_REPR_DEC:
		break;
	case STDINT_BASE_REPR_HEX:
		repr_len = 2;
		break;
	case STDINT_BASE_REPR_BIN:
		repr_len = 2;
		break;
	case STDINT_BASE_REPR_OCT:
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
			value = (uint64) - signed_value;
		} else {
			value = (uint64)signed_value;
		}
	} else {
		value = unsigned_value;
	}

	if (value == 0 && !negative) {
		if (buf_len < 2 + repr_len)
			goto panic;
		int64 i = 0;
		switch (repr) {
		case STDINT_BASE_REPR_DEC:
			break;
		case STDINT_BASE_REPR_HEX:
			buf[i++] = '0';
			buf[i++] = 'x';
			break;
		case STDINT_BASE_REPR_BIN:
			buf[i++] = '0';
			buf[i++] = 'b';
			break;
		case STDINT_BASE_REPR_OCT:
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
		if (i + 1 + (uint64)negative + repr_len >= buf_len)
			goto panic;

		buf[i++] = STDINT_BASE_REPR_CHARS_[digit];
	}

	switch (repr) {
	case STDINT_BASE_REPR_DEC:
		break;
	case STDINT_BASE_REPR_HEX: // Values are reversed after
		buf[i++] = 'x';
		buf[i++] = '0';
		break;
	case STDINT_BASE_REPR_BIN:
		buf[i++] = 'b';
		buf[i++] = '0';
		break;
	case STDINT_BASE_REPR_OCT:
		buf[i++] = '0';
		break;
	}

	if (negative)
		buf[i++] = '-';
	buf[i] = '\0';

	// Reverse created string
	uint64 start = 0;
	uint64 end = i - 1;  // i -1 to not include the '\0'
	while (start < end) {
		char tmp = buf[start];
		buf[start] = buf[end];
		buf[end] = tmp;
		start++;
		end--;
	}

	return buf;

panic:
	PANIC("stdint_to_ascii: provided buffer is smaller than requied "
	      "for provided stdint");

	return (char *)0x0;
}


static inline void puts_(str_fmt_putc putc, void *args, const char *s)
{
	while (*s)
		putc(*s++, args);
}

void str_fmt_print(str_fmt_putc putc, void *args, const char *f, va_list ap)
{
	char buf[1024];

	while (*f) {
		if (*f != '%') {
			putc(*f++, args);
			continue;
		}

		f++;

		switch (*f++) {
		case '%':
			putc('%', args);
			break;

		case 'c':
			putc((char)va_arg(ap, int), args);
			break;

		case 's': {
			const char *str = va_arg(ap, const char *);
			if (!str)
				str = "(null)";
			puts_(putc, args, str);
			break;
		}

		case 'd':
			stdint_to_ascii((STDINT_UNION) { .int32 = va_arg(ap, int32) }, STDINT_INT32, buf,
					sizeof(buf), STDINT_BASE_REPR_DEC);
			puts_(putc, args, buf);
			break;

		case 'u':
			stdint_to_ascii((STDINT_UNION) { .uint32 = va_arg(ap, uint32) }, STDINT_UINT32, buf,
					sizeof(buf), STDINT_BASE_REPR_DEC);
			puts_(putc, args, buf);
			break;

		case 'x':
			stdint_to_ascii((STDINT_UNION) { .uint32 = va_arg(ap, uint32) }, STDINT_UINT32, buf,
					sizeof(buf), STDINT_BASE_REPR_HEX);
			puts_(putc, args, buf);
			break;

		case 'p':
			stdint_to_ascii((STDINT_UNION) { .uint64 = (uint64)va_arg(ap, void *) }, STDINT_UINT64,
					buf, sizeof(buf), STDINT_BASE_REPR_HEX);
			puts_(putc, args, buf);
			break;

		case 'b':
			stdint_to_ascii((STDINT_UNION) { .uint64 = (uint64)va_arg(ap, void *) }, STDINT_UINT64,
					buf, sizeof(buf), STDINT_BASE_REPR_BIN);
			puts_(putc, args, buf);
			break;

		default:
			putc('%', args);
			putc(f[-1], args);
			break;
		}
	}
}
