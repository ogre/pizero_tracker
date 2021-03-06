#include "ublox_cmds.h"
#include "ublox.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include <cstring>
#include <memory>
#include <iostream>


int uBLOX_i2c_open(const std::string i_device, const unsigned char addr) //"/dev/i2c-7", 0x42
{
	using namespace std;

	int file_i2c;

	if ((file_i2c = open(i_device.c_str(), O_RDWR)) < 0)
	{
		cerr << "Failed to open the i2c bus " << i_device << endl;
		return 0;
	}

	if (ioctl(file_i2c, I2C_SLAVE, addr) < 0)
	{
		cerr << "Failed to acquire bus access and/or talk to slave Address: " << addr << endl;
		return 0;
	}

	return file_i2c;
}


// read char from file descriptor
// and returns a std:string message (anything  between 255 or /n characters)
// sometimes returns just \n
// input:
// 		fd - file descriptor
// 		usec_sleep = sleep  microseconds when uBLOX talks 255
std::vector<char> uBLOX_read_msg(int fd, int usec_sleep)
{
	using namespace std;

	char res_str[256] = {0}; // copy incoming chars here
	size_t res_str_i = 0;

	thread_local unsigned char buffer[128] = {0};
	thread_local int buff_i = 0;
	thread_local int buff_o = 0;

	while( res_str_i < sizeof(res_str) )
	{
		if( buff_i == buff_o ) // all buffer was scanned since last call
		{
			// read some bytes...
			int read_bytes = read(fd, buffer, sizeof(buffer));
			if (read_bytes < 1)
			{
				cerr<<" Error reading I2C. Read bytes: "<<read_bytes<<endl;
				return vector<char>(0);
			}
			if (read_bytes > sizeof(buffer))
			{
				cerr<<" Error reading I2C. Read bytes more than requested: "<<read_bytes<<endl;
				return vector<char>(0);
			}

			buff_i = 0;
			buff_o = read_bytes;
		}

		// uBLOX talks 255 when it sleeps
		// find first msg char, start where we left in last call
		while( 	buff_i < buff_o
				&& 	(  buffer[buff_i] == 255
					|| buffer[buff_i] == '\n'
					|| buffer[buff_i] == '\r' )
			)
			buff_i++;

		if(buff_i == buff_o) // no msg found
		{
			usleep(usec_sleep);
			continue;
		}

		// copy to output buffer
		while(buff_i < buff_o)
		{
			if(		buffer[buff_i] == 255
				|| 	buffer[buff_i] == '\n'
				|| 	buffer[buff_i] == '\r') // end of message
			{
				buff_i++;
				// string res(res_str); // should we resize string ?
				// return res;
				vector<char> res(res_str_i);
				memcpy(&res[0], res_str, res_str_i);
				return res;
			}

			res_str[res_str_i++] = buffer[buff_i++];
		}
	}

	return vector<char>(0);

}


// write message with ACK
// wait - how many reply lines to check for ACK
bool uBLOX_write_msg_ack(int fd, uint8_t* p_msg, size_t msg_sz, const size_t wait)
{
	using namespace std;

	int written_bytes = write(fd, p_msg, msg_sz);

	if(written_bytes != msg_sz)
		return false;

	unique_ptr<uint8_t[]> ack_packet( new uint8_t[10] );
	UBX_MAKE_PACKET_ACK( p_msg[2], p_msg[3], ack_packet.get() );

	unique_ptr<uint8_t[]> nak_packet( new uint8_t[10] );
	UBX_MAKE_PACKET_ACK( p_msg[2], p_msg[3], nak_packet.get() );

	size_t tries = wait;
	while(tries--)
	{
		vector<char> result_msg = uBLOX_read_msg(fd, 0); // no sleep, we can't miss any message

		if( UBX_PACKET_EQ(result_msg, ack_packet.get(), 10) )
			return true;

		if( UBX_PACKET_EQ(result_msg, nak_packet.get(), 10) )
			return false;

		// cerr<<"uBLOX_write_msg_ack: some other message"<<endl;
	}

	return false;
}

std::string vec2str(std::vector<char> v)
{
	char* buff = new char [v.size()+1];
	memcpy(buff, v.data(), v.size());
	buff[v.size()] = '\0';
	std::string res = std::string(buff);
	delete [] buff;
	return res;
}
