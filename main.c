#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define TOKEN_SPLIT " ,\n"
#define MEMORY_SIZE 65536 // 2^16

static size_t current_line = 1;
static size_t current_address = 0;

int16_t int_pow(int16_t x, int16_t y) {
	while(y--)
		x *= x;
	return x;
}

uint16_t uint_pow(uint16_t x, uint16_t y) {
	while(y--)
		x *= x;
	return x;
}

void print_error(const char* format, ...) {
	va_list args;
	printf("Error parsing (line %llu): ", current_line);
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void print_warning(const char* format, ...) {
	va_list args;
	printf("Warning (line %llu): ", current_line);
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

int is_valid_register(const char *token) {
	if (strlen(token) < 2 || token[0] != 'R')
		return 0;
	if (token[1] < '0' || token[1] > '7')
		return 0;
	return 1;
}

int get_register(char** token, size_t current_line) {
	*token = strtok(NULL, TOKEN_SPLIT);

	if (*token == NULL) {
		print_error("found 'IN' token without a register.\n");
		return 0;
	}

	if (!is_valid_register(*token)) {
		print_error("Invalid register '%s'.\n", *token);
		return 0;
	}
	return 1;
}

int get_register_num(char* token) {
	return strtol(&token[1], NULL, 10);
}

int get_3reg(char** token, int* reg_d, int* reg_a, int* reg_b) {
	if(!get_register(token, current_line))
		return 0;

	*reg_d = get_register_num(*token);

	if(!get_register(token, current_line))
		return 0;

	*reg_a = get_register_num(*token);

	if(!get_register(token, current_line))
		return 0;

	*reg_b = get_register_num(*token);
	return 1;
}

int get_2reg(char** token, int* reg_d, int* reg_a) {
	if(!get_register(token, current_line))
		return 0;

	*reg_d = get_register_num(*token);

	if(!get_register(token, current_line))
		return 0;

	*reg_a = get_register_num(*token);
	return 1;
}

/**
 * Pretty prints a binary in the SISA format
 * @param format Any char that is not a newline or a space will be filled with a bit.
 * @param num The integer to get the bits from.
 */
void printf_bits(const char *format, int num, int bits) {
	size_t len = strlen(format);

	--bits;

	for (int i = 0; i < len; ++i) {
		if (format[i] == '\n')
			printf("\n");
		else if (format[i] != ' ') {
			if (num & (1 << bits))
				printf("1");
			else
				printf("0");
			--bits;
		} else
			printf(" ");
	}
}

int main(int argc, const char *argv[]) {
	int16_t Registers[8] = {0};
	int8_t Memory[MEMORY_SIZE] = {0};

	FILE *file;
	if (argc < 2) {
		// Interpreter mode.
		file = stdin;
	} else {
		file = fopen(argv[1], "r");

		if (file == NULL) {
			printf("Error opening file: %s", argv[1]);
		}
	}

	while (1) {
		char line[100];
		int16_t bits;
		char *what = fgets(line, 100, file);

		if (what == NULL || feof(file))
			break;

		if (strcmp(line, "\n") == 0) {
			++current_line;
			continue;
		}

		char *token;

		token = strtok(line, TOKEN_SPLIT);

		if (strcmp(token, "IN") == 0) {
			if(!get_register(&token, current_line))
				break;

			const int reg = get_register_num(token);

			token = strtok(NULL, TOKEN_SPLIT);

			if (token == NULL) {
				print_error("found 'IN' token without a address.\n");
				break;
			}

			const int8_t addr = (int8_t)strtol(token, NULL, 10);
			const int16_t formatted = (10 << 12) | (reg << 9) | addr;

			printf_bits("xxxx xxx x xxxxxxxx\n", formatted, 16);
			printf("%X", formatted);
		}
		else if(!strcmp(token, "MOVI")) {
			if(!get_register(&token, current_line))
				break;

			const int reg = get_register_num(token);

			token = strtok(NULL, TOKEN_SPLIT);

			if (token == NULL) {
				print_error("found 'MOVI' token without a value.\n");
				break;
			}

			int val = strtol(token, NULL, 10);

			if(val > INT8_MAX || val < INT8_MIN)
				print_warning("Value passed to MOVI (%d) uses more capacity than the allowed for a 8 bit signed integer.\n", val);

			int8_t value = (int8_t)val;
			Registers[reg] = value;
			printf("R%d: 0x%04X\n", reg, Registers[reg]);
		}
		else if(!strcmp(token, "MOVHI")) {
			if(!get_register(&token, current_line))
				break;

			const int reg = get_register_num(token);

			token = strtok(NULL, TOKEN_SPLIT);

			if (token == NULL) {
				print_error("found 'MOVI' token without a value.\n");
				break;
			}

			int val = strtol(token, NULL, 10);

			if(val > INT8_MAX || val < INT8_MIN)
				print_warning("Value passed to MOVI (%d) uses more capacity than the allowed for a 8 bit signed integer.\n", val);

			int8_t value = (int8_t)val;

			// 0xff cleans the upper bits.
			Registers[reg] = (Registers[reg] & 0xFF) | (value << 8);
			printf("R%d: 0x%04X\n", reg, Registers[reg]);
		}
		else if(!strcmp(token, "ADD")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] + Registers[b];
			printf("R%d: 0x%04X\n", d, Registers[d]);
		}
		else if(!strcmp(token, "SUB")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] - Registers[b];
		}
		else if(!strcmp(token, "AND")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] & Registers[b];
		}
		else if(!strcmp(token, "OR")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] | Registers[b];
		}
		else if(!strcmp(token, "XOR")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] ^ Registers[b];
		}
		else if(!strcmp(token, "NOT")) {
			int d, a;
			get_2reg(&token, &d, &a);
			Registers[d] = ~Registers[a];
		}
		else if(!strcmp(token, "SHA")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] * int_pow(2, (Registers[b] & 8) | (Registers[b] & 4) | (Registers[b] & 2) | (Registers[b] & 1));
		}
		else if(!strcmp(token, "SHL")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			uint16_t regb = (uint16_t)Registers[b];
			Registers[d] = (uint16_t)Registers[a] * uint_pow(2, (regb & 8) | (regb & 4) | (regb & 2) | (regb & 1));
		}
		else if(!strcmp(token, "CMPLT")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] < Registers[b];
		}
		else if(!strcmp(token, "CMPLE")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] <= Registers[b];
		}
		else if(!strcmp(token, "CMPEQ")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] == Registers[b];
		}
		else if(!strcmp(token, "CMPLTU")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = (uint16_t)Registers[a] < (uint16_t)Registers[b];
		}
		else if(!strcmp(token, "CMPLEU")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			Registers[d] = (uint16_t)Registers[a] <= (uint16_t)Registers[b];
		}

		++current_address;
		++current_line;
	}

	fclose(file);

	return 0;
}
