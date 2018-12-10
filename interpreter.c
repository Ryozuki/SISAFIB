void printf_info(int reg, int16_t cur_reg_val, int16_t old_reg_val, int16_t formatted, const char *format)
{
	printf("R%d (0x%04X -> 0x%04X): ", reg, old_reg_val & 0xffff, cur_reg_val & 0xffff);
	// printf_bits(format, formatted, 16);
	printf(" (%04X) (PC: %04X)\n", formatted & 0xffff, PC & 0xffff);
}