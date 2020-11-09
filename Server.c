/*
	A simple Modbus Server
	- It listens to 127.0.0.1, port 502. It could accept communications from up 
	to 3 clients. More clients will be rejected.
	- Its holding registers start from 0x100, and contain 10 32-bit float numbers.
	The initial values for these registers are from 1.0 to 10.0.
*/


#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/select.h>
#include <modbus.h>

int main() {
	modbus_t* mb;
	modbus_mapping_t* mb_mapping;
	int s, fdmax, fd, rc, i;
	fd_set refset, rdset;
	uint16_t tab_reg[32];
	uint8_t query[MODBUS_MAX_ADU_LENGTH];

	mb = modbus_new_tcp("127.0.0.1", 502);
	s = modbus_tcp_listen(mb,3);
	
	/*
	  Set 10 float number to 10 32-bits holding registers.Two 16-bit 
	  registers are treated as one 32-bits registers as libmodbus defined 
	  holding registers as uint16_t type. As we only deal with one decimal 
	  point float numbers, the first 16 bits of 32-bits float number are 
	  the same as 16-bits float number. 
	 */

	mb_mapping = modbus_mapping_new_start_address(
		0, 0,
		0, 0,
		100, 20,// allocate 20 `uint16_t` registers at 0x100
		0, 0);
	if (mb_mapping == NULL) {
		fprintf(stderr, "Failed to allocate the mapping: %s\n",
			modbus_strerror(errno));
		modbus_free(mb);
		return -1;
	}

	for (i = 0; i < 10; i++)
	{
		//Set float number 1~10 to 10 32-bits holding registers
		modbus_set_float(i*1.0, mb_mapping->tab_registers + i*32);
	}

	//Clear set of reference socket
	FD_ZERO(&refset);
	//Add the server socket
	FD_SET(s, &refset);

	//Keep track of the max file descriptor
	fdmax = s;

	while (1) {
		if (-1 == select(fdmax+1, &rdset, NULL, NULL, NULL))
		{
			perror("Server select error.");
		}

		//Looping through possible avaible connection sockets
		for (fd = 0; fd < fdmax+1 ; fd++)
		{
			//Found avaiable read connection socket
			if (FD_ISSET(fd, &rdset))
			{
				if (fd == s)
				{
					//New connection
					int newfd;
					newfd = modbus_tcp_accept(mb, s);
					if (-1 == newfd)
					{
						perror("Server accept error");
					}
					else
					{
						FD_SET(newfd, &refset);
						if (fdmax < newfd)
						{
							fdmax = newfd;
						}
						printf("New connection established.");
					}
				}		
				else
				{
					//Existing connection
					modbus_set_socket(mb, fd);
					do
					{
						rc = modbus_receive(mb, query);
					} while (0 == rc);

					if (0 < rc)
					{
						modbus_reply(mb, query, rc, mb_mapping);
					}
					else if (-1 == rc)
					{
						close(fd);
						FD_CLR(fd, &refset);

						if (fdmax == fd)
						{
							fdmax--;
						}
					}
				} 
			} //if (FD_ISSET(fd, &rdset))
		} // for (fd = 0; fd < fdmax+1 ; fd++)
	} // while(1)

	modbus_mapping_free(mb_mapping);
	free(query);
	modbus_close(mb);
	modbus_free(mb);
}