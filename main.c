#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define TOKEN_SPLIT " ,\n"
#define MEMORY_SIZE 65536 // 2^16

#define FFF_AND 0
#define FFF_OR 1
#define FFF_XOR 2
#define FFF_NOT 3
#define FFF_ADD 4
#define FFF_SUB 5
#define FFF_SHA 6
#define FFF_SHL 7

#define FFF_CMPLT 0
#define FFF_CMPLE 1
#define FFF_CMPEQ 3
#define FFF_CMPLTU 4
#define FFF_CMPLEU 5

static size_t current_line = 1;
static size_t PC = 0;

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
	printf("Error parsing (line %lu): ", current_line);
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void print_warning(const char* format, ...) {
	va_list args;
	printf("Warning (line %lu): ", current_line);
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

int16_t get_register_num(char* token) {
	return (int16_t)strtol(&token[1], NULL, 10);
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

int16_t format_in_out(int16_t reg, int8_t addr, int16_t is_out) {
	// OP | REG_DEST | IN/OUT | ADDR
	return (int16_t) ((10 << 12) | (reg << 9) | (is_out << 8) | addr);
}

int16_t format_mov(int16_t reg, int8_t value, int16_t is_movhi) {
	return (int16_t) ((9 << 12) | (reg << 9) | (is_movhi << 8) | value);
}


int16_t format_r3(int16_t dreg, int16_t areg, int16_t breg, int16_t f, int16_t is_boolean) {
	return (int16_t) ((is_boolean << 12) | (areg << 9) | (breg << 6) | (dreg << 3) | f);
}

void printf_info(int reg, int16_t cur_reg_val, int16_t old_reg_val, int16_t formatted, const char* format) {
	printf("R%d (0x%04X -> 0x%04X): ", reg , old_reg_val & 0xffff, cur_reg_val & 0xffff);
	printf_bits(format, formatted, 16);
	printf(" (%04X)\n", formatted & 0xffff);
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

			const int16_t reg = get_register_num(token);
			const int16_t old_reg_val = Registers[reg];
			// TODO: Make a system to have input and output?

			token = strtok(NULL, TOKEN_SPLIT);

			if (token == NULL) {
				print_error("found 'IN' token without a address.\n");
				break;
			}

			const int8_t addr = (int8_t)strtol(token, NULL, 10);
			const int16_t formatted = format_in_out(reg, addr, 0);

			printf_info(reg, Registers[reg], old_reg_val, formatted, "xxxx xxx x xxxxxxxx");
		}
		else if (strcmp(token, "OUT") == 0) {
			token = strtok(NULL, TOKEN_SPLIT);
			if (token == NULL) {
				print_error("found 'OUT' token without a address.\n");
				break;
			}
			const int8_t addr = (int8_t)strtol(token, NULL, 10);

			if(!get_register(&token, current_line))
				break;

			const int16_t reg = get_register_num(token);
			const int16_t old_reg_val = Registers[reg];

			const int16_t formatted = format_in_out(reg, addr, 1);

			printf_info(reg, Registers[reg], old_reg_val, formatted, "xxxx xxx x xxxxxxxx");
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
			int16_t old_value = Registers[reg];
			Registers[reg] = value;
			int16_t formatted = format_mov(reg, value, 0);
			printf_info(reg, Registers[reg], old_value, formatted, "xxxx xxx x xxxxxxxx");
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
			int16_t old_value = Registers[reg];
			Registers[reg] = (Registers[reg] & 0xFF) | (value << 8);
			int16_t formatted = format_mov(reg, value, 1);
			printf_info(reg, Registers[reg], old_value, formatted, "xxxx xxx x xxxxxxxx");
		}
		else if(!strcmp(token, "ADD")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = Registers[a] + Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_ADD, 0);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "SUB")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = Registers[a] - Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_SUB, 0);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "AND")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = Registers[a] & Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_AND, 0);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "OR")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = Registers[a] | Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_OR, 0);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "XOR")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = Registers[a] ^ Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_XOR, 0);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "NOT")) {
			int d, a;
			int16_t old_value = Registers[d];
			get_2reg(&token, &d, &a);
			Registers[d] = ~Registers[a];
			int16_t formatted = format_r3(d, a, 0, FFF_NOT, 0);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "SHA")) {
			int d, a, b;
			int16_t old_value = Registers[d];
			get_3reg(&token, &d, &a, &b);
			Registers[d] = Registers[a] * int_pow(2, (Registers[b] & 8) | (Registers[b] & 4) | (Registers[b] & 2) | (Registers[b] & 1));
			int16_t formatted = format_r3(d, a, b, FFF_SHA, 0);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "SHL")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			uint16_t regb = (uint16_t)Registers[b];
			Registers[d] = (uint16_t)Registers[a] * uint_pow(2, (regb & 8) | (regb & 4) | (regb & 2) | (regb & 1));
			int16_t formatted = format_r3(d, a, b, FFF_SHL, 0);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "CMPLT")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = Registers[a] < Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_CMPLT, 1);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "CMPLE")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = Registers[a] <= Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_CMPLE, 1);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "CMPEQ")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = Registers[a] == Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_CMPEQ, 1);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "CMPLTU")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = (uint16_t)Registers[a] < (uint16_t)Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_CMPLTU, 1);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}
		else if(!strcmp(token, "CMPLEU")) {
			int d, a, b;
			get_3reg(&token, &d, &a, &b);
			int16_t old_value = Registers[d];
			Registers[d] = (uint16_t)Registers[a] <= (uint16_t)Registers[b];
			int16_t formatted = format_r3(d, a, b, FFF_CMPLEU, 1);
			printf_info(d, Registers[d], old_value, formatted, "XXXX XXX XXX XXX XXX");
		}

		++PC;
		++current_line;
	}

	fclose(file);

	return 0;
}
