/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#ifdef TEST_RUNNER


#define CATCH_CONFIG_RUNNER
#include "catch.hpp"
#include "volrng/test.hpp"

int main(int argc, char* argv[])
{
    return Catch::Session().run(argc, argv);
}


#endif //TEST_RUNNER



#if ! defined(TEST_RUNNER)


#include "clipp.h"

#include <string>
#include <iostream>

using namespace clipp;

#ifdef _WIN32

#include "volrng/vhd_win.hpp"

using namespace volrng::win;

#define DISK VHD
#define MOUNT "Z:"

#endif

#include "volrng/volume.hpp"


int main(int argc, char* argv[])
{
    bool mount = false, dismount = false, step = false, validate = false;
    string path = "", param = "";

    auto cli = (
        value("test directory", path),
        opt_value("parameter", param),
        option("-m", "--mount").set(mount).doc("encryption password"),
        option("-s", "--step").set(step).doc("encode regenerating file"),
        option("-v", "--validate").set(step).doc("encode regenerating file"),
        option("-d", "--dismount").set(dismount).doc("encode regenerating file")
        );

    if (!parse(argc, argv, cli)) cout << make_man_page(cli, argv[0]);
    else
    {
        volrng::volume::Test<DISK> handle(path);

        if (mount)
            handle.Mount(param);
        else if (dismount)
            handle.Dismount();
        else if (validate)
            handle.Validate(param);
        else if (step)
            handle.Run(1*1024*1024,param);
    }

    return 0;
}


#endif //! defined(BENCHMARK_RUNNER) && ! defined(TEST_RUNNER)


