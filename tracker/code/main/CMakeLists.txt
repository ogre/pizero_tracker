
set ( Boost_USE_STATIC_LIBS ON )
find_package(Boost REQUIRED COMPONENTS program_options )
include_directories( ${Boost_INCLUDE_DIRS} )

# https://github.com/zeromq/cppzmq/
#find cppzmq wrapper, installed by make of cppzmq
find_package(cppzmq)

set (tracker_src
	GLOB.h GLOB.cpp
	cli.h cli.cpp
	dynamics_t.h dynamics_t.cpp
	ssdv_t.cpp
	main.cpp
	gps_distance_t.h gps_distance_t.cpp
	../mtx2/mtx2.cpp
	../nmea/nmea.cpp
	../ublox/ublox_cmds.cpp
	../ublox/ublox.cpp
	../ds18b20/ds18b20.cpp
)

include_directories( ${CMAKE_SOURCE_DIR} )

add_executable( tracker ${tracker_src})

target_link_libraries( tracker pthread atomic pigpio cppzmq-static ${Boost_LIBRARIES} )
