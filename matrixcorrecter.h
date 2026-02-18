#ifndef MATRIXCORRECTER_H
#define MATRIXCORRECTER_H

struct CorrectionalCodes {
	int RowCorrectionalCode;
	int ColumnCorrectionalCode;
	int RowReceivedCode;
	int ColumnReceivedCode;
};

typedef struct CorrectionalCodes *codes;

int calculate_CorrectionalCodes(struct CorrectionalCodes *codes, bool data_matrix[4][4], int generator_array[4]);
void build_Matrices(char data[], bool data_matrix[][4], int generator_array[]);
int calculate_errors(struct CorrectionalCodes *codes, int error_coordinates[][2]);
void correct_packet(bool data_matrix[][4], int error_coordinates[][2], int number_of_errors);
int power(int base, int exponent);


#endif
