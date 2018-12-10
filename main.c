#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#define TOKEN_SPLIT " ,\n"
#define MAX_PROGRAM_SIZE 5000
#define MAX_LINE_SIZE 256

#define FFF_AND 0u
#define FFF_OR 1u
#define FFF_XOR 2u
#define FFF_NOT 3u
#define FFF_ADD 4u
#define FFF_SUB 5u
#define FFF_SHA 6u
#define FFF_SHL 7u

#define FFF_CMPLT 0u
#define FFF_CMPLE 1u
#define FFF_CMPEQ 3u
#define FFF_CMPLTU 4u
#define FFF_CMPLEU 5u

#define PARSE_ERROR 0
#define PARSE_SUCCESS 1

static size_t current_line = 1;
static unsigned int PC = 0;

/*
 * TODO:
 * - Add labels support
 * - Add Comments Support
 * - More error checking and warnings.
 * - Add macro MOVE which uses both MOVI AND MOVHI only if needed.
 */

uint16_t get_fff(const char* token) {
	if(!strcmp(token, "ADD"))
		return FFF_ADD;
	if(!strcmp(token, "SUB"))
		return FFF_SUB;
	if(!strcmp(token, "OR"))
		return FFF_OR;
	if(!strcmp(token, "XOR"))
		return FFF_XOR;
	if(!strcmp(token, "NOT"))
		return FFF_NOT;
	if(!strcmp(token, "SHA"))
		return FFF_SHA;
	if(!strcmp(token, "SHL"))
		return FFF_SHL;
	if(!strcmp(token, "CMPLT"))
		return FFF_CMPLT;
	if(!strcmp(token, "CMPLTU"))
		return FFF_CMPLTU;
	if(!strcmp(token, "CMPLE"))
		return FFF_CMPLE;
	if(!strcmp(token, "CMPLEU"))
		return FFF_CMPLEU;
	if(!strcmp(token, "CMPEQ"))
		return FFF_CMPEQ;
	if(!strcmp(token, "AND"))
		return FFF_AND;
	return 0;
}

