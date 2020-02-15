/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <string>



#ifdef _WIN32

#include "volrng/vhd_win.hpp"

namespace volrng
{
	typedef volrng::win::VHD DISK;
	std::string MOUNT("Z:");
}

#endif