#include "ssdv_t.h"

#include <iostream>
#include <cstring>

size_t ssdv_t::load_file(const std::string file_path)
{
    FILE* p_file = fopen(file_path.c_str(), "rb");
    if(!p_file)
        return 0;

    packet_t tile;
    memset(tile.data(), 0x00, sizeof(tile));
    size_t total_tiles = 0;
    while(1)
    {
        const size_t read_bytes = fread( tile.data(), 1, sizeof(tile), p_file );
        if(!read_bytes)
            break;
        tiles_que_.push_back(tile);
        ++total_tiles;
    }

    if(!total_tiles) {  // could not load this file. delete it.
        std::cout<<"SSDV Failed loading "<<file_path<<std::endl;
        system( (std::string("rm -f ") + file_path + " || echo \"Can't delete SSDV image.\"").c_str() );
    }

    return  total_tiles;
}

ssdv_t::packet_t ssdv_t::next_packet()
{
    packet_t tile = tiles_que_.front();
    tiles_que_.pop_front();
    return tile;
}
