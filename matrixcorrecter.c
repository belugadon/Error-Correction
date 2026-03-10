#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <math.h>
#include "matrixcorrecter.h"

int build_data_packet(char data[])
{
	int i;
	int generator_array[4];
	bool data_matrix[4][4];
	struct CorrectionalCodes code;
	
	build_Matrices(data, data_matrix, generator_array);
	
	calculate_CorrectionalCodes(&code, data_matrix, generator_array);
	
	data[2] = code.RowCorrectionalCode;
	data[3] = code.ColumnCorrectionalCode;
	printf("Row correction code:\n");
	for(i=0;i<8;i++)
	{
		putchar(data[2] & (1u << i) ? '1' : '0');
	}
	printf("\n");
	if(code.parity){ data[2] = (data[2] | (1u << 7)); }
	printf("Row correction code w parity:\n");
	for(i=0;i<8;i++)
	{
		putchar(data[2] & (1u << i) ? '1' : '0');
	}
	printf("\n");
	return 0 ;
}

int check_data_packet(char data[])
{
	int generator_array[4];
	int error_coordinates[10][2];
	bool data_matrix[4][4];
	bool temp_matrix[4][4];
	struct CorrectionalCodes code;
	
	build_Matrices(data, data_matrix, generator_array);
	calculate_CorrectionalCodes(&code, data_matrix, generator_array);
	capture_correctional_codes(&code, data);
	//code.RowReceivedCode = data[2];
	//code.ColumnReceivedCode = data[3];	
	if (calculate_errors(&code, error_coordinates, generator_array) != 0)
	{
		printf("Correction attempt #1:\n	");
		code.attempt_no=0;
		memcpy(temp_matrix, data_matrix,16*sizeof(bool));
		correct_packet(temp_matrix, &code, error_coordinates);
		calculate_CorrectionalCodes(&code, temp_matrix, generator_array);
		capture_correctional_codes(&code, data);
		//code.RowReceivedCode = data[2];
		//code.ColumnReceivedCode = data[3];	
		if (error_check(&code) != 0)
		{
			printf("Attempt #1 failed.\nCorrection attempt #2:\n	");
			code.attempt_no=1;
			memcpy(temp_matrix, data_matrix,16*sizeof(bool));
			correct_packet(temp_matrix, &code, error_coordinates);
			calculate_CorrectionalCodes(&code, temp_matrix, generator_array);
			capture_correctional_codes(&code, data);
			//code.RowReceivedCode = data[2];
			//code.ColumnReceivedCode = data[3];	
			if (error_check(&code) != 0)
			{
				printf("Attempt #2 failed - Aborting message recovery attemts.\n	");
			} else {printf("Attempt #2 Suceeded\n"); }
		} else { printf("Attempt #1 Suceeded\n");}
	}
	return 0;
}
int calculate_CorrectionalCodes(struct CorrectionalCodes *ECC, bool data_matrix[4][4], int generator_array[4])
{
	ECC->RowCorrectionalCode = 0;
	ECC->ColumnCorrectionalCode = 0;
	int temp = 0;
	int a, i;
	for(a=0;a<4;a++)
	{
		for(i=0;i<4;i++)
		{
			ECC->ColumnCorrectionalCode = ECC->ColumnCorrectionalCode + data_matrix[a][i] * generator_array[i];
		}
	}
	for(a=0;a<4;a++)
	{
		for(i=0;i<4;i++)
		{
			ECC->RowCorrectionalCode = ECC->RowCorrectionalCode + data_matrix[i][a] * generator_array[i];
			temp = temp + data_matrix[a][i];
		}
	}
	ECC->parity = fmod(temp,2);	

	return 0;	
}
int capture_correctional_codes(struct CorrectionalCodes *ECC, char rx_data[])
{
	int i;
	ECC->RowReceivedCode = rx_data[2];
	ECC->ColumnReceivedCode = rx_data[3];
	printf("Received row correctional code w parity bit:\n");
	for(i=0;i<8;i++)
	{
		putchar(ECC->RowReceivedCode & (1u << i) ? '1' : '0');
	}
	printf("\n");
	ECC->parity_recieved = (ECC->RowReceivedCode & (1u << 7) ? 1 : 0);
	ECC->RowReceivedCode = (ECC->RowReceivedCode & ~(1u << 7));
	printf("Received row correctional code w parity bit parsed:\n");
	for(i=0;i<8;i++)
	{
		putchar(ECC->RowReceivedCode & (1u << i) ? '1' : '0');
	}
	printf("\n");
		
	return 0;
}
void build_Matrices(char data[], bool data_matrix[][4], int generator_array[])
{
	int i;
	for(i=0;i<10;i++)
	{
		generator_array[i] = 0;
	}
	for(i=0;i<4;i++)
	{
		generator_array[i] = power(2,(i));
	}
	for(i=0;i<4;i++)
	{
		data_matrix[0][i] = (data[0] & (1u << i) ? 1 : 0);
		data_matrix[1][i] = (data[0] & (1u << (i + 4)) ? 1 : 0);
		data_matrix[2][i] = (data[1] & (1u << i) ? 1 : 0);
		data_matrix[3][i] = (data[1] & (1u << (i + 4)) ? 1 : 0);			
	}
	
}
int error_check(struct CorrectionalCodes *ECC)
{
	double row_diff = fabs(ECC->RowCorrectionalCode - ECC->RowReceivedCode);
	double column_diff = fabs(ECC->ColumnCorrectionalCode - ECC->ColumnReceivedCode);
	printf("\nrow diff: %f", row_diff);
	printf("\ncolumn diff: %f", column_diff);
	printf("\n");
	if ((row_diff > 0) || (column_diff > 0))//if ((fmod(row_diff, 1) == 0) && (fmod(column_diff, 1) == 0))
	{
		printf("\nError detected.\n");
		if ((fmod(log2(row_diff), 1) == 0) && (fmod(log2(column_diff), 1) == 0))
		{
			printf("\nSingle bit error.\n");
			return 1;
		} else 
		{
			printf("Multi bit error.\n");
			return 2;
		}
	} else {
		printf("\nNo errors detected.\n\n");
		if(ECC->parity == ECC->parity_recieved){
			return 0;
		} else { 
			printf("\nParity Check failed.\n\n");
			return 3;
		}
	}
}
int calculate_errors(struct CorrectionalCodes *ECC, int error_coordinates[][2], int generator_array[])
{
	int temp1[10];
	int temp2[10];
	int i;
	int row1 = 0;
	int row2 = 0;
	int col1 = 0;
	int col2 = 0;
	
	//printf("\nrow cal: %d", ECC->RowCorrectionalCode);
	//printf("\nrow rx: %d", ECC->RowReceivedCode);
	//printf("\ncol cal: %d", ECC->ColumnCorrectionalCode);
	//printf("\ncol rx: %d", ECC->ColumnReceivedCode);
	double row_diff = fabs(ECC->RowCorrectionalCode - ECC->RowReceivedCode);
	double column_diff = fabs(ECC->ColumnCorrectionalCode - ECC->ColumnReceivedCode);
	//printf("\nrow diff: %f", row_diff);
	//printf("\ncolumn diff: %f", column_diff);
	//printf("\n");
	if ((row_diff > 0) || (column_diff > 0))//if ((fmod(row_diff, 1) == 0) && (fmod(column_diff, 1) == 0))
	{
		//printf("\nError detected.\n");
		if ((fmod(log2(row_diff), 1) == 0) && (fmod(log2(column_diff), 1) == 0))
		{
			printf("\nSingle bit error.\n");
			if(row_diff != 0)
			{
				error_coordinates[0][0] = log2(row_diff);
			} else {
				error_coordinates[0][0] = -1;
			}
			if(column_diff != 0)
			{ 
				error_coordinates[0][1] = log2(column_diff); 
			} else {
				error_coordinates[0][1] = -1;
			}
			ECC->no_of_errors = 1;
			printf("Error coordinates:\n%d, %d\n", error_coordinates[0][0], error_coordinates[0][1]);
			return 1;
		} else 
		{
			printf("Multi bit error.\n");
			//row
			for (i=0;i<10;i++)
			{
				temp1[i] = 0;
				temp2[i] = 0;
			}
				
			for (i=0;i<4;i++)
			{
				//printf("row diff: %f, gen array: %d\n", row_diff, generator_array[i]);
				if(row_diff >= generator_array[i])
				{
					temp1[i] = 1;
				}else {temp1[i] = 0;}
			}	
			for (i=0;i<4;i++)
			{
				if(fmod(row_diff,generator_array[i])==0)
				{
					temp2[i] = 1;
				}else {temp2[i] = 0;}
			}	
			for (i=0;i<4;i++)
			{
				row1 = row1 + temp1[i];
				row2 = row2 + temp2[i];
			}
			error_coordinates[0][0] = row1 - 1;
			error_coordinates[1][0] = row2 - 1;
			error_coordinates[2][0] = row2 - 1;
			error_coordinates[3][0] = row1 - 1;
			ECC->no_of_errors = 2;
			//column
			for (i=0;i<10;i++)
			{
				temp1[i] = 0;
				temp2[i] = 0;
			}		
			for (i=0;i<4;i++)
			{
				if(column_diff >= generator_array[i])
				{
					temp1[i] = 1;
				}else {temp1[i] = 0;}
			}	
			for (i=0;i<4;i++)
			{
				if(fmod(column_diff,generator_array[i])==0)
				{
					temp2[i] = 1;
				}else {temp2[i] = 0;}
			}	
			for (i=0;i<4;i++)
			{
				col1 = col1 + temp1[i];
				col2 = col2 + temp2[i];
			}
			error_coordinates[0][1] = col1 - 1;
			error_coordinates[1][1] = col2 - 1;
			error_coordinates[2][1] = col1 - 1;
			error_coordinates[3][1] = col2 - 1;
			ECC->no_of_errors = 2;
			printf("Error coordinates\nGuess#1:\n");			
			printf("%d, %d\n", error_coordinates[0][0], error_coordinates[0][1]);
			printf("%d, %d\n", error_coordinates[1][0], error_coordinates[1][1]);
			printf("Guess#2:\n");			
			printf("%d, %d\n", error_coordinates[2][0], error_coordinates[2][1]);
			printf("%d, %d\n\n", error_coordinates[3][0], error_coordinates[3][1]);
			return 2;
		}
	} else {
		printf("\nNo errors detected.\n\n");
		if(ECC->parity == ECC->parity_recieved){
			return 0;
		} else { 
			printf("\nParity Check failed.\n\n");
			return 3;
		}
		}
}
int calculate_burst_errors(struct CorrectionalCodes *ECC, struct burst_errors *burst, int generator_array[])
{
	int temp1[10];
	int temp2[10];
	int i;
	double row_diff = fabs(ECC->RowCorrectionalCode - ECC->RowReceivedCode);
	double column_diff = fabs(ECC->ColumnCorrectionalCode - ECC->ColumnReceivedCode);

	burst->highest_line = 0;
	if ((row_diff > 0) || (column_diff > 0))
	{
		for(i=0;i<10;i++)
		{
			if((log2(column_diff)-i)>0)
			{
				temp1[i] = 1;
			}else {temp1[i] = 0;}
		}
		for(i=0;i<10;i++)
		{
			burst->highest_line = burst->highest_line + temp1[i];
		}
		for (i=0;i<10;i++)
		{
			if(fmod(row_diff/power(i,2),1)==0)
			{
				temp2[i] = 1;
			}else{temp2[i] = 0;}
		}
	}
	return 0;
}
void correct_packet(bool data_matrix[][4], struct CorrectionalCodes *ECC, int error_coordinates[][2])
{
	int i, a;
	int temp[10];
	for(i=0;i<ECC->no_of_errors;i++)
	{
		if((error_coordinates[i+ECC->attempt_no*2][0] >= 0) && (error_coordinates[i+ECC->attempt_no*2][1] >= 0))
		{
			data_matrix[error_coordinates[i+ECC->attempt_no*2][0]][error_coordinates[i+ECC->attempt_no*2][1]] = !data_matrix[error_coordinates[i+ECC->attempt_no*2][0]][error_coordinates[i+ECC->attempt_no*2][1]];
		} else if ((error_coordinates[i][0] >= 0) && (error_coordinates[i][1] == -1))
		{
			for(a=0;a<8;a++)
			{
				temp[a] = (ECC->RowReceivedCode & (1u << a) ? 1 : 0);
			}
			temp[error_coordinates[i][0]] = !temp[error_coordinates[i][0]];
			ECC->RowReceivedCode = 0;
			for(a=0;a<8;a++)
			{
				ECC->RowReceivedCode = ECC->RowReceivedCode + (temp[a]*power(2,a));
			}
			for(a=0;a<8;a++) temp[a] = 0;
		} else if ((error_coordinates[i][0] == -1) && (error_coordinates[i][1] >= 0))
		{
			for(a=0;a<8;a++)
			{
				temp[a] = (ECC->ColumnReceivedCode & (1u << a) ? 1 : 0);
			}
			temp[error_coordinates[i][1]] = !temp[error_coordinates[i][1]];
			ECC->ColumnReceivedCode = 0;
			for(a=0;a<8;a++)
			{
				ECC->ColumnReceivedCode = ECC->ColumnReceivedCode + (temp[a]*power(2,a));
			}
			for(a=0;a<8;a++) temp[a] = 0;
		}
	}
}

int power(int base, int exponent)
{
	int i, result;
	if(exponent == 0)
	{
		result = 1;
	} else {
		result = base;
		for(i=0;i<exponent-1;i++)
		{
			result = result * base;
		}
	}
	return result;
}
