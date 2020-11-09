/*
	A modbus client
	- Read the first 5 numbers from the server.
	- Write to the server to double the last 5 numbers.
	- Read the last five numbers to confirm you have changed them.
*/


#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <modbus.h>


int main()
{
	int rc, i;
	float f;
	float f_arr[5];
	uint16_t* tab_reg;
	modbus_t* mb = NULL;

	mb = modbus_new_tcp("127.0.0.1", 1502);

	if (NULL == mb)
	{
		fprintf(stderr, "Unable to allocate libmodbus: %s\n", 
			modbus_strerror(errno));
		return -1;
	}

	if (-1 == modbus_connect(mb)) 
	{
		fprintf(stderr, "Connection failed: %s\n",
			modbus_strerror(errno));
		modbus_free(mb);
		return -1;
	}

	/* Allocate and initialize 20*16 bits memory to store
	   the registers for 10 float numbers (10*32 bits)   */
	tab_reg = (uint16_t*)malloc(20 * sizeof(uint16_t));
	memset(tab_reg, 0, 20 * sizeof(uint16_t));

	//read all 10 float numbers
	rc = modbus_read_registers(mb, 100, 20, tab_reg);
	if (-1 == rc) 
	{
		fprintf(stderr, "%s\n", modbus_strerror(errno));
		return -1;
	}
	
	for (i = 0; i < 10; i++) 
	{
		f = modbus_get_float(tab_reg + i*32);
		if (5 > i)
		{
			//print out first 5 float numbers
			printf("%dth number is: %f\n", i, f);
		}
		else
		{
			//store last 5 float numbers in an array and double them
			f_arr[(i - 5)] = f * 2;
		}
	}

	//Write doubled last 5 float number to server
	for (i = 0; i < 5; i++)
	{
		uint16_t reg;
		modbus_set_float(f_arr[i], reg);
		rc = modbus_write_register(mb, 100 + 32 * 5, reg);
		if (-1 == rc)
		{
			fprintf(stderr, "%s\n", modbus_strerror(errno));
			return -1;
		}
	}

	//Read the last 5 float number again
	rc = modbus_read_registers(mb, 100 + 32 * 5, 10, tab_reg);
	if (-1 == rc)
	{
		fprintf(stderr, "%s\n", modbus_strerror(errno));
		return -1;
	}

	for (i = 0; i < 5; i++) 
	{
		f = modbus_get_float(tab_reg + i * 32);
		printf("%d number is: %f\n", i, f);
	}

	modbus_close(mb);
	modbus_free(mb);
	free(tab_reg);
}