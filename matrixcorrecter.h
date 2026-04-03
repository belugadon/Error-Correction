#ifndef MATRIXCORRECTER_H
#define MATRIXCORRECTER_H
struct data {
	char data_array[8];
	bool data_matrix[5][8];
	//bool temp_matrix[5][8];
	unsigned int generator_array[8];
};
struct CorrectionalCodes {
	int RowCorrectionalCode;
	int ColumnCorrectionalCode;
	int RowReceivedCode;
	int ColumnReceivedCode;
	int temp_rowcode_low;
	int temp_rowcode_high;
	bool parity;
	bool parity_recieved;
	int no_of_errors;
	int attempt_no;
};

struct errors {
	float single_error_coordinates[3][2];
	int dual_error_coordinates[4][2];
	unsigned int row_length;
	unsigned int row_line_no;
	unsigned int row_highest_line;
	unsigned int col_length;
	unsigned int col_line_no;
	unsigned int col_highest_line;
};


typedef struct CorrectionalCodes *ECC;
typedef struct data *transaction_data;
typedef struct errors *transaction_errors;

int build_data_packet(char send_data[]);
int check_data_packet(char receive_data[]);
int error_check(struct CorrectionalCodes *ECC);
int calculate_CorrectionalCodes(struct CorrectionalCodes *ECC, struct data *transaction_data);
int capture_correctional_codes(struct CorrectionalCodes *ECC, struct data *transaction_data);
void build_Matrices(struct data *transaction_data);
int calculate_single_bit_errors(struct CorrectionalCodes *ECC, struct errors *transaction_errors, struct data *transaction_data);
int calculate_dual_bit_errors(struct CorrectionalCodes *ECC, struct errors *transaction_errors, struct data *transaction_data);
int calculate_burst_errors(struct CorrectionalCodes *ECC, struct errors *transaction_errors, struct data *transaction_data);
void correct_packet(int attempt_no, struct data *transaction_data, struct CorrectionalCodes *ECC, struct errors *transaction_errors);
int repackage_data(struct data *transaction_data);
int power(int base, int exponent);
int copy_structure(struct data *source, struct data *destination);

#endif
