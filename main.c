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


int main(int argc, char **argv)
{
	struct CorrectionalCodes tx_code;
	struct CorrectionalCodes rx_code;
	bool transmit_matrix[4][4], receive_matrix[4][4];
	int generator_array[4];
	int serial_port;
	char tx_dat[4], rx_dat[4];
	if ((serial_port = open("/dev/ttyS0", O_RDWR | O_NOCTTY | O_NDELAY)) < 0) /*open serial port*/
	{
		fprintf(stderr, "Unable to open serial device: %s\n", strerror (errno));
		return 1;
	}
	fcntl(serial_port, F_SETFL, 0);
	
	struct termios options;
	tcgetattr(serial_port, &options);
	cfsetispeed(&options, B9600);
	cfsetospeed(&options, B9600);
	tcsetattr(serial_port, TCSANOW, &options);

	if(serial_port)
	{
		int i,a;

		tx_dat[0] = getchar();	
		tx_dat[1] = getchar();
		printf("Binary Data(LSB->MSB):\n");
		for(i=0;i<8;i++)
		{
			putchar(tx_dat[0] & (1u << i) ? '1' : '0');
		}
		printf("\n");
		for(i=0;i<8;i++)
		{
			putchar(tx_dat[1] & (1u << i) ? '1' : '0');
		}
		printf("\n");
		
		build_Matrices(tx_dat, transmit_matrix, generator_array);
		
		printf("Generator Array:\n");
		for(i=0;i<4;i++)
		{
		    printf("	%d,\n", generator_array[i]);
		}
		printf("Transmit Data Matrix:\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",transmit_matrix[0][i]);
		}
		printf("\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",transmit_matrix[1][i]);
		}
		printf("\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",transmit_matrix[2][i]);
		}
		printf("\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",transmit_matrix[3][i]);
		}
		printf("\n");
		
		calculate_CorrectionalCodes(&tx_code, transmit_matrix, generator_array);
		printf("Row Correctional Code: %d\n", tx_code.RowCorrectionalCode);
		printf("Column Correctional Code: %d\n", tx_code.ColumnCorrectionalCode);
		tx_dat[2] = tx_code.RowCorrectionalCode;
		tx_dat[3] = tx_code.ColumnCorrectionalCode;
		
		printf("%u,", tx_dat[0]);
		printf("%u,", tx_dat[1]);
		printf("%u,", tx_dat[2]);
		printf("%u,", tx_dat[3]);
		printf("\n----------------------------------------\n");
		
		write(serial_port, &tx_dat, 4); //transmit character
		fflush(stdout);
		for(i=0;i<4;i++)
		{
			read(serial_port, &rx_dat[i], 1); //recieve character
		}
		rx_code.RowReceivedCode = rx_dat[2];
		rx_code.ColumnReceivedCode = rx_dat[3];
		build_Matrices(rx_dat, receive_matrix, generator_array);
        //receive_matrix[1][1] = !receive_matrix[1][1]; //inject error
		int temp[10];
			for(a=0;a<8;a++)
			{
				temp[a] = (rx_code.RowReceivedCode & (1u << a) ? 1 : 0);
				printf("%b", temp[a]);
			}
			printf("\n");
			//for(a=0;a<8;a++) printf("%b", temp[a]);
			rx_code.RowReceivedCode = 0;
			temp[2] = !temp[2];
			printf("\n");
			for(a=0;a<8;a++)
			{
				printf("%b\n", rx_code.RowReceivedCode);
				rx_code.RowReceivedCode = rx_code.RowReceivedCode + (temp[a]*pow(2,a));
			}
			printf("\n");
			for(a=0;a<8;a++) temp[a] = 0;
		printf("Receive Data Matrix:\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",receive_matrix[0][i]);
		}
		printf("\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",receive_matrix[1][i]);
		}
		printf("\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",receive_matrix[2][i]);
		}
		printf("\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",receive_matrix[3][i]);
		}
		printf("\n");
		printf("%u,", rx_dat[0]);
		printf("%u,", rx_dat[1]);
		printf("%u,", rx_code.RowReceivedCode);
		printf("%u,", rx_code.ColumnReceivedCode);
		printf("\n");
				
		calculate_CorrectionalCodes(&rx_code, receive_matrix, generator_array);
		printf("Row Correctional Code(calculated): %d\n", rx_code.RowCorrectionalCode);
		printf("Row Correctional Code(received): %d\n", rx_code.RowReceivedCode);
		int row_diff = fabs(rx_code.RowCorrectionalCode - rx_code.RowReceivedCode);
		printf("Row diff: %d\n", row_diff);
		printf("Column Correctional Code(calculated): %d\n", rx_code.ColumnCorrectionalCode);
		printf("Column Correctional Code(received): %d\n", rx_code.ColumnReceivedCode);
		int col_diff = fabs(rx_code.ColumnCorrectionalCode - rx_code.ColumnReceivedCode);
		printf("Column diff: %d\n", col_diff);
		int error_coordinates[10][2];
		//rx_code.RowReceivedCode = rx_dat[2];
		//rx_code.ColumnReceivedCode = rx_dat[3];
		calculate_errors(&rx_code, error_coordinates);
		//error_coordinates[0][0] = log2(row_diff); 
		//error_coordinates[0][1] = log2(col_diff); 
		correct_packet(receive_matrix, &rx_code, error_coordinates, 1);
		printf("Receive Data Matrix:\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",receive_matrix[0][i]);
		}
		printf("\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",receive_matrix[1][i]);
		}
		printf("\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",receive_matrix[2][i]);
		}
		printf("\n	");
		for(i=0;i<4;i++)
		{
			printf("%d, ",receive_matrix[3][i]);
		}
		printf("\n");
		printf("%u,", rx_dat[0]);
		printf("%u,", rx_dat[1]);
		printf("%u,", rx_code.RowReceivedCode);
		printf("%u,", rx_code.RowReceivedCode);
		printf("\n");
				
		calculate_CorrectionalCodes(&rx_code, receive_matrix, generator_array);
		printf("Row Correctional Code(calculated): %d\n", rx_code.RowCorrectionalCode);
		printf("Row Correctional Code(received): %d\n", rx_code.RowReceivedCode);
		row_diff = abs(rx_code.RowCorrectionalCode - rx_code.RowReceivedCode);
		printf("Row diff: %d\n", row_diff);
		printf("Column Correctional Code(calculated): %d\n", rx_code.ColumnCorrectionalCode);
		printf("Column Correctional Code(received): %d\n", rx_code.ColumnReceivedCode);
		col_diff = abs(rx_code.ColumnCorrectionalCode - rx_code.ColumnReceivedCode);
		printf("Column diff: %d\n", col_diff);		
		
	}
	return 0;
}


