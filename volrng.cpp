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
#include <filesystem>

#include "volrng/platform.hpp"
#include "volrng/volume.hpp"

using namespace clipp;
using namespace std;


int main(int argc, char* argv[])
{
    bool mount = false, dismount = false, step = false, validate = false;
    string path = "test", param = string(volrng::MOUNT);
    uint64_t size = 1;

    auto cli = (
        option("-p", "--path").doc("Path where the vhd is stored") & value("directory", path),
        option("-l", "--letter").doc("Drive letter to mount vhd") & value("drive letter", param),
        option("-z", "--size").doc("Path where the vhd is stored") & value("size", size),
        option("-m", "--mount").set(mount).doc("Mount the test volume ( path )"),
        option("-s", "--step").set(step).doc("Mutate the test data"),
        option("-v", "--validate").set(validate).doc("Validate test metadata against path ( path )"),
        option("-d", "--dismount").set(dismount).doc("Dismount the test data")
        );

    try 
    {
        if (!parse(argc, argv, cli))
            cout << make_man_page(cli, argv[0]);
        else
        {
            filesystem::create_directories(path);
            volrng::volume::Test<volrng::DISK> handle(path);

            if (mount)
            {
                cout << "--mount " << path << " " << param << endl;
                handle.Mount(param);
                cout << "success" << endl;
            }
            else if (dismount)
            {
                cout << "--dismount " << path << endl;
                handle.Dismount();
                cout << "success" << endl;
            }
            else if (validate)
            {
                cout << "--validate " << path << " " << param << endl;
                if (handle.Validate(param))
                    cout << "Point in Time Valid" << endl;
                else
                    cout << "Point in Time INVALID" << endl;
            }
            else if (step)
            {
                handle.Dismount();

                cout << "--step " << path << " " << param << " " << size << "mb" << endl;
                handle.Run(size * 1024 * 1024, param);
                cout << "success" << endl;
            }
            else
                cout << make_man_page(cli, argv[0]);
        }
    }
    catch (const exception & ex) 
    {
        cerr << ex.what() << endl;
        return -1;
    }

    return 0;
}


#endif //! defined(BENCHMARK_RUNNER) && ! defined(TEST_RUNNER)


