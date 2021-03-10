// test2.cpp : This file contains the 'main' function. Program execution begins
// and ends there.
//

#include <chrono>
#include <iostream>



#include "async_pipe.hpp"

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>
#include <asio/write.hpp>
#include <asio.hpp>
#include <asio/strand.hpp>

using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using asio::ip::tcp;
namespace this_coro = asio::this_coro;

std::string buffer;
std::shared_ptr<async_pipe> pipe;
std::vector<asio::executor_work_guard<asio::io_context::executor_type>> g_works;


int g_index = 0;

void deal_read(asio::error_code ec, std::size_t size) {
  do
  {
    if (ec) {
      if (ec.value() == ERROR_PIPE_LISTENING) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        break;
      }
      if (ec.value() == ERROR_BROKEN_PIPE) {
        std::cout << "pipe ended" << std::endl;
        pipe->reopen();
        std::this_thread::sleep_for(std::chrono::seconds(2));

        break;
      }

      std::cout << ec.message() << " " << ec.value() << " "
                << ec.category().name() << " "
                << ec.category().message(ec.value()) << std::endl;
    }
  } while (false);
  if (size > 0) {
    std::cout << ++g_index << " " << buffer.substr(0, size) << std::endl;
  }

  pipe->async_read_some(
      asio::buffer((char*)buffer.data(), buffer.length()), deal_read);
}

void deal_write(asio::error_code ec, std::size_t size) {
  if (ec) {
    std::cout << ec.message() << " " << ec.value() << " "
              << ec.category().name() << " "
              << ec.category().message(ec.value()) << std::endl;
  }
  if (size > 0) {
    std::cout << buffer.substr(0, size) << std::endl;
  }

  std::string write_buffer;
  std::cin >> write_buffer;

  pipe->async_write_some(
      asio::buffer(write_buffer.data(), write_buffer.length()), deal_write);
}

awaitable<void> do_read()
{
  do
  {
    try
    {
      std::size_t size = co_await pipe->async_read_some(
          asio::buffer((char*)buffer.data(), buffer.length()), use_awaitable);
      if (size > 0) {
        std::cout << ++g_index << " " << std::this_thread::get_id() << " "  << buffer.substr(0, size) << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
      }
    } catch (std::system_error ec) {
      if (ec.code().value() == ERROR_PIPE_LISTENING) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
      }
      if (ec.code().value() == ERROR_BROKEN_PIPE) {
        std::cout << "pipe ended" << std::endl;
        pipe->reopen();
        std::this_thread::sleep_for(std::chrono::seconds(2));
      }
      std::cout << ec.code().message() << " " << ec.code().value() << " "
                << ec.code().category().name() << std::endl;
    } catch (...) {
      std::cout << "unknow exception" << std::endl;
    }

  } while (true);

}

awaitable<void> listener() {
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), 55555});
  tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
  //for (;;) {
  //  tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
  // /* co_spawn(
  //      executor,
  //      [socket = std::move(socket)]() mutable {
  //        return echo(std::move(socket));
  //      },
  //      detached);*/
  //}
}


int main(int argc, char**argv) {
  buffer.resize(1024);

  try {
    asio::io_context io_context;

    if (std::string(argv[1]) == "s")
    {
      pipe = std::make_shared<async_pipe>(
          io_context, R"(\\.\pipe\boltrend_process_pipe_test)", true);
      std::cout << "server prepare read, ..." << std::endl;
      co_spawn(io_context, do_read, detached);

      // g_works.push_back(asio::make_work_guard(io_context));
      // co_spawn(io_context, listener, detached);
    }

    if (std::string(argv[1]) == "c") {
      pipe = std::make_shared<async_pipe>(
          io_context, R"(\\.\pipe\boltrend_process_pipe_test)", false);
      std::cout << "client prepare write, ..." << std::endl;
      std::string write_buffer;
      std::cin >> write_buffer;

      pipe->async_write_some(
          asio::buffer(write_buffer.data(), write_buffer.length()), deal_write);
    }
    std::thread t1([&]() { io_context.run(); });
    std::thread t2([&]() { io_context.run(); });
    t1.join();
    t2.join();
  } catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }
}
