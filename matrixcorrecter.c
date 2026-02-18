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

int calculate_CorrectionalCodes(struct CorrectionalCodes *codes, bool data_matrix[4][4], int generator_array[4])
{
	codes->RowCorrectionalCode = 0;
	codes->ColumnCorrectionalCode = 0;
	int a, i;
	for(a=0;a<4;a++)
	{
		for(i=0;i<4;i++)
		{
			codes->RowCorrectionalCode = codes->RowCorrectionalCode + data_matrix[a][i] * generator_array[i];
		}
	}
	for(a=0;a<4;a++)
	{
		for(i=0;i<4;i++)
		{
			codes->ColumnCorrectionalCode = codes->ColumnCorrectionalCode + data_matrix[i][a] * generator_array[i];
		}
	}	
	return 0;	
}
void build_Matrices(char data[], bool data_matrix[][4], int generator_array[])
{
	int i;
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
int calculate_errors(struct CorrectionalCodes *codes, int error_coordinates[][2])
{
	printf("\nrow cal: %d", codes->RowCorrectionalCode);
	printf("\nrow rx: %d", codes->RowReceivedCode);
	printf("\ncol cal: %d", codes->ColumnCorrectionalCode);
	printf("\ncol rx: %d", codes->ColumnReceivedCode);
	double row_diff = fabs(codes->RowCorrectionalCode - codes->RowReceivedCode);
	double column_diff = fabs(codes->ColumnCorrectionalCode - codes->ColumnReceivedCode);
	printf("\nrow diff: %f", row_diff);
	printf("\ncolumn diff: %f", column_diff);
	printf("\n");
	if ((fmod(row_diff, 1) == 0) && (fmod(column_diff, 1) == 0))
	{
		printf("\nSingle bit error.\n");
		if((log2(row_diff) != '-inf')&&(log2(row_diff) != '-nan'))
		{
			error_coordinates[0][0] = log2(row_diff);
		} else {
			error_coordinates[0][0] = -1;
		}
		if((log2(column_diff) != '-inf')&&(log2(column_diff) != '-nan'))
		{ 
		error_coordinates[0][1] = log2(column_diff); 
		} else {
				error_coordinates[0][1] = -1;
		}
	}
		printf("\nerror coordinates: %d, %d\n", error_coordinates[0][0], error_coordinates[0][1]);
	return 0;
}
void correct_packet(bool data_matrix[][4], int error_coordinates[][2], int number_of_errors)
{
	int i;
	for(i=0;i<number_of_errors;i++)
	{
		data_matrix[error_coordinates[i][0]][error_coordinates[i][1]] = !data_matrix[error_coordinates[i][0]][error_coordinates[i][1]];
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
