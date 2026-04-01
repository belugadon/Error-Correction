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
	int i, a;
	int generator_array[4];
	bool data_matrix[4][4];
	struct CorrectionalCodes code;
	
	build_Matrices(data, data_matrix, generator_array);
	printf("Transmitted data:");
	printf("\n\r");
	for(i=0;i<4;i++)
	{
		for(a=0;a<4;a++)
		{
			printf("%d",data_matrix[i][a]);
		}
		printf("\n");
	}	
	
	calculate_CorrectionalCodes(&code, data_matrix, generator_array);
	
	data[2] = code.RowCorrectionalCode;
	data[3] = code.ColumnCorrectionalCode;
	//printf("Row correction code:\n");
	//for(i=0;i<8;i++)
	//{
	//	putchar(data[2] & (1u << i) ? '1' : '0');
	//}
	//printf("\n");
	if(code.parity){ data[2] = (data[2] | (1u << 7)); }
	//printf("Row correction code w parity:\n");
	//for(i=0;i<8;i++)
	//{
	//	putchar(data[2] & (1u << i) ? '1' : '0');
	//}
	//printf("\n");
	return 0 ;
}

int check_data_packet(char data[])
{
	int correction_attempt_no = 0;
	int generator_array[4];
	int dual_bit_error_coordinates[4][2];
	int single_bit_error_coordinates[1][2];
	bool data_matrix[4][4];
	bool temp_matrix[4][4];
	struct CorrectionalCodes code;
	struct burst_errors row_burst;
	int i, a;
	build_Matrices(data, data_matrix, generator_array);
	printf("Recieved data:");
	printf("\n\r");
	for(i=0;i<4;i++)
	{
		for(a=0;a<4;a++)
		{
			printf("%d",data_matrix[i][a]);
		}
		printf("\n");
	}
	calculate_CorrectionalCodes(&code, data_matrix, generator_array);
	capture_correctional_codes(&code, data);
	//code.RowReceivedCode = data[2];
	//code.ColumnReceivedCode = data[3];	
	calculate_single_bit_errors(&code, single_bit_error_coordinates, generator_array);
	calculate_dual_bit_errors(&code, dual_bit_error_coordinates, generator_array);
	calculate_burst_errors(&code, &row_burst, generator_array);
	code.attempt_no=1;
	while ((error_check(&code) != 0) && (correction_attempt_no < 4))//(calculate_errors(&code, multi_bit_error_coordinates, generator_array) != 0)
	{
		printf("Correction attempt #%d:\n	", correction_attempt_no+1);
		memcpy(temp_matrix, data_matrix,16*sizeof(bool));
		/*printf("\n\r");
		for(i=0;i<4;i++)
		{
			for(a=0;a<4;a++)
			{
				printf("%d",temp_matrix[i][a]);
			}
			printf("\n");
		}*/
		correct_packet(correction_attempt_no, temp_matrix, &code, single_bit_error_coordinates, dual_bit_error_coordinates, &row_burst);
		calculate_CorrectionalCodes(&code, temp_matrix, generator_array);
		correction_attempt_no++;
		//capture_correctional_codes(&code, data);
		/*if (error_check(&code) != 0)
		{
			printf("Attempt #%d failed.\n	", correction_attempt_no);
			memcpy(temp_matrix, data_matrix,16*sizeof(bool));
			correct_packet(correction_attempt_no, temp_matrix, &code, single_bit_error_coordinates, multi_bit_error_coordinates);
			code.attempt_no++;
			calculate_CorrectionalCodes(&code, temp_matrix, generator_array);
			//capture_correctional_codes(&code, data);
			if (error_check(&code) != 0)
			{
				printf("Attempt #2 failed - Aborting message recovery attemts.\n	");
			} else {printf("Attempt #2 Suceeded\n"); }
		} else { printf("Attempt #%d Suceeded\n", correction_attempt_no);}*/
	}
	if(error_check(&code) == 0)
	{
		memcpy(data_matrix, temp_matrix,16*sizeof(bool));
		repackage_data(data, data_matrix);
		printf("Attempt #%d suceeded\n", correction_attempt_no);
		printf("corrected data:");
		printf("\n\r");
		for(i=0;i<4;i++)
		{
			for(a=0;a<4;a++)
			{
				printf("%d",data_matrix[i][a]);
			}
			printf("\n");
		}
	} else {
		printf("Attempt #%d failed, Aborting\n", correction_attempt_no);
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
	//printf("\nrow cal: %d", ECC->RowCorrectionalCode);
	//printf("\nrow rx: %d", ECC->RowReceivedCode);
	//printf("\ncol cal: %d", ECC->ColumnCorrectionalCode);
	//printf("\ncol rx: %d", ECC->ColumnReceivedCode);
	return 0;	
}
int capture_correctional_codes(struct CorrectionalCodes *ECC, char rx_data[])
{
	int i;
	ECC->RowReceivedCode = rx_data[2];
	ECC->ColumnReceivedCode = rx_data[3];
	//printf("Received row correctional code w parity bit:\n");
	//for(i=0;i<8;i++)
	//{
	//	putchar(ECC->RowReceivedCode & (1u << i) ? '1' : '0');
	//}
	//printf("\n");
	ECC->parity_recieved = (ECC->RowReceivedCode & (1u << 7) ? 1 : 0);
	ECC->RowReceivedCode = (ECC->RowReceivedCode & ~(1u << 7));
	//printf("Received row correctional code w parity bit parsed:\n");
	//for(i=0;i<8;i++)
	//{
	//	putchar(ECC->RowReceivedCode & (1u << i) ? '1' : '0');
	//}
	//printf("\n");
		
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
	//printf("\nrow diff: %f", row_diff);
	//printf("\ncolumn diff: %f", column_diff);
	//printf("\n");
	if ((row_diff > 0) || (column_diff > 0))//if ((fmod(row_diff, 1) == 0) && (fmod(column_diff, 1) == 0))
	{
		printf("Error detected.\n");
		return 1;
		/*if ((fmod(log2(row_diff), 1) == 0) && (fmod(log2(column_diff), 1) == 0))
		{
			printf("\nSingle bit error.\n");
			return 1;
		} else 
		{
			printf("Multi bit error.\n");
			return 2;
		}*/
	} else {
		printf("No errors detected.\n");
		if(ECC->parity == ECC->parity_recieved){
			return 0;
		} else { 
			printf("\nParity Check failed.\n");
			return 2;
		}
	}
}
int calculate_single_bit_errors(struct CorrectionalCodes *ECC, int error_coordinates[][2], int generator_array[])
{	
	//printf("\nrow cal: %d", ECC->RowCorrectionalCode);
	//printf("\nrow rx: %d", ECC->RowReceivedCode);
	//printf("\ncol cal: %d", ECC->ColumnCorrectionalCode);
	//printf("\ncol rx: %d", ECC->ColumnReceivedCode);
	double row_diff = fabs(ECC->RowCorrectionalCode - ECC->RowReceivedCode);
	double column_diff = fabs(ECC->ColumnCorrectionalCode - ECC->ColumnReceivedCode);
	//printf("\nrow diff: %f", row_diff);
	//printf("\ncolumn diff: %f", column_diff);
	//printf("\n");
		//printf("\nError detected.\n");
	printf("Single bit error possibilities,\n");
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
	printf("coordinates:\n%d, %d\n", error_coordinates[0][0], error_coordinates[0][1]);
			
	return 0;

}
int calculate_dual_bit_errors(struct CorrectionalCodes *ECC, int error_coordinates[][2], int generator_array[])
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
	
	if ((fmod(log2(column_diff),1) != 0) && (fmod(log2(row_diff),1) == 0))
	{
		row_diff = row_diff/2;
	}
	if ((fmod(log2(row_diff),1) != 0) && (fmod(log2(column_diff),1) == 0))
	{
		column_diff = column_diff/2;
	}
	//printf("\nrow diff: %f", row_diff);
	//printf("\ncolumn diff: %f", column_diff);
	//printf("\n");
	printf("dual bit error possibilities.\n");
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
	printf("%d, %d\n", error_coordinates[3][0], error_coordinates[3][1]);
	
	return 0;
}
int calculate_burst_errors(struct CorrectionalCodes *ECC, struct burst_errors *burst, int generator_array[])
{
	int temp1[10];
	int temp2[10];
	float temp3[10];
	int temp4[10];
	int temp5[10];
	int i, a;
	double row_diff = fabs(ECC->RowCorrectionalCode - ECC->RowReceivedCode);
	double column_diff = fabs(ECC->ColumnCorrectionalCode - ECC->ColumnReceivedCode);

	burst->highest_line = 0;
	if ((row_diff > 0) || (column_diff > 0))
	{
		for(i=0;i<8;i++)
		{
			temp5[i] = 0;
			if((log2(column_diff)-i)>0)
			{
				temp1[i] = 1;
			}else {temp1[i] = 0;}
			temp2[i] = pow(2,i);
		}
		for(i=0;i<8;i++)
		{
			burst->highest_line = burst->highest_line + temp1[i];
		}
		burst->highest_line = burst->highest_line - 1;
		//printf("\nrow diff: %f", row_diff);
	    //printf("\ncolumn diff: %f", column_diff);
		for (i=0;i<8;i++)
		{
			temp3[i] = row_diff/temp2[i];
			if(fmod((row_diff/temp2[i]),1)==0)
			{
				temp3[i] = temp3[i];
				
			}else{temp3[i] = 0;}
			temp4[i] = temp3[i];
		}
		for (i=0;i<8;i++)
		{
			for (a=7;a>=0;a--)
			{
				if(temp1[a]==1)
				{
					temp3[i] = temp3[i] - 1;
					if (temp3[i]>=0)
					{
						temp5[i] = temp5[i] + temp2[a];
					}
				} else {}
			}			
		}
		for (i=7;i>=0;i--)
		{
			if (temp5[i] == column_diff)
			{
				burst->length = temp4[i];//burst->length + 1;
				break;
			}
		}
		if (row_diff == 0)
		{
		burst->line_no = -1;
		} else {burst->line_no = log2(row_diff/burst->length);}
		/*printf("\n");
		for(i=0;i<8;i++)
		{
			printf("%d,", temp1[i]);
		}
		printf("\n");
		for(i=0;i<8;i++)
		{
			printf("%d,", temp2[i]);
		}
		printf("\n");
		for(i=0;i<8;i++)
		{
			printf("%f,", temp3[i]);
		}
		printf("\n");
		for(i=0;i<8;i++)
		{
			printf("%d,", temp4[i]);
		}
		printf("\n");
				for(i=0;i<8;i++)
		{
			printf("%d,", temp5[i]);
		}
		printf("\n");*/
	}
	printf("burst length: %d\n", burst->length);
	printf("in row: %d\n", burst->line_no);
	printf("to column: %d\n", burst->highest_line);
	
	return 0;
}
void correct_packet(int attempt_no, bool matrix[][4], struct CorrectionalCodes *ECC, int single_error_coordinates[][2], int dual_error_coordinates[][2], struct burst_errors *burst)
{
	int i, a, b;
	int temp[10];
	int row_dim = 4;
	int col_dim = 4;
	if (attempt_no == 0){
		if (single_error_coordinates[0][0] == -1)
		{
			for (i=0;i<8;i++)
			{
				temp[i] = (ECC->ColumnReceivedCode & (1u << i) ? 1 : 0);
			}
			ECC->ColumnReceivedCode = 0;
			temp[single_error_coordinates[0][1]] = !temp[single_error_coordinates[0][1]];
			for (i=0;i<8;i++)
			{
				ECC->ColumnReceivedCode = ECC->ColumnReceivedCode + (temp[i]*pow(2,i));
			}
		} else if (single_error_coordinates[0][1] == -1)
		{
			printf("Received row correctional code:\n");
			for(i=0;i<8;i++)
			{
				putchar(ECC->RowReceivedCode & (1u << i) ? '1' : '0');
			}
			printf("\n");
			for (i=0;i<8;i++)
			{
				temp[i] = (ECC->RowReceivedCode & (1u << i) ? 1 : 0);
			}
			printf("temp buffer:\n");
			for(i=0;i<8;i++)
			{
				putchar(temp[i]+48);
			}
			printf("\n");
			ECC->RowReceivedCode = 0;
			temp[single_error_coordinates[0][0]] = !temp[single_error_coordinates[0][0]];
			printf("temp buffer corrected:\n");
			for(i=0;i<8;i++)
			{
				putchar(temp[i]+48);
			}
			printf("\n");
			for (i=0;i<8;i++)
			{
				ECC->RowReceivedCode = ECC->RowReceivedCode + (temp[i]*pow(2,i));
			}
			printf("Received row correctional code corrected:\n");
			for(i=0;i<8;i++)
			{
				putchar(ECC->RowReceivedCode & (1u << i) ? '1' : '0');
			}
			printf("\n");
		} else {
			if ((single_error_coordinates[0][0] < row_dim) && (single_error_coordinates[0][1] < col_dim))
			{
				matrix[single_error_coordinates[0][0]][single_error_coordinates[0][1]] = !matrix[single_error_coordinates[0][0]][single_error_coordinates[0][1]];
			}
		}
	} else if ((attempt_no == 1) || (attempt_no == 2)) {
		/*for(i=0;i<ECC->no_of_errors;i++)
		{
			if ((dual_error_coordinates[i][0] >= 0) && (dual_error_coordinates[i][1] == -1))
			{
				for(a=0;a<8;a++)
				{
					temp[a] = (ECC->RowReceivedCode & (1u << a) ? 1 : 0);
				}
				temp[dual_error_coordinates[i][0]] = !temp[dual_error_coordinates[i][0]];
				ECC->RowReceivedCode = 0;
				for(a=0;a<8;a++)
				{
					ECC->RowReceivedCode = ECC->RowReceivedCode + (temp[a]*power(2,a));
				}
				for(a=0;a<8;a++) temp[a] = 0;
			} else if ((dual_error_coordinates[i][0] == -1) && (dual_error_coordinates[i][1] >= 0))
			{
				for(a=0;a<8;a++)
				{
					temp[a] = (ECC->ColumnReceivedCode & (1u << a) ? 1 : 0);
				}
				temp[dual_error_coordinates[i][1]] = !temp[dual_error_coordinates[i][1]];
				ECC->ColumnReceivedCode = 0;
				for(a=0;a<8;a++)
				{
					ECC->ColumnReceivedCode = ECC->ColumnReceivedCode + (temp[a]*power(2,a));
				}
				for(a=0;a<8;a++) temp[a] = 0;
			} else {*/
			if (attempt_no == 1){
				if (((single_error_coordinates[0][0] < row_dim) && (single_error_coordinates[0][1] < col_dim)) && ((single_error_coordinates[1][0] < row_dim) && (single_error_coordinates[1][1] < col_dim)))
				{
					matrix[dual_error_coordinates[0][0]][dual_error_coordinates[0][1]] = !matrix[dual_error_coordinates[0][0]][dual_error_coordinates[0][1]];
					matrix[dual_error_coordinates[1][0]][dual_error_coordinates[1][1]] = !matrix[dual_error_coordinates[1][0]][dual_error_coordinates[1][1]];
				}
			} else if (attempt_no == 2){
				if (((single_error_coordinates[2][0] < row_dim) && (single_error_coordinates[2][1] < col_dim)) && ((single_error_coordinates[3][0] < row_dim) && (single_error_coordinates[3][1] < col_dim)))
				{
					matrix[dual_error_coordinates[2][0]][dual_error_coordinates[2][1]] = !matrix[dual_error_coordinates[2][0]][dual_error_coordinates[2][1]];
					matrix[dual_error_coordinates[3][0]][dual_error_coordinates[3][1]] = !matrix[dual_error_coordinates[3][0]][dual_error_coordinates[3][1]];
				}	
			}
			//}
		//}
	} else {
		if (burst->length <= row_dim)
		{
			for(i=(burst->length-1);i>=0;i--)
			{
				//printf("%d,",i);
				matrix[burst->line_no][burst->highest_line-i] = !matrix[burst->line_no][burst->highest_line-i];
			}
		}
	}
}

int repackage_data(char data[], bool data_matrix[][4])
{
	int i;
	data[0] = 0;
	data[1] = 0;
	for (i=0;i<4;i++)
	{
		data[0] = data[0] + (int)data_matrix[0][i]*power(2,i);
	}
	for (i=0;i<4;i++)
	{
		data[0] = data[0] + (int)data_matrix[1][i]*power(2,i+4);
	}	
		for (i=0;i<4;i++)
	{
		data[1] = data[1] + (int)data_matrix[2][i]*power(2,i);
	}
	for (i=0;i<4;i++)
	{
		data[1] = data[1] + (int)data_matrix[3][i]*power(2,i+4);
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
