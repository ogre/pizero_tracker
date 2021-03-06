#include "cli.h"


#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "GLOB.h"

#include <boost/program_options.hpp>

void CLI(int ac, char* av[])
{
    namespace po = boost::program_options;
	using namespace std;

	try
	{
		po::options_description generic("Command Line Interface options");
		generic.add_options()
			("help", "Display help message")
            ("callsign",po::value<string>(),    "callsign")
            ("freq",	po::value<float>(),     "frequency in MHz")
            ("baud",	po::value<int>(),       "baud: 50, 150, 200, 300, 600, 1200")
            ("ssdv",	po::value<string>(),    "ssdv encoded image path")
            ("msg_num",	po::value<int>(),       "number of telemetry sentences emitted between SSDV packets")
            ("port",	po::value<int>(),       "zeroMQ server port")
            ("logsdir", po::value<string>(),    "logs dir")
            ("hw_pin_radio_on",	po::value<int>(),       "gpio numbered pin for radio enable. current board: 22")
            ("hw_radio_serial",	po::value<string>(),    "serial device for MTX2 radio. for rPI4: /dev/serial0")
            ("hw_ublox_device",	po::value<string>(),    "I2C device for uBLOX. for rPI4: /dev/i2c-7")
            ("latlonalt",	po::value< std::vector<float> >()->multitoken(), "launch site GPS location (decimal) and alt meters")
            ("testgps",	    po::value<bool>(),  "generate fake GPS for testing")
            ("watchdog",	po::value<bool>(),  "enable watchdog")
            ;

        po::options_description cli_options("Command Line Interface options");
		cli_options.add(generic);

		string config_file;
		cli_options.add_options()
            ("config", po::value<string>(&config_file), "config file");

		po::options_description file_options;
        file_options.add(generic);

		po::variables_map vm;
		store( po::command_line_parser(ac, av).options(cli_options).allow_unregistered().run(), vm );
		notify(vm);

        if(vm.count("config"))
		{
            const string config_file = vm["config"].as<string>();
			ifstream ifs(config_file.c_str());
			if (!ifs)
			{
				cout << "Can not open config file: " << config_file << endl;
			}
			else
			{
				cout<<"Reading config from file "<<config_file<<endl;
				store(parse_config_file(ifs, file_options, 1), vm);
				notify(vm);
			}
		}

        if(vm.count("help"))
        {
            cout<<cli_options<<endl;
			exit(0);
		}

        if (vm.count("callsign"))           GLOB::get().cli.callsign =          vm["callsign"].as<string>();
        if (vm.count("freq"))               GLOB::get().cli.freqMHz =           vm["freq"].as<float>();
        if (vm.count("msg_num"))            GLOB::get().cli.msg_num =           vm["msg_num"].as<int>();
        if (vm.count("port"))               GLOB::get().cli.port =              vm["port"].as<int>();
        if (vm.count("logsdir"))            GLOB::get().cli.logsdir =           vm["logsdir"].as<string>();
        if (vm.count("hw_pin_radio_on"))    GLOB::get().cli.hw_pin_radio_on =   vm["hw_pin_radio_on"].as<int>();
        if (vm.count("hw_radio_serial"))    GLOB::get().cli.hw_radio_serial =   vm["hw_radio_serial"].as<string>();
        if (vm.count("hw_ublox_device"))    GLOB::get().cli.hw_ublox_device =   vm["hw_ublox_device"].as<string>();
        if (vm.count("ssdv"))               GLOB::get().cli.ssdv_image =        vm["ssdv"].as<string>();
        if (vm.count("baud"))               GLOB::get().cli.baud = static_cast<baud_t>( vm["baud"].as<int>() );
        if (vm.count("testgps"))            GLOB::get().cli.testgps =           vm["testgps"].as<bool>();
        if (vm.count("watchdog"))            GLOB::get().cli.watchdog =           vm["watchdog"].as<bool>();
        if (vm.count("latlonalt")) {
            vector<float> latlonalt_vec = vm["latlonalt"].as< vector<float> >();
            GLOB::get().cli.lat = latlonalt_vec[0];
            GLOB::get().cli.lon = latlonalt_vec[1];
            GLOB::get().cli.alt = latlonalt_vec[2];
        }

    }
	catch(exception& e)
	{
        cerr<<"CLI ERROR:\n\t";
		cerr<<e.what()<<endl;
        exit(1);
	}
}
