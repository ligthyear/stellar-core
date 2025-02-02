// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "util/asio.h"
#include "main/Config.h"
#include "main/Application.h"
#include "xdrpp/autocheck.h"
#include "main/test.h"
#include "lib/catch.hpp"
#include "util/Logging.h"
#include "util/Timer.h"
#include "lib/util/format.h"
#include "util/Fs.h"
#include <future>
#include "process/ProcessManager.h"

using namespace stellar;

TEST_CASE("subprocess", "[process]")
{
    VirtualClock clock;
    Config const& cfg = getTestConfig();
    Application::pointer app = Application::create(clock, cfg);
    auto evt = app->getProcessManager().runProcess("hostname");
    bool exited = false;
    bool failed = false;
    evt.async_wait([&](asio::error_code ec)
                   {
                       CLOG(DEBUG, "Process") << "process exited: " << ec;
                       if (ec)
                       {
                           CLOG(DEBUG, "Process")
                               << "error code: " << ec.message();
                       }
                       failed = !!ec;
                       exited = true;
                   });

    while (!exited && !clock.getIOService().stopped())
    {
        clock.crank(true);
    }
    REQUIRE(!failed);
}

TEST_CASE("subprocess fails", "[process]")
{
    VirtualClock clock;
    Config const& cfg = getTestConfig();
    Application::pointer app = Application::create(clock, cfg);
    auto evt = app->getProcessManager().runProcess("hostname -xsomeinvalid");
    bool exited = false;
    bool failed = false;
    evt.async_wait([&](asio::error_code ec)
                   {
                       CLOG(DEBUG, "Process") << "process exited: " << ec;
                       if (ec)
                       {
                           CLOG(DEBUG, "Process")
                               << "error code: " << ec.message();
                       }
                       failed = !!ec;
                       exited = true;
                   });

    while (!exited && !clock.getIOService().stopped())
    {
        clock.crank(true);
    }
    REQUIRE(failed);
}

TEST_CASE("subprocess redirect to file", "[process]")
{
    VirtualClock clock;
    Config const& cfg = getTestConfig();
    Application::pointer appPtr = Application::create(clock, cfg);
    Application& app = *appPtr;
    std::string filename("hostname.txt");
    auto evt = app.getProcessManager().runProcess("hostname", filename);
    bool exited = false;
    evt.async_wait([&](asio::error_code ec)
                   {
                       CLOG(DEBUG, "Process") << "process exited: " << ec;
                       if (ec)
                       {
                           CLOG(DEBUG, "Process")
                               << "error code: " << ec.message();
                       }
                       exited = true;
                   });

    while (!exited && !clock.getIOService().stopped())
    {
        clock.crank(true);
    }

    std::ifstream in(filename);
    CHECK(in);
    std::string s;
    in >> s;
    CLOG(DEBUG, "Process") << "opened redirect file, read: " << s;
    CHECK(!s.empty());
    std::remove(filename.c_str());
}

#ifndef _MSC_VER
TEST_CASE("subprocess storm", "[process]")
{
    VirtualClock clock;
    Config const& cfg = getTestConfig();
    Application::pointer appPtr = Application::create(clock, cfg);
    Application& app = *appPtr;

    size_t n = 100;
    size_t completed = 0;

    std::string dir(cfg.TMP_DIR_PATH + "/process-storm");
    fs::mkdir(dir);
    fs::mkdir(dir + "/src");
    fs::mkdir(dir + "/dst");

    for (size_t i = 0; i < n; ++i)
    {
        std::string src(fmt::format("{:s}/src/{:d}", dir, i));
        std::string dst(fmt::format("{:s}/dst/{:d}", dir, i));
        CLOG(INFO, "Process") << "making file " << src;
        {
            std::ofstream out(src);
            out << i;
        }
        auto evt = app.getProcessManager().runProcess("mv " + src + " " + dst);
        evt.async_wait([&](asio::error_code ec)
                       {
                       CLOG(INFO, "Process") << "process exited: " << ec;
                       if (ec)
                       {
                           CLOG(DEBUG, "Process")
                               << "error code: " << ec.message();
                       }
                       ++completed;
                       });
    }

    while (completed < n && !clock.getIOService().stopped())
    {
        clock.crank(false);
        size_t n = app.getProcessManager().getNumRunningProcesses();
        CLOG(INFO, "Process") << "running subprocess count: " << n;
        REQUIRE(n <= cfg.MAX_CONCURRENT_SUBPROCESSES);
    }

    for (size_t i = 0; i < n; ++i)
    {
        std::string src(fmt::format("{:s}/src/{:d}", dir, i));
        std::string dst(fmt::format("{:s}/dst/{:d}", dir, i));
        REQUIRE(!fs::exists(src));
        REQUIRE(fs::exists(dst));
    }
}
#endif


