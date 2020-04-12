/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#include <string_view>



#ifdef _WIN32

#include "volrng/vhd_win.hpp"

namespace volrng
{
	typedef volrng::win::VHD DISK;
	std::string_view MOUNT("Z:");
	std::string_view MOUNT2("Y:");
	std::string_view MOUNT3("X:");
}

#endif