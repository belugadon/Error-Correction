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

int build_data_packet(char send_data[])
{
	int i, a;
	//int generator_array[5];
	//bool data_matrix[5][5];
	struct CorrectionalCodes code;
	struct data tx_data;
	
	for (i=0;i<8;i++){ tx_data.data_array[i] = send_data[i];}
	
	build_Matrices(&tx_data);
	printf("Transmitted data:");
	printf("\n\r");
	for(i=0;i<5;i++)
	{
		for(a=0;a<8;a++)
		{
			printf("%d",tx_data.data_matrix[i][a]);
		}
		printf("\n");
	}	
	
	calculate_CorrectionalCodes(&code, &tx_data);
	
	tx_data.data_array[5] = code.RowCorrectionalCode;
	tx_data.data_array[6] = code.ColumnCorrectionalCode & 0xff;
	tx_data.data_array[7] = (code.ColumnCorrectionalCode>>8) & 0xff;

	//if(code.parity){ tx_data.data_array[7] = (tx_data.data_array[7] | (1u << 7)); }
	tx_data.data_array[7] = (tx_data.data_array[7] | (code.parity << 7));
	for(a=0;a<8;a++)
	{
		send_data[a] = tx_data.data_array[a]; 
	}
	printf("\nTransmit data array:\n");
	for(a=0;a<8;a++)
	{
		printf("%d,", tx_data.data_array[a]);
	}
	printf("\n");
	//printf("Row correction code w parity:\n");
	//for(i=0;i<8;i++)
	//{
	//	putchar(data[2] & (1u << i) ? '1' : '0');
	//}
	//printf("\n");
	return 0 ;
}

