/* Copyright (C) 2020 D8DATAWORKS - All Rights Reserved */

#pragma once

#ifdef _WIN32

#include "vhd_win.hpp"
#include "block_db.hpp"

using namespace volrng::win;

#define DISK VHD
#define MOUNT "Z:"

#endif

#include <filesystem>
#include "../catch.hpp"
#include "volume.hpp"

using namespace volrng::volume;
using namespace volrng::blocks;



#ifdef _WIN32

#include "iscsi_win.hpp"

TEST_CASE("ISCSI Client", "[volrng::iscsi]")
{
    //volrng::win::ISCSIClient cli("127.0.0.1","3260");
}

#endif

TEST_CASE("Run test volume", "[volrng::volume]")
{
    filesystem::remove_all("testdir");
    filesystem::create_directory("testdir");

    {
        Test<DISK> test("testdir");
        test.Run(volrng::util::_mb(1), MOUNT);

        CHECK(true == test.Validate(MOUNT));
    }

    filesystem::remove_all("testdir");
}

TEST_CASE("Block Database", "[volrng::block_db]")
{
    filesystem::remove("temp.db");

    auto constexpr limit = 256 * 1024;

    block _short, _a1;
    {
        Database<limit> db("temp.db");

        _short = db.Short();
        CHECK(_short.size() <= limit);

        auto d1 = db.Duplicate();
        CHECK(d1.size() == _short.size());

        _a1 = db.Allocate();
        CHECK(_a1.size() == limit);

        auto d2 = db.Duplicate();
        CHECK((d2.size() == _short.size() || d2.size() == _a1.size()));
    }

    {
        Database<limit> db("temp.db");

        auto d2 = db.Duplicate();
        CHECK((d2.size() == _short.size() || d2.size() == _a1.size()));
    }

    filesystem::remove("temp.db");
}

TEST_CASE("Create, Partition and Mount VHD", "[volrng::vhd_win]")
{
    filesystem::remove("temp.img");

    {
        DISK disk("temp.img", 50 * 1024 * 1024, MOUNT);

        CHECK(true == filesystem::exists(MOUNT));
    }

    filesystem::remove("temp.img");
}

