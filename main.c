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
	bool transmit_matrix[4][4], receive_matrix[4][4], temp_matrix[4][4];
	int generator_array[4];
	int serial_port;
	char tx_dat[8], rx_dat[8];
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
		tx_dat[2] = getchar();
		tx_dat[3] = getchar();
		tx_dat[4] = getchar();
		build_data_packet(tx_dat);
		printf("Transmit data:");
		printf("\n\r");
		for(a=0;a<8;a++)
		{
			printf("%d,", tx_dat[a]);//rx_data.data_array[a]);
		}
		write(serial_port, &tx_dat, 8); //transmit character
		fflush(stdout);
		for(i=0;i<8;i++)
		{
			read(serial_port, &rx_dat[i], 1); //recieve character
		}

        //inject error
		int temp0[10], temp1[10], temp2[10], temp3[10], temp4[10];
			for(a=0;a<8;a++)
			{
				temp0[a] = (rx_dat[0] & (1u << a) ? 1 : 0);
				temp1[a] = (rx_dat[1] & (1u << a) ? 1 : 0);
				temp2[a] = (rx_dat[2] & (1u << a) ? 1 : 0);
				temp3[a] = (rx_dat[3] & (1u << a) ? 1 : 0);
				temp4[a] = (rx_dat[4] & (1u << a) ? 1 : 0);
				//printf("%b", temp[a]);
			}
			//printf("\n");
			rx_dat[0] = 0;
			rx_dat[1] = 0;
			rx_dat[2] = 0;
			rx_dat[3] = 0;
			rx_dat[4] = 0;
			//temp0[1] = !temp0[1];
			//temp0[6] = !temp0[6];
			temp0[2] = !temp0[2];
			temp1[2] = !temp1[2];
			temp2[2] = !temp2[2];
			temp3[2] = !temp3[2];
			temp4[2] = !temp4[2];
			//temp[2] = !temp[2];
			//temp[0] = !temp[0];
			//temp[1] = !temp[1];
			//temp[2] = !temp[2];
			//temp[3] = !temp[3];
			//printf("\n");
			for(a=0;a<8;a++)
			{
				//printf("%b\n", rx_dat[0]);
				rx_dat[0] = rx_dat[0] + (temp0[a]*pow(2,a));
				rx_dat[1] = rx_dat[1] + (temp1[a]*pow(2,a));
				rx_dat[2] = rx_dat[2] + (temp2[a]*pow(2,a));
				rx_dat[3] = rx_dat[3] + (temp3[a]*pow(2,a));
				rx_dat[4] = rx_dat[4] + (temp4[a]*pow(2,a));
			}
			//printf("\n");
			check_data_packet(rx_dat);
			printf("\n%c%c%c%c%c\n", rx_dat[0], rx_dat[1], rx_dat[2], rx_dat[3], rx_dat[4]);
	}
	return 0;
}


