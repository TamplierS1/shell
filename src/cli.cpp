#include <iostream>
#include <filesystem>
#include <sstream>

#include "boost/process.hpp"
#include "fmt/core.h"
#include "cli.h"

namespace fs = std::filesystem;
namespace bp = boost::process;

ErrorCode Cli::run()
{
    while (true)
    {
        m_directory = fs::current_path().string();
        print_interface();

        std::string command = advance();
        m_prev_command_result = execute(command);

        std::cout << "\n";
    }
}

ErrorCode Cli::execute(const std::string& command)
{
    const std::string& binary = command.substr(0, command.find(' '));

    if (m_builtin_commands.contains(binary))
    {
        // Extract arguments.
        std::stringstream stream{command.substr(command.find(' '))};
        std::string argument;
        std::vector<std::string> args;

        while (stream.good())
        {
            stream >> argument;
            args.emplace_back(argument);
        }

        return m_builtin_commands[binary](this, args);
    }

    try
    {
        bp::child process{command};
        process.wait();
        return ErrorCode{process.exit_code(), std::system_category()};
    }
    catch (std::system_error& e)
    {
        fmt::print(e.what());
        return e.code();
    }
}

std::string Cli::advance() const
{
    std::string command;
    std::getline(std::cin, command);
    return command;
}

void Cli::print_interface() const
{
    fmt::print("{}@{}: {}> [{}] ", m_username, m_machine_name, m_directory,
               m_prev_command_result.value());
}

ErrorCode Cli::cd(const std::vector<std::string>& args)
{
    // `cd` receives only 1 argument.
    assert(args.size() == 1);

    int result = chdir(args[0].c_str());
    if (result != 0)
    {
        switch (errno)
        {
            case EACCES:
                fmt::print("Permission denied.");
                break;
            case ENAMETOOLONG:
                fmt::print("The path was to long.");
                break;
            case ENOENT:
                fmt::print("The path does not exist.");
                break;
            case ENOTDIR:
                fmt::print("The path is not a directory.");
                break;
            default:
                fmt::print("Not every error case was handled: {}.", errno);
                break;
        }
        return ErrorCode(errno, std::system_category());
    }

    return ErrorCode(0, std::generic_category());
}

ErrorCode Cli::exit(const std::vector<std::string>& args)
{
    return ErrorCode();
}
