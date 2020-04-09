#include <unistd.h> // write()
#include <signal.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "pigpio.h"

#include "mtx2/mtx2.h"
#include "nmea/nmea.h"
#include "ublox/ublox_cmds.h"
#include "ublox/ublox.h"
#include "ds18b20/ds18b20.h"

#include "ssdv_t.h"
#include "cli.h"
#include "GLOB.h"


int radio_fd = 0;


char _hex(char Character)
{
	char _hexTable[] = "0123456789ABCDEF";
	return _hexTable[int(Character)];
}

std::string CRC(std::string i_str)
{
	using std::string;

	unsigned int CRC = 0xffff;
	// unsigned int xPolynomial = 0x1021;

	for (size_t i = 0; i < i_str.length(); i++)
	{
		CRC ^= (((unsigned int)i_str[i]) << 8);
		for (int j=0; j<8; j++)
		{
			if (CRC & 0x8000)
			    CRC = (CRC << 1) ^ 0x1021;
			else
			    CRC <<= 1;
		}
	}

	string result;
	result += _hex((CRC >> 12) & 15);
	result += _hex((CRC >> 8) & 15);
	result += _hex((CRC >> 4) & 15);
	result += _hex(CRC & 15);

	return result;
}


void CTRL_C(int sig)
{
	gpioWrite(GLOB::get().cli.hw_pin_radio_on, 0);
	close(radio_fd);
	gpioTerminate();
	exit(0);
}


int main1(int argc, char** argv)
{
    using namespace std;

	CLI(argc, argv); // command line interface
	auto& G = GLOB::get();

	if( !G.cli.callsign.size() ) {
		cerr<<"ERROR:\n\tNo Callsign."<<endl;
		return 1;
	}

	if( !G.cli.freqMHz ) {
		cerr<<"ERROR:\n\tNo frequency."<<endl;
		return 1;
	}

	if(G.cli.baud == baud_t::kInvalid) {
		cerr<<"ERROR:\n\tNo baud."<<endl;
		return 1;
	}

	if( !G.cli.hw_pin_radio_on ) {
		cerr<<"ERROR:\n\tNo hw_pin_radio_on."<<endl;
		return 1;
	}

	if( !G.cli.hw_radio_serial.size() ) {
		cerr<<"ERROR:\n\tNo hw_radio_serial."<<endl;
		return 1;
	}

	if( !G.cli.hw_ublox_device.size() ) {
		cerr<<"ERROR:\n\tNo hw_ublox_device."<<endl;
		return 1;
	}

	cout<<G.str()<<endl;

	signal(SIGINT, CTRL_C);
	signal(SIGTERM, CTRL_C);

	system("sudo modprobe w1-gpio");

	if (gpioInitialise() < 0)
	{
		std::cerr<<"pigpio initialisation failed\n";
		return 1;
	}

    // RADIO
    //
	gpioSetPullUpDown( G.cli.hw_pin_radio_on, PI_PUD_DOWN );
	gpioSetMode( G.cli.hw_pin_radio_on, PI_OUTPUT );
	gpioWrite ( G.cli.hw_pin_radio_on, 1 );
	mtx2_set_frequency( G.cli.hw_pin_radio_on, G.cli.freqMHz );
	radio_fd = mtx2_open( G.cli.hw_radio_serial, G.cli.baud );
    if (radio_fd < 1)
	{
		cerr<<"Failed opening radio UART "<<G.cli.hw_radio_serial<<endl;
		return 1;
	}

    // uBLOX I2C
    //
    int uBlox_i2c_fd = uBLOX_i2c_open( G.cli.hw_ublox_device, 0x42 );
	if (!uBlox_i2c_fd)
	{
		cerr<<"Failed opening I2C "<<G.cli.hw_ublox_device<<" 0x42"<<endl;
		return 1;
	}
	write(uBlox_i2c_fd, UBX_CMD_EnableOutput_ACK_ACK, sizeof(UBX_CMD_EnableOutput_ACK_ACK));
	write(uBlox_i2c_fd, UBX_CMD_EnableOutput_ACK_NAK, sizeof(UBX_CMD_EnableOutput_ACK_NAK));
	sleep(3);
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GSV_OFF, sizeof(UBX_CMD_GSV_OFF) );
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GLL_OFF, sizeof(UBX_CMD_GLL_OFF) );
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_GSA_OFF, sizeof(UBX_CMD_GSA_OFF) );
	uBLOX_write_msg_ack( uBlox_i2c_fd, UBX_CMD_VTG_OFF, sizeof(UBX_CMD_VTG_OFF) );
    while (!uBLOX_write_msg_ack(uBlox_i2c_fd, UBX_CMD_NAV5_Airbororne1G, sizeof(UBX_CMD_NAV5_Airbororne1G)))
        cout << "Retry Setting uBLOX Airborne1G Model" << endl;
	sleep(1);

    // DS18B20 temp sensor
    //
    const string ds18b20_device = find_ds18b20_device();
    if(ds18b20_device == "")
    {
		cerr << "Failed ds18b20_device "<<ds18b20_device<<endl;
        return 1;
    }
    cout<<"ds18b20_device "<<ds18b20_device<<endl;


    nmea_t nmea; // parsed GPS data
	ssdv_t ssdv_data;
	int msg_num = 0;
	while(true)
	{
		msg_num++;

        // gps data
		//
		const vector<char> ublox_data = uBLOX_read_msg(uBlox_i2c_fd);
		const string nmea_str( NMEA_get_last_msg(ublox_data.data(), ublox_data.size()) );
		// cout<<nmea_str<<endl;
        if( !NMEA_msg_checksum_ok(nmea_str) )
        {
            cerr<<"NMEA Checksum Fail: "<<nmea_str<<endl;
            continue;
        }

		NMEA_parse( nmea_str.c_str(), nmea );
		const bool gps_fix_valid = nmea.fix_status == nmea_t::fix_status_t::kValid;

        // ds18b20
        //
        const float temperature_cels = read_temp_from_ds18b20(ds18b20_device);

		// telemetry message
		//
        stringstream  msg_stream;
        // msg_stream<<nmea;
        msg_stream<<G.cli.callsign;
        msg_stream<<","<<msg_num;
        msg_stream<<","<<nmea.utc;
        msg_stream<<","<<nmea.lat<<","<<nmea.lon<<","<<nmea.alt;
		msg_stream<<","<<nmea.sats<<","<<gps_fix_valid;
        // msg_stream<<","<<"05231.4567"<<","<<"2117.8412"<<","<<nmea.alt; // example NMEA format
        msg_stream<<","<<setprecision(1)<<fixed<<temperature_cels;

		const string msg_and_crc = string("\0",1) + "$$$" + msg_stream.str() + '*' + CRC(msg_stream.str());
        cout<<msg_and_crc<<endl;

		// emit telemetry msg RF
		//
		mtx2_write(radio_fd, msg_and_crc + '\n');

		// SSDV image
		//
		if( argc > 1 && !ssdv_data.size() )
			ssdv_data.load_file(argv[1]);
		if( ssdv_data.size() )
		{
			const ssdv_t::tile_t tile = ssdv_data.next_tile();
			mtx2_write( radio_fd, tile.data(), sizeof(tile) );
		}
	}

    close(uBlox_i2c_fd);
	close(radio_fd);
	gpioWrite (G.cli.hw_pin_radio_on, 0);
	gpioTerminate();

    return 0;
}


int main(int argc, char** argv)
{
	return main1(argc, argv);
}
