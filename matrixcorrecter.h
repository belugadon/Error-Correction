#ifndef MATRIXCORRECTER_H
#define MATRIXCORRECTER_H

struct CorrectionalCodes {
	int RowCorrectionalCode;
	int ColumnCorrectionalCode;
	int RowReceivedCode;
	int ColumnReceivedCode;
	bool parity;
	bool parity_recieved;
	int no_of_errors;
	int attempt_no;
};

struct burst_errors {
	int length;
	int line_no;
	int highest_line;
};

typedef struct CorrectionalCodes *ECC;

int build_data_packet(char data[]);
int check_data_packet(char data[]);
int error_check(struct CorrectionalCodes *ECC);
int calculate_CorrectionalCodes(struct CorrectionalCodes *ECC, bool data_matrix[4][4], int generator_array[4]);
int capture_correctional_codes(struct CorrectionalCodes *ECC, char rx_data[]);
void build_Matrices(char data[], bool data_matrix[][4], int generator_array[]);
int calculate_errors(struct CorrectionalCodes *ECC, int error_coordinates[][2], int generator_array[]);
int calculate_burst_errors(struct CorrectionalCodes *ECC, struct burst_errors *burst, int generator_array[]);
void correct_packet(bool data_matrix[][4], struct CorrectionalCodes *ECC, int error_coordinates[][2]);
int power(int base, int exponent);


#endif