void print_error(const char *format, ...)
{
	va_list args;
	printf("Error parsing (line %lu): ", current_line);
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void print_warning(const char *format, ...)
{
	va_list args;
	printf("Warning (line %lu): ", current_line);
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

int is_valid_register(const char *token)
{
	if (strlen(token) < 2 || token[0] != 'R')
		return 0;
	if (token[1] < '0' || token[1] > '7')
		return 0;
	return 1;
}

int get_register(char **token)
{
	*token = strtok(NULL, TOKEN_SPLIT);

	if (*token == NULL)
	{
		print_error("found 'IN' token without a register.\n");
		return 0;
	}

	if (!is_valid_register(*token))
	{
		print_error("Invalid register '%s'.\n", *token);
		return 0;
	}
	return 1;
}

int16_t get_register_num(char *token)
{
	return (int16_t) strtol(&token[1], NULL, 10);
}

int get_3reg(char **token, int *reg_d, int *reg_a, int *reg_b)
{
	if (!get_register(token))
		return 0;

	*reg_d = get_register_num(*token);

	if (!get_register(token))
		return 0;

	*reg_a = get_register_num(*token);

	if (!get_register(token))
		return 0;

	*reg_b = get_register_num(*token);
	return 1;
}

int get_2reg(char **token, int *reg_d, int *reg_a)
{
	if (!get_register(token))
		return 0;

	*reg_d = get_register_num(*token);

	if (!get_register(token))
		return 0;

	*reg_a = get_register_num(*token);
	return 1;
}

/**
 * Pretty prints a binary in the SISA format
 * @param format Any char that is not a newline or a space will be filled with a bit.
 * @param num The integer to get the bits from.
 */
void printf_bits(FILE *fp, const char *format, int num, int bits)
{
	size_t len = strlen(format);

	--bits;

	for (int i = 0; i < len; ++i)
	{
		if (format[i] == '\n')
			fprintf(fp,"\n");
		else if (format[i] != ' ')
		{
			if (num & (1 << bits))
				fprintf(fp, "1");
			else
				fprintf(fp, "0");
			--bits;
		} else
			fprintf(fp, " ");
	}
}

uint16_t format_n8(int16_t opcode, int16_t reg, int8_t n8, int16_t bool)
{
	return (uint16_t) ((opcode << 12) | (reg << 9) | (bool << 8) | (n8 & 0xff));
}


uint16_t format_r3(uint16_t dreg, uint16_t areg, uint16_t breg, uint16_t f, uint16_t is_boolean)
{
	return (uint16_t) ((is_boolean << 12) | (areg << 9) | (breg << 6) | (dreg << 3) | (f & 0x7));
}

uint16_t format_n6(int16_t opcode, int16_t areg, int16_t dbreg, int16_t value)
{
	return (uint16_t) ((opcode << 12) | (areg << 9) | (dbreg << 6) | (value & 0x3F));
}

int compile_line(char *line, uint16_t *pOutOperation)
{
	char *token;
	token = strtok(line, TOKEN_SPLIT);

	if (strcmp(token, "IN") == 0)
	{
		if (!get_register(&token))
			return PARSE_ERROR;

		const int16_t reg = get_register_num(token);
		// TODO: Make a system to have input and output?

		token = strtok(NULL, TOKEN_SPLIT);

		if (token == NULL)
		{
			print_error("found 'IN' token without a address.\n");
			return PARSE_ERROR;
		}

		const int8_t addr = (int8_t) strtol(token, NULL, 10);
		const uint16_t formatted = format_n8(10, reg, addr, 0);
		*pOutOperation = formatted;
	}
	else if (strcmp(token, "OUT") == 0)
	{
		token = strtok(NULL, TOKEN_SPLIT);
		if (token == NULL)
		{
			print_error("found 'OUT' token without a address.\n");
			return PARSE_ERROR;
		}
		const int8_t addr = (int8_t) strtol(token, NULL, 10);

		if (!get_register(&token))
			return PARSE_ERROR;

		const int16_t reg = get_register_num(token);
		const uint16_t formatted = format_n8(10, reg, addr, 1);
		*pOutOperation = formatted;
	}
	else if (!strcmp(token, "MOVI"))
	{
		if (!get_register(&token))
			return PARSE_ERROR;

		const int reg = get_register_num(token);

		token = strtok(NULL, TOKEN_SPLIT);

		if (token == NULL)
		{
			print_error("found 'MOVI' token without a value.\n");
			return PARSE_ERROR;
		}

		int val = strtol(token, NULL, 10);

		if (val > INT8_MAX || val < INT8_MIN)
			print_warning("Value passed to MOVI (%d) uses more capacity than the allowed for a 8 bit signed integer.\n",
						  val);

		int8_t value = (int8_t) val;
		uint16_t formatted = format_n8(9, reg, value, 0);
		*pOutOperation = formatted;
	}
	else if (!strcmp(token, "MOVHI"))
	{
		if (!get_register(&token))
			return PARSE_ERROR;

		const int reg = get_register_num(token);

		token = strtok(NULL, TOKEN_SPLIT);

		if (token == NULL)
		{
			print_error("found 'MOVI' token without a value.\n");
			return PARSE_ERROR;
		}

		int val = strtol(token, NULL, 10);

		if (val > INT8_MAX || val < INT8_MIN)
			print_warning("Value passed to MOVI (%d) uses more capacity than the allowed for a 8 bit signed integer.\n",
						  val);

		int8_t value = (int8_t) val;

		uint16_t formatted = format_n8(9, reg, value, 1);
		*pOutOperation = formatted;
	}
	else if (!strcmp(token, "BZ") || !strcmp(token, "BNZ"))
	{
		const char* opname = token;
		if (!get_register(&token))
			return PARSE_ERROR;

		const int reg = get_register_num(token);

		token = strtok(NULL, TOKEN_SPLIT);

		if (token == NULL)
		{
			print_error("found '%s' token without a value.\n", opname);
			return PARSE_ERROR;
		}

		int val = strtol(token, NULL, 10);

		if (val > INT8_MAX || val < INT8_MIN)
			print_warning("Value passed to %s (%d) uses more capacity than the allowed for a 8 bit signed integer.\n",
						  opname,
						  val);

		int8_t value = (int8_t) val;
		uint16_t formatted = format_n8(8, reg, value, (!strcmp(opname, "BZ")) ? 0 : 1);
		*pOutOperation = formatted;
	}
	else if (!strcmp(token, "ADD") || !strcmp(token, "SUB") || !strcmp(token, "AND")
	|| !strcmp(token, "OR") || !strcmp(token, "XOR") || !strcmp(token, "SHA")
	|| !strcmp(token, "SHL") || !strcmp(token, "CMPLT") || !strcmp(token, "CMPLE") || !strcmp(token, "CMPEQ")
	|| !strcmp(token, "CMPLTU") || !strcmp(token, "CMPLEU"))
	{
		int d, a, b;
		int is_bool = !strcmp(token, "CMPLT") || !strcmp(token, "CMPLE") || !strcmp(token, "CMPEQ")
					  || !strcmp(token, "CMPLTU") || !strcmp(token, "CMPLEU");
		uint16_t f = get_fff(token);
		get_3reg(&token, &d, &a, &b);
		uint16_t formatted = format_r3(d, a, b, f, is_bool);
		*pOutOperation = formatted;
	}
	else if(!strcmp(token, "NOT"))
	{
		int d, a, b;
		get_2reg(&token, &d, &a);
		uint16_t formatted = format_r3(d, a, 0, get_fff(token), 0);
		*pOutOperation = formatted;
	}
	else if (!strcmp(token, "LD") || !strcmp(token, "LDB"))
	{
		const char* name = token;
		if (!get_register(&token))
			return PARSE_ERROR;
		const int regd = get_register_num(token);

		token = strtok(NULL, TOKEN_SPLIT);

		if (token == NULL)
		{
			print_error("found %s mnemonic without a Address(Reg)\n", name);
			return PARSE_ERROR;
		}
		char *pEnd;
		int value = strtol(token, &pEnd, 10);

		if(*(pEnd) != '(') {
			print_error("missing (\n");
			return PARSE_ERROR;
		}

		if(*(pEnd + 1) != 'R') {
			print_error("missing register after address\n");
			return PARSE_ERROR;
		}

		int rega = strtol(&pEnd[2], &pEnd, 10);
		uint16_t formatted = format_n6((!strcmp(name, "LD")) ? 3 : 5, rega, regd, value);
		*pOutOperation = formatted;
	}
	else if (!strcmp(token, "ST") || !strcmp(token, "STB"))
	{
		const char* name = token;
		token = strtok(NULL, TOKEN_SPLIT);

		if (token == NULL)
		{
			print_error("found %s mnemonic without a Address(Reg)\n", name);
			return PARSE_ERROR;
		}
		char *pEnd;
		int value = strtol(token, &pEnd, 10);

		if(*(pEnd) != '(') {
			print_error("missing (\n");
			return PARSE_ERROR;
		}

		if(*(pEnd + 1) != 'R') {
			print_error("missing register after address\n");
			return PARSE_ERROR;
		}

		int rega = strtol(&pEnd[2], &pEnd, 10);
		// TODO: ADD check for this register integer 0 <= x < 8

		if (!get_register(&token))
			return PARSE_ERROR;
		const int regd = get_register_num(token);

		uint16_t formatted = format_n6((!strcmp(name, "ST")) ? 4 : 6, rega, regd, value);
		*pOutOperation = formatted;
	}
	else if (!strcmp(token, "JALR"))
	{
		int d, a;
		get_2reg(&token, &d, &a);
		uint16_t formatted = format_n6(7, a, d, 0);
		*pOutOperation = formatted;
	}
	else if (!strcmp(token, "ADDI"))
	{
		int d, a;
		const char* opname = token;
		get_2reg(&token, &d, &a);

		token = strtok(NULL, TOKEN_SPLIT);

		if (token == NULL)
		{
			print_error("found '%s' token without a value.\n", opname);
			return PARSE_ERROR;
		}

		int val = strtol(token, NULL, 10);

		if (val > 63 || val < -31)
			print_warning("Value passed to %s (%d) uses more capacity than the allowed for a 6 bit signed integer.\n",
						  opname,
						  val);

		int8_t value = (int8_t) val;

		uint16_t formatted = format_n6(2, a, d, value);
		*pOutOperation = formatted;
	}
	else
	{
		print_error("Unknown token: %s", token);
		return PARSE_ERROR;
	}
	return PARSE_SUCCESS;
}

int main(int argc, const char *argv[])
{
	uint8_t Instructions[MAX_PROGRAM_SIZE];
	FILE *file;
	if (argc < 2)
	{
		printf("You must pass a file.");
		return 0;
	} else
	{
		file = fopen(argv[1], "r");

		if (file == NULL)
		{
			printf("Error opening file: %s", argv[1]);
		}
	}

	while (1)
	{
		char line[MAX_LINE_SIZE];
		int16_t bits;
		char *what = fgets(line, MAX_LINE_SIZE, file);
		if (what == NULL)
			break;
		if (strcmp(line, "\n") == 0)
		{
			++current_line;
			continue;
		}

		int16_t operation = 0;

		if(compile_line(line, &operation) == PARSE_ERROR)
			break;

		//printf_bits(stdout, "xxxxxxxxxxxxxxxx\n", operation 16);
		Instructions[PC] = (operation & 0xff); // lower 8 bits
		Instructions[PC + 1] = ((operation >> 8) & 0xff);
		PC+=2;
		++current_line;

		if(feof(file))
			break;
	}
	fclose(file);

	FILE *fp;
	fp = fopen("output.txt", "w");
	for(int i = 0; i < PC; i+=2) {
		printf_bits(fp, "xxxxxxxxxxxxxxxx\n", (Instructions[i + 1] << 8) | Instructions[i], 16);
	}

	return 0;
}
