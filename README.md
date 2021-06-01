## shell

A basic UNIX-like shell.

## Dependencies

- [{fmt}](https://github.com/fmtlib/fmt) - a modern formatting library.
- [Boost.Process](https://www.boost.org/doc/libs/1_76_0/doc/html/process.html) - a Boost library that is used for 
  spawning external processes and the communication between them.
- [Boost.Filesystem](https://www.boost.org/doc/libs/1_69_0/libs/filesystem/doc/index.htm) - 
    `boost::process::child` constructor does not support `std::filesystem::path`. So I had to use this library to 
    obtain compatible paths.

Only Linux is supported.

## Build

1. `mkdir build`
2. `cd build`
3. `cmake ..`
4. `make`

## Usage

`cli [username] [machine name]`