int check_data_packet(char receive_data[])
{
	int correction_attempt_no = 0;
	int dual_bit_error_coordinates[4][2];
	float single_bit_error_coordinates[1][2];
	struct CorrectionalCodes code;
	struct data rx_data, temp_data;
	struct errors rx_errors;
	//struct burst_errors row_burst;
	int i, a;
	printf("Recieved data:");
	printf("\n\r");
	for(a=0;a<8;a++)
	{
		printf("%d,", receive_data[a]);//rx_data.data_array[a]);
	}
	for (i=0;i<8;i++){ rx_data.data_array[i] = receive_data[i];}
	build_Matrices(&rx_data);

	printf("\n");
	for(i=0;i<5;i++)
	{
		for(a=0;a<8;a++)
		{
			printf("%d", rx_data.data_matrix[i][a]);
		}
		printf("\n");
	}

	calculate_CorrectionalCodes(&code, &rx_data);
	capture_correctional_codes(&code, &rx_data);
	//code.RowReceivedCode = data[2];
	//code.ColumnReceivedCode = data[3];	
	calculate_single_bit_errors(&code, &rx_errors, &rx_data);
	calculate_dual_bit_errors(&code, &rx_errors, &rx_data);
	calculate_burst_errors(&code, &rx_errors, &rx_data);
	code.attempt_no=1;
	//memcpy(temp_data.data_matrix, rx_data.data_matrix,16*sizeof(bool));
	copy_structure(&rx_data, &temp_data);
	for(i=0;i<5;i++)
	{
		for(a=0;a<8;a++)
		{
			//temp_data.data_matrix[i][a] = rx_data.data_matrix[i][a];
			//printf("%d",temp_data.data_matrix[i][a]);
		}
		//printf("\n");
	}
	while ((error_check(&code) != 0) && (correction_attempt_no < 5))//(calculate_errors(&code, multi_bit_error_coordinates, generator_array) != 0)
	{
		//printf("Correction attempt #%d:\n	", correction_attempt_no+1);
		//memcpy(temp_data.data_matrix, rx_data.data_matrix,16*sizeof(bool));
		copy_structure(&rx_data, &temp_data);
		for(i=0;i<5;i++)
		{
			for(a=0;a<8;a++)
			{
				//temp_data.data_matrix[i][a] = rx_data.data_matrix[i][a];
				//printf("%d",temp_data.data_matrix[i][a]);
			}
			//printf("\n");
		}
		correct_packet(correction_attempt_no, &temp_data, &code, &rx_errors);
		calculate_CorrectionalCodes(&code, &temp_data);
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
		//memcpy(rx_data.data_matrix, temp_data.data_matrix,16*sizeof(bool));
		copy_structure(&temp_data, &rx_data);
		for(i=0;i<5;i++)
		{
			for(a=0;a<8;a++)
			{
				//rx_data.data_matrix[i][a] = temp_data.data_matrix[i][a];
				//printf("%d",temp_data.data_matrix[i][a]);
			}
			//printf("\n");
		}
		repackage_data(&rx_data);
		printf("Attempt #%d suceeded\n", correction_attempt_no);
		printf("corrected data:");
		printf("\n\r");
		for(i=0;i<5;i++)
		{
			for(a=0;a<8;a++)
			{
				printf("%d", rx_data.data_matrix[i][a]);
			}
			printf("\n");
		}
		for(i=0;i<8;i++)
		{
			receive_data[i] = rx_data.data_array[i];
		}
	} else {
		printf("Attempt #%d failed, Aborting\n", correction_attempt_no);
	}
	return 0;
}
int calculate_CorrectionalCodes(struct CorrectionalCodes *ECC, struct data *transaction_data)
{
	ECC->RowCorrectionalCode = 0;
	ECC->ColumnCorrectionalCode = 0;
	int temp = 0;
	int a, i;
	for(a=0;a<8;a++)
	{
		for(i=0;i<5;i++)
		{
			ECC->ColumnCorrectionalCode = ECC->ColumnCorrectionalCode+(int)transaction_data->data_matrix[i][a]*(int)transaction_data->generator_array[a];
		}
	}
	for(a=0;a<8;a++)
	{
		for(i=0;i<5;i++)
		{
			ECC->RowCorrectionalCode = ECC->RowCorrectionalCode+transaction_data->data_matrix[i][a]*transaction_data->generator_array[i];
			temp = temp + transaction_data->data_matrix[i][a];
			//printf("matrix value: %d,data sum: %d\n", transaction_data->data_matrix[i][a], temp);
		}
	}
	
	ECC->parity = fmod(temp,2);
	//printf("\nparity: %d,", ECC->parity);	
	//printf("\nrow cal: %d", ECC->RowCorrectionalCode);
	//printf("\nrow rx: %d", ECC->RowReceivedCode);
	//printf("\ncol cal: %d", ECC->ColumnCorrectionalCode);
	//printf("\ncol rx: %d", ECC->ColumnReceivedCode);
	return 0;	
}
int capture_correctional_codes(struct CorrectionalCodes *ECC, struct data *transaction_data)
{
	int i;
	ECC->RowReceivedCode = transaction_data->data_array[5];
	ECC->ColumnReceivedCode = transaction_data->data_array[6];
	ECC->parity_recieved = (transaction_data->data_array[7] & (1u << 7) ? 1 : 0);
	transaction_data->data_array[7] = (transaction_data->data_array[7] & ~(1u << 7));
	ECC->ColumnReceivedCode = ECC->ColumnReceivedCode + (transaction_data->data_array[7]*256);
	//printf("Received row correctional code w parity bit:\n");
	//for(i=0;i<8;i++)
	//{
	//	putchar(ECC->RowReceivedCode & (1u << i) ? '1' : '0');
	//}
	//printf("\n");
	//ECC->parity_recieved = (ECC->ColumnReceivedCode & (1u << 7) ? 1 : 0);
	//ECC->ColumnReceivedCode = (ECC->ColumnReceivedCode & ~(1u << 7));
	//printf("Received row correctional code w parity bit parsed:\n");
	//for(i=0;i<8;i++)
	//{
	//	putchar(ECC->RowReceivedCode & (1u << i) ? '1' : '0');
	//}
	//printf("\n");
	printf("\nrow cal: %d", ECC->RowCorrectionalCode);
	printf("\nrow rx: %d", ECC->RowReceivedCode);
	printf("\ncol cal: %d", ECC->ColumnCorrectionalCode);
	printf("\ncol rx: %d", ECC->ColumnReceivedCode);
		
	return 0;
}
void build_Matrices(struct data *transaction_data)
{
	int i;
	for(i=0;i<8;i++)
	{
		transaction_data->generator_array[i] = 0;
	}
	//printf("\n");
	for(i=0;i<8;i++)
	{
		transaction_data->generator_array[i] = power(2,(i));
		//printf("%d,",transaction_data->generator_array[i]);
	}
	/*printf("\ngenerator array:\n");
	for(i=0;i<8;i++)
	{
		printf("%c,",transaction_data->generator_array[i]);
	}
	printf("\n");*/
	/*printf("first byte:\n");
	for(i=0;i<8;i++)
	{
		putchar(transaction_data->data_array[0] & (1u << i) ? '1' : '0');
	}
	printf("\n");
		printf("second byte:\n");
	for(i=0;i<8;i++)
	{
		putchar(transaction_data->data_array[1] & (1u << i) ? '1' : '0');
	}
	printf("\n");
		printf("third byte:\n");
	for(i=0;i<8;i++)
	{
		putchar(transaction_data->data_array[2] & (1u << i) ? '1' : '0');
	}
	printf("\n");
		printf("fourth byte:\n");
	for(i=0;i<8;i++)
	{
		putchar(transaction_data->data_array[3] & (1u << i) ? '1' : '0');
	}
	printf("\n");
		printf("fifth byte:\n");
	for(i=0;i<8;i++)
	{
		putchar(transaction_data->data_array[4] & (1u << i) ? '1' : '0');
	}
	printf("\n");*/
	for(i=0;i<8;i++)
	{
		transaction_data->data_matrix[0][i] = (transaction_data->data_array[0] & (1u << i) ? 1 : 0);
		transaction_data->data_matrix[1][i] = (transaction_data->data_array[1] & (1u << i) ? 1 : 0);
		transaction_data->data_matrix[2][i] = (transaction_data->data_array[2] & (1u << i) ? 1 : 0);
		transaction_data->data_matrix[3][i] = (transaction_data->data_array[3] & (1u << i) ? 1 : 0);
		transaction_data->data_matrix[4][i] = (transaction_data->data_array[4] & (1u << i) ? 1 : 0);			
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
int calculate_single_bit_errors(struct CorrectionalCodes *ECC, struct errors *transaction_errors, struct data *transaction_data)
{	
	//printf("\nrow cal: %d", ECC->RowCorrectionalCode);
	//printf("\nrow rx: %d", ECC->RowReceivedCode);
	//printf("\ncol cal: %d", ECC->ColumnCorrectionalCode);
	//printf("\ncol rx: %d", ECC->ColumnReceivedCode);
	double row_diff = fabs(ECC->RowCorrectionalCode - ECC->RowReceivedCode);
	double column_diff = fabs(ECC->ColumnCorrectionalCode - ECC->ColumnReceivedCode);
	//printf("\nrow diff: %f", row_diff);
	//printf("\ncolumn diff: %f", column_diff);
	printf("\n");
		//printf("\nError detected.\n");
	printf("\nSingle bit error possibilities,\n");
	if(row_diff != 0)
	{
		transaction_errors->single_error_coordinates[0][0] = log2(row_diff);
	} else {
		transaction_errors->single_error_coordinates[0][0] = -1;
	}
	if(column_diff != 0)
	{ 
		transaction_errors->single_error_coordinates[0][1] = log2(column_diff); 
	} else {
		transaction_errors->single_error_coordinates[0][1] = -1;
	}
	ECC->no_of_errors = 1;
	printf("coordinates:\n%f, %f\n", transaction_errors->single_error_coordinates[0][0], transaction_errors->single_error_coordinates[0][1]);
			
	return 0;

}
int calculate_dual_bit_errors(struct CorrectionalCodes *ECC, struct errors *transaction_errors, struct data *transaction_data)
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
	printf("\nrow diff: %f", row_diff);
	printf("\ncolumn diff: %f", column_diff);
	printf("\n");
	printf("dual bit error possibilities.\n");
	//row
	for (i=0;i<10;i++)
	{
		temp1[i] = 0;
		temp2[i] = 0;
	}
				
	for (i=1;i<5;i++)
	{
		//printf("row diff: %f, gen array: %d\n", row_diff, generator_array[i]);
		if(row_diff >= transaction_data->generator_array[i])
		{
			temp1[i] = 1;
		}else {temp1[i] = 0;}
	}	
	for (i=1;i<5;i++)
	{
		if(fmod(row_diff,transaction_data->generator_array[i])==0)
		{
			temp2[i] = 1;
		}else {temp2[i] = 0;}
	}	
	for (i=1;i<5;i++)
	{
		row1 = row1 + temp1[i];
		row2 = row2 + temp2[i];
	}
	if((fmod(row_diff,1)>=0)&&(fmod(column_diff,1)>=0)){transaction_errors->dual_error_coordinates[0][0] = row1;} else {transaction_errors->dual_error_coordinates[0][0] = -1;}
	if((fmod(transaction_errors->single_error_coordinates[0][0],1)!=0)||(fmod(transaction_errors->single_error_coordinates[0][1],1)!=0)){transaction_errors->dual_error_coordinates[1][0] = row2;} else {transaction_errors->dual_error_coordinates[1][0] = -1;}
	if((fmod(transaction_errors->single_error_coordinates[0][0],1)!=0)||(fmod(transaction_errors->single_error_coordinates[0][1],1)!=0)){transaction_errors->dual_error_coordinates[2][0] = row2;} else {transaction_errors->dual_error_coordinates[2][0] = -1;}
	if((fmod(transaction_errors->single_error_coordinates[0][0],1)!=0)||(fmod(transaction_errors->single_error_coordinates[0][1],1)!=0)){transaction_errors->dual_error_coordinates[3][0] = row1;} else {transaction_errors->dual_error_coordinates[3][0] = -1;}
	ECC->no_of_errors = 2;
	//column
	for (i=0;i<10;i++)
	{
		temp1[i] = 0;
		temp2[i] = 0;
	}		
	for (i=1;i<8;i++)
	{
		if(column_diff >= transaction_data->generator_array[i])
		{
			temp1[i] = 1;
		}else {temp1[i] = 0;}
	}	
	for (i=1;i<8;i++)
	{
		if(fmod(column_diff, transaction_data->generator_array[i])==0)
		{
			temp2[i] = 1;
		}else {temp2[i] = 0;}
	}	
	for (i=1;i<8;i++)
	{
		col1 = col1 + temp1[i];
		col2 = col2 + temp2[i];
	}
	if((fmod(transaction_errors->single_error_coordinates[0][0],1)!=0)||(fmod(transaction_errors->single_error_coordinates[0][1],1)!=0)){transaction_errors->dual_error_coordinates[0][1] = col1;} else {transaction_errors->dual_error_coordinates[0][1] = -1;}
	if((fmod(row_diff,1)>=0)&&(fmod(column_diff,1)>=0)){transaction_errors->dual_error_coordinates[1][1] = col2;} else {transaction_errors->dual_error_coordinates[1][1] = -1;}
	if((fmod(transaction_errors->single_error_coordinates[0][0],1)!=0)||(fmod(transaction_errors->single_error_coordinates[0][1],1)!=0)){transaction_errors->dual_error_coordinates[2][1] = col1;} else {transaction_errors->dual_error_coordinates[2][1] = -1;}
	if((fmod(transaction_errors->single_error_coordinates[0][0],1)!=0)||(fmod(transaction_errors->single_error_coordinates[0][1],1)!=0)){transaction_errors->dual_error_coordinates[3][1] = col2;} else {transaction_errors->dual_error_coordinates[3][1] = -1;}
	ECC->no_of_errors = 2;
	printf("Error coordinates\nGuess#1:\n");			
	printf("%d, %d\n", transaction_errors->dual_error_coordinates[0][0], transaction_errors->dual_error_coordinates[0][1]);
	printf("%d, %d\n", transaction_errors->dual_error_coordinates[1][0], transaction_errors->dual_error_coordinates[1][1]);
	printf("Guess#2:\n");			
	printf("%d, %d\n", transaction_errors->dual_error_coordinates[2][0], transaction_errors->dual_error_coordinates[2][1]);
	printf("%d, %d\n", transaction_errors->dual_error_coordinates[3][0], transaction_errors->dual_error_coordinates[3][1]);
	
	return 0;
}
int calculate_burst_errors(struct CorrectionalCodes *ECC, struct errors *transaction_errors, struct data *transaction_data)
{
	int temp1[10];
	int temp2[10];
	float temp3[10];
	int temp4[10];
	int temp5[10];
	int i, a;
	double row_diff = fabs(ECC->RowCorrectionalCode - ECC->RowReceivedCode);
	double column_diff = fabs(ECC->ColumnCorrectionalCode - ECC->ColumnReceivedCode);

	transaction_errors->row_highest_line = 0;
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
			transaction_errors->row_highest_line = transaction_errors->row_highest_line + temp1[i];
		}
		transaction_errors->row_highest_line = transaction_errors->row_highest_line - 1;
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
				transaction_errors->row_length = temp4[i];//burst->length + 1;
				break;
			}
		}
		if (row_diff == 0)
		{
		transaction_errors->row_line_no = -1;
		} else {transaction_errors->row_line_no = log2(row_diff/transaction_errors->row_length);}
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
	printf("row burst length: %d\n", transaction_errors->row_length);
	printf("in row: %d\n", transaction_errors->row_line_no);
	printf("to column: %d\n", transaction_errors->row_highest_line);

	for(i=0;i<10;i++)
	{
		temp1[i] = 0;
		temp2[i] = 0;
		temp3[i] = 0;
		temp4[i] = 0;
		temp5[i] = 0;
	}
	transaction_errors->col_highest_line = 0;
	if ((row_diff > 0) || (column_diff > 0))
	{
		for(i=0;i<8;i++)
		{
			temp5[i] = 0;
			if((log2(row_diff)-i)>0)
			{
				temp1[i] = 1;
			}else {temp1[i] = 0;}
			temp2[i] = pow(2,i);
		}
		for(i=0;i<8;i++)
		{
			transaction_errors->col_highest_line = transaction_errors->col_highest_line + temp1[i];
		}
		transaction_errors->col_highest_line = transaction_errors->col_highest_line - 1;
		//printf("\nrow diff: %f", row_diff);
	    //printf("\ncolumn diff: %f", column_diff);
		for (i=0;i<8;i++)
		{
			temp3[i] = column_diff/temp2[i];
			if(fmod((column_diff/temp2[i]),1)==0)
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
			if (temp5[i] == row_diff)
			{
				transaction_errors->col_length = temp4[i];//burst->length + 1;
				break;
			}
		}
		if (column_diff == 0)
		{
		transaction_errors->col_line_no = -1;
		} else {transaction_errors->col_line_no = log2(column_diff/transaction_errors->col_length);}
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
	printf("column burst length: %d\n", transaction_errors->col_length);
	printf("in column: %d\n", transaction_errors->col_line_no);
	printf("to row: %d\n", transaction_errors->col_highest_line);
		
	return 0;
}
void correct_packet(int attempt_no, struct data *transaction_data, struct CorrectionalCodes *ECC, struct errors *transaction_errors)
{
	int i, a, b;
	int temp[16];
	int row_dim = 5;
	int col_dim = 8;
	if (attempt_no == 0){
		printf("Attempting single bit correction:\n");
		if (transaction_errors->single_error_coordinates[0][0] == -1)
		{
			for (i=0;i<11;i++)
			{
				temp[i] = (ECC->ColumnReceivedCode & (1u << i) ? 1 : 0);
			}
			ECC->ColumnReceivedCode = 0;
			temp[(int)transaction_errors->single_error_coordinates[0][1]] = !temp[(int)transaction_errors->single_error_coordinates[0][1]];
			for (i=0;i<11;i++)
			{
				ECC->ColumnReceivedCode = ECC->ColumnReceivedCode + (temp[i]*pow(2,i));
			}
		} else if (transaction_errors->single_error_coordinates[0][1] == -1)
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
			temp[(int)transaction_errors->single_error_coordinates[0][0]] = !temp[(int)transaction_errors->single_error_coordinates[0][0]];
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
			if ((transaction_errors->single_error_coordinates[0][0] < row_dim) && (transaction_errors->single_error_coordinates[0][1] < col_dim))
			{
				transaction_data->data_matrix[(int)transaction_errors->single_error_coordinates[0][0]][(int)transaction_errors->single_error_coordinates[0][1]] = !transaction_data->data_matrix[(int)transaction_errors->single_error_coordinates[0][0]][(int)transaction_errors->single_error_coordinates[0][1]];
			}
		}
	} else if ((attempt_no == 1) || (attempt_no == 2)) {
		if(attempt_no == 1) { printf("Attempting dual bit correction #1:\n");}
		if(attempt_no == 2) { printf("Attempting dual bit correction #2:\n");}
		for(i=0;i<ECC->no_of_errors;i++)
		{
			if ((transaction_errors->dual_error_coordinates[i][0] >= 0) && (transaction_errors->dual_error_coordinates[i][1] == -1))
			{
				for(a=0;a<8;a++)
				{
					temp[a] = (ECC->RowReceivedCode & (1u << a) ? 1 : 0);
				}
				temp[transaction_errors->dual_error_coordinates[i][0]] = !temp[transaction_errors->dual_error_coordinates[i][0]];
				ECC->RowReceivedCode = 0;
				for(a=0;a<8;a++)
				{
					ECC->RowReceivedCode = ECC->RowReceivedCode + (temp[a]*power(2,a));
				}
				for(a=0;a<8;a++) temp[a] = 0;
			} else if ((transaction_errors->dual_error_coordinates[i][0] == -1) && (transaction_errors->dual_error_coordinates[i][1] >= 0))
			{
				for(a=0;a<8;a++)
				{
					temp[a] = (ECC->ColumnReceivedCode & (1u << a) ? 1 : 0);
				}
				temp[transaction_errors->dual_error_coordinates[i][1]] = !temp[transaction_errors->dual_error_coordinates[i][1]];
				ECC->ColumnReceivedCode = 0;
				for(a=0;a<8;a++)
				{
					ECC->ColumnReceivedCode = ECC->ColumnReceivedCode + (temp[a]*power(2,a));
				}
				for(a=0;a<8;a++) temp[a] = 0;
			} else {
			if (attempt_no == 1){
				if (((transaction_errors->single_error_coordinates[0][0] < row_dim) && (transaction_errors->single_error_coordinates[0][1] < col_dim)) && ((transaction_errors->single_error_coordinates[1][0] < row_dim) && (transaction_errors->single_error_coordinates[1][1] < col_dim)))
				{
					/*for(i=0;i<5;i++)
					{
						for(a=0;a<8;a++)
						{
							printf("%d",transaction_data->data_matrix[i][a]);
						}
						printf("\n");
					}	
					printf("\n");*/
					transaction_data->data_matrix[transaction_errors->dual_error_coordinates[0][0]][transaction_errors->dual_error_coordinates[0][1]] = !transaction_data->data_matrix[transaction_errors->dual_error_coordinates[0][0]][transaction_errors->dual_error_coordinates[0][1]];
					transaction_data->data_matrix[transaction_errors->dual_error_coordinates[1][0]][transaction_errors->dual_error_coordinates[1][1]] = !transaction_data->data_matrix[transaction_errors->dual_error_coordinates[1][0]][transaction_errors->dual_error_coordinates[1][1]];
					i++;
					/*for(i=0;i<5;i++)
					{
						for(a=0;a<8;a++)
						{
							printf("%d",transaction_data->data_matrix[i][a]);
						}
						printf("\n");
					}
					printf("\n");*/
				}
			} else if (attempt_no == 2){
				if (((transaction_errors->single_error_coordinates[2][0] < row_dim) && (transaction_errors->single_error_coordinates[2][1] < col_dim)) && ((transaction_errors->single_error_coordinates[3][0] < row_dim) && (transaction_errors->single_error_coordinates[3][1] < col_dim)))
				{
					/*for(i=0;i<5;i++)
					{
						for(a=0;a<8;a++)
						{
							printf("%d",transaction_data->data_matrix[i][a]);
						}
						printf("\n");
					}
					printf("\n");*/
					transaction_data->data_matrix[transaction_errors->dual_error_coordinates[2][0]][transaction_errors->dual_error_coordinates[2][1]] = !transaction_data->data_matrix[transaction_errors->dual_error_coordinates[2][0]][transaction_errors->dual_error_coordinates[2][1]];
					transaction_data->data_matrix[transaction_errors->dual_error_coordinates[3][0]][transaction_errors->dual_error_coordinates[3][1]] = !transaction_data->data_matrix[transaction_errors->dual_error_coordinates[3][0]][transaction_errors->dual_error_coordinates[3][1]];
					i++;
					/*for(i=0;i<5;i++)
					{
						for(a=0;a<8;a++)
						{
							printf("%d",transaction_data->data_matrix[i][a]);
						}
						printf("\n");
					}
					printf("\n");*/
				}	
			}
		  }
		}
	} else if (attempt_no == 3){
		printf("Attempting row burst error correction:\n");
		if (transaction_errors->row_length <= row_dim)
		{
			for(i=(transaction_errors->row_length-1);i>=0;i--)
			{
				//printf("%d,",i);
				transaction_data->data_matrix[transaction_errors->row_line_no][transaction_errors->row_highest_line-i] = !transaction_data->data_matrix[transaction_errors->row_line_no][transaction_errors->row_highest_line-i];
			}
		}
	} else if (attempt_no == 4) {
		printf("Attempting column burst error correction:\n");
		if (transaction_errors->col_length <= col_dim)
		{
			for(i=(transaction_errors->col_length-1);i>=0;i--)
			{
				//printf("%d,",i);
				transaction_data->data_matrix[transaction_errors->col_highest_line-i][transaction_errors->col_line_no] = !transaction_data->data_matrix[transaction_errors->col_highest_line-i][transaction_errors->col_line_no];
			}
		}
	}
}

