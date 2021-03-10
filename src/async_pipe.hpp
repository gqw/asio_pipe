#ifndef AYNC_PIPE_HPP
#define AYNC_PIPE_HPP
#pragma once


#include <string>
#include <asio/windows/stream_handle.hpp>
#include <asio.hpp>

class async_pipe 
{
  asio::windows::stream_handle _read;
  asio::windows::stream_handle _write;
  bool _is_server = false;
  std::string _prefix;

public:
  using handle_type = asio::windows::stream_handle;

  inline async_pipe(asio::io_context& ios, const std::string& name, bool is_server);
  ~async_pipe();

  asio::error_code close();
  inline void async_close();

  void reopen();

  template <typename MutableBufferSequence, typename ReadHandler>
  void async_read(const MutableBufferSequence& buffers, ReadHandler&& handler)
  {
    asio::async_read(_read, buffers, std::forward<ReadHandler>(handler));
  }

  template <typename MutableBufferSequence, typename ReadHandler>
  auto async_read_some(const MutableBufferSequence& buffers, ReadHandler&& handler) {
    return _read.async_read_some(buffers, std::forward<ReadHandler>(handler));
  }

  template <typename ConstBufferSequence, typename WriteHandler>
  void async_write_some(const ConstBufferSequence& buffers,
                        WriteHandler&& handler) {
    _write.async_write_some(buffers, std::forward<WriteHandler>(handler));
  }
};

async_pipe::async_pipe(asio::io_context& ios, const std::string& name,
                       bool is_server)
    : _read(ios),
      _write(ios), _is_server(is_server), _prefix(name){
  reopen();
}

async_pipe::~async_pipe()
{
  close();
}

asio::error_code async_pipe::close()
{
  asio::error_code ec; 
  if (_write.is_open()) {
    _write.close(ec);
    _write = handle_type(_write.get_executor());
  }
  if (_read.is_open()) {
    _read.close(ec);
    _read = handle_type(_read.get_executor());
  }
  return ec;
}

void async_pipe::async_close()
{
  if (_write.is_open()) {
    asio::post(_write.get_executor(), [this]() { _write.close(); });
  }

  if (_read.is_open()) {
    asio::post(_read.get_executor(), [this]() { _read.close(); });
  }
}

void async_pipe::reopen()
{
  close();
  if (_is_server) {
    auto h_read = CreateNamedPipeA((_prefix + "_1").c_str(),
                                   PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                   0, 1, 8192, 8192, 0, nullptr);
    if (h_read == INVALID_HANDLE_VALUE) {
      throw std::logic_error(std::string("create source named pipe failed.") +
                             std::to_string(GetLastError()));
    }
    _read.assign(h_read);

    auto h_write = CreateNamedPipeA((_prefix + "_2").c_str(),
                                    PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
                                    0, 1, 8192, 8192, 0, nullptr);
    if (h_write == INVALID_HANDLE_VALUE) {
      throw std::logic_error(std::string("create sink named pipe failed.") +
                             std::to_string(GetLastError()));
    }
    _write.assign(h_write);
  } else {
    auto h_read =
        CreateFileA((_prefix + "_2").c_str(), GENERIC_WRITE, 0, nullptr,
                              OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (h_read == INVALID_HANDLE_VALUE) {
      throw std::logic_error(std::string("open source named pipe failed.") +
                             std::to_string(GetLastError()));
    }
    _read.assign(h_read);

    auto h_write =
        CreateFileA((_prefix + "_1").c_str(), GENERIC_WRITE, 0, nullptr,
                               OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (h_write == INVALID_HANDLE_VALUE) {
      throw std::logic_error(std::string("open sink named pipe failed.") +
                             std::to_string(GetLastError()));
    }
    _write.assign(h_write);
  }
}

#endif  // AYNC_PIPE_HPP