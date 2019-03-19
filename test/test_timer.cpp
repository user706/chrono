/****************************************************************************
**
**  Copyright (c) 2018 Du Yang
**
**    Author        : Du Yang < duyang.seu @ gmail.com >
**    File          : test_timer.cpp
**    Date          : 2018-12-21 17:13:40
**    Description   :
**
****************************************************************************/

#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <catch.hpp>

#include "timer.hpp"
using namespace Chrono;

#include <memory>
#include <iostream>
#include <thread>

class X {
public:
    X(int i) : i_(i) {
    }

    int &i() { return i_; }
private:
    int i_;
};

class A {
public:
    A(boost::asio::io_context &ioc) : i_(111), spi_(std::make_shared<X>(222)), timer_{ioc, std::chrono::steady_clock::now() + std::chrono::milliseconds(1000)}
    {
        std::cout << "i_        == " << i_    << std::endl;
        std::cout << "spi_->i() == " << spi_->i() << std::endl;
        timer_.Start(
                    [&]() {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        std::cout << "i_        => " << i_    << std::endl;
                        std::cout << "spi_->i() => " << spi_->i() << std::endl; });
    }
    
private:
    int i_;
    std::shared_ptr<X> spi_;
    PointTimer<std::chrono::hours, boost::asio::io_context, boost::asio::steady_timer> timer_;
};


SCENARIO(
    "lifetime issues",
    "[timer000][common][timer]")
{
  auto second_from_now =
      std::chrono::system_clock::now() + std::chrono::seconds{1};
  std::chrono::system_clock::time_point expire_time;

  boost::asio::io_context ioc;
  std::thread th([&ioc]()
                 {
                     A a(ioc);

#define DO_FAIL
//#undef DO_FAIL
                     
#ifdef DO_FAIL
                     std::chrono::milliseconds delay(1010);
#else
                     std::chrono::milliseconds delay(1100);
#endif
                     
                     std::this_thread::sleep_for(delay);
                 });

  std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait a bit for the thread above the start, so that ioc has work to do, instead of falling through when ioc.run() is called below

  ioc.run();
  th.join();
}


SCENARIO(
    "test point timer expire at specific a time point",
    "[timer001][common][timer]")
{
  auto second_from_now =
      std::chrono::system_clock::now() + std::chrono::seconds{1};
  std::chrono::system_clock::time_point expire_time;

  boost::asio::io_context ioc;
  PointTimer<std::chrono::hours> timer{ioc, second_from_now};
  timer.Start(
      [&expire_time]() { expire_time = std::chrono::system_clock::now(); });
  ioc.run();

  REQUIRE((expire_time - second_from_now) < std::chrono::milliseconds{1});
}

SCENARIO(
    "test point timer repeats at 3 times with 1 second interval",
    "[timer002][common][timer]")
{
  const auto second_from_now =
      std::chrono::system_clock::now() + std::chrono::seconds{1};
  const auto three_seconds_from_now = second_from_now + std::chrono::seconds{2};
  std::chrono::system_clock::time_point expire_time;

  boost::asio::io_context ioc;
  PointTimer<std::chrono::seconds> timer{ioc, second_from_now};
  auto repeat = 3u;
  timer.Start([&timer, &expire_time, &repeat]() {
    expire_time = std::chrono::system_clock::now();
    --repeat;
    if (!repeat)
    {
      timer.Recursive(false);
    }
  });
  ioc.run();

  REQUIRE(
      (expire_time - three_seconds_from_now) < std::chrono::milliseconds{1});
}

SCENARIO("test point timer with ptime support", "[timer003][common][timer]")
{
  using namespace boost::posix_time;
  auto second_from_now = second_clock::universal_time() + seconds{1};
  ptime expire_time;

  boost::asio::io_context ioc;
  PointTimer<hours, boost::asio::io_context, boost::asio::deadline_timer> timer{
      ioc, second_from_now};
  timer.Start(
      [&expire_time]() { expire_time = second_clock::universal_time(); });
  ioc.run();

  REQUIRE((expire_time - second_from_now) < milliseconds{1});
}

SCENARIO(
    "test duration timer expire after 1 second", "[timer004][common][timer]")
{
  auto start_time = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point expired_time;

  boost::asio::io_context ioc;
  DurationTimer<std::chrono::duration<double>> timer{ioc, 1, false};
  timer.Start(
      [&expired_time]() { expired_time = std::chrono::steady_clock::now(); });
  ioc.run();

  REQUIRE(
      (expired_time - start_time - std::chrono::seconds{1}) <
      std::chrono::milliseconds{1});
}

SCENARIO(
    "test duration timer expire repeats 3 times with 1 second interval",
    "[timer005][common][timer]")
{
  auto start_time = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point expired_time;

  boost::asio::io_context ioc;
  DurationTimer<std::chrono::duration<double>> timer{ioc, 1, true};
  auto repeat = 3u;
  timer.Start([&expired_time, &repeat, &timer]() {
    expired_time = std::chrono::steady_clock::now();
    --repeat;
    if (!repeat)
    {
      timer.Recursive(false);
    }
  });
  ioc.run();

  REQUIRE(
      (expired_time - start_time - std::chrono::seconds{3}) <
      std::chrono::milliseconds{1});
}