int repackage_data(struct data *transaction_data)
{
	int i;
	transaction_data->data_array[0] = 0;
	transaction_data->data_array[1] = 0;
	transaction_data->data_array[2] = 0;
	transaction_data->data_array[3] = 0;
	transaction_data->data_array[4] = 0;
	transaction_data->data_array[5] = 0;	
	transaction_data->data_array[6] = 0;
	transaction_data->data_array[7] = 0;
	for (i=0;i<8;i++)
	{
		transaction_data->data_array[0] = transaction_data->data_array[0] + (int)transaction_data->data_matrix[0][i]*power(2,i);
	}
	for (i=0;i<8;i++)
	{
		transaction_data->data_array[1] = transaction_data->data_array[1] + (int)transaction_data->data_matrix[1][i]*power(2,i);
	}	
	for (i=0;i<8;i++)
	{
		transaction_data->data_array[2] = transaction_data->data_array[2] + (int)transaction_data->data_matrix[2][i]*power(2,i);
	}
	for (i=0;i<8;i++)
	{
		transaction_data->data_array[3] = transaction_data->data_array[3] + (int)transaction_data->data_matrix[3][i]*power(2,i);
	}
	for (i=0;i<8;i++)
	{
		transaction_data->data_array[4] = transaction_data->data_array[4] + (int)transaction_data->data_matrix[4][i]*power(2,i);
	}
	for (i=0;i<8;i++)
	{
		transaction_data->data_array[5] = transaction_data->data_array[5] + (int)transaction_data->data_matrix[5][i]*power(2,i);
	}
		for (i=0;i<8;i++)
	{
		transaction_data->data_array[6] = transaction_data->data_array[6] + (int)transaction_data->data_matrix[6][i]*power(2,i);
	}
		for (i=0;i<8;i++)
	{
		transaction_data->data_array[7] = transaction_data->data_array[7] + (int)transaction_data->data_matrix[7][i]*power(2,i);
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
int copy_structure(struct data *source, struct data *destination)
{
	int i, a;
	for(i=0;i<5;i++)
	{
		for(a=0;a<8;a++)
		{
			destination->data_matrix[i][a] = source->data_matrix[i][a];
			//printf("%d",temp_data.data_matrix[i][a]);
		}
	}
	for(i=0;i<8;i++)
	{
		destination->data_array[i] = source->data_array[i];
		destination->generator_array[i] = source->generator_array[i];
	}
	return 0;
}
