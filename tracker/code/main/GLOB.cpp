#include "GLOB.h"

#include <sstream>
#include <iostream>
#include <iomanip>

std::string GLOB::str() const
{
    std::stringstream s;

    s<<"\tcallsign="<<cli.callsign<<"\n";
    s<<"\tfreqMHz="<<cli.freqMHz<<"\n";
    s<<"\tbaud="<<static_cast<int>(cli.baud)<<"\n";
    s<<"\tssdv_image="<<cli.ssdv_image<<"\n";
    s<<"\tmsg_num="<<cli.msg_num<<"\n";
    s<<"\tlat="<<cli.lat<<"\n";
    s<<"\tlon="<<cli.lon<<"\n";
    s<<"\talt="<<cli.alt<<"\n";
    s<<"\ttestgps="<<cli.testgps<<"\n";
    s<<"\twatchdog="<<cli.watchdog<<"\n";
    s<<"\tport="<<cli.port<<"\n";
    s<<"\tlogsdir="<<cli.logsdir<<"\n";
    s<<"\thw_pin_radio_on="<<cli.hw_pin_radio_on<<"\n";
    s<<"\thw_radio_serial="<<cli.hw_radio_serial<<"\n";
    s<<"\thw_ublox_device="<<cli.hw_ublox_device<<"\n";

    return s.str();
}


void GLOB::dynamics_add(const std::string& name, const dynamics_t::tp timestamp, const float value)
{
    auto d = dynamics_.find(name);
    if( d == dynamics_.end() )
    {
        dynamics_[name] = dynamics_t();
        dynamics_[name].add(timestamp, value);
    }
    else
    {
        d->second.add(timestamp, value);
    }

}

dynamics_t GLOB::dynamics_get(const std::string& name) const
{
    auto d = dynamics_.find(name);
    if( d == dynamics_.end() )
        return dynamics_t(); // default, uninitialised
    else
        return d->second;
}

std::vector<std::string> GLOB::dynamics_keys() const
{
    std::vector<std::string> ret;
    for(const auto& k : dynamics_)
        ret.push_back(k.first);
    return ret;
}

std::string GLOB::runtime_str() const
{
    using namespace std;

    const auto run_secs = runtime_secs_;
    const int secs = run_secs % 60;
    const int minutes = ((run_secs - secs) / 60) % 60;
    const int hours = ((run_secs - secs - 60*minutes) / 3600) % 12;

    stringstream  ss;
    ss  <<setfill('0')<<setw(2)<<hours
        <<":"<<setfill('0')<<setw(2)<<minutes
        <<":"<<setfill('0')<<setw(2)<<secs;
    return ss.str();
}
