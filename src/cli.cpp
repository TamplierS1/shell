#include <iostream>
#include <filesystem>

#include "boost/process.hpp"
#include "fmt/core.h"
#include "cli.h"

namespace Cli
{
namespace fs = std::filesystem;
namespace bp = boost::process;

ErrorCode Cli::run()
{
    while (m_running)
    {
        m_directory = fs::current_path().string();
        print_interface();

        std::string command = advance();
        m_prev_command_result = execute(command);

        std::cout << "\n";
    }

    return success();
}

ErrorCode Cli::execute(const std::string& command)
{
    const std::string& binary = command.substr(0, command.find(' '));

    // Extract arguments.
    std::vector<std::string> args;

    size_t arg_start_indx = command.find(' ');
    if (arg_start_indx != std::string::npos)
    {
        std::stringstream stream{command.substr(command.find(' '))};
        std::string argument;

        while (stream.good())
        {
            stream >> argument;
            args.emplace_back(argument);
        }
    }

    if (m_builtin_commands.contains(binary))
    {
        return m_builtin_commands[binary](this, args);
    }

    try
    {
        bf::path path_to_binary = bp::search_path(binary, m_path);
        if (path_to_binary.empty())
        {
            fmt::print("Failed to find command '{}'.\n", binary);
            return failure();
        }

        bp::child process{path_to_binary, args};
        process.wait();
        return ErrorCode{process.exit_code(), std::system_category()};
    }
    catch (std::system_error& e)
    {
        fmt::print(e.what());
        return e.code();
    }
}

inline void Cli::print_interface() const
{
    fmt::print("{}@{}: {}> [{}] ", m_username, m_machine_name, m_directory,
               m_prev_command_result.value());
}

ErrorCode Cli::cd(const std::vector<std::string>& args)
{
    if (args.size() != 1)
    {
        fmt::print("usage: cd [path_to_directory]\n");
        return failure();
    }

    std::string_view path = args[0];

    if (path == "~")
    {
        path = "/home/" + m_username;
    }

    int result = chdir(path.data());
    if (result != 0)
    {
        switch (errno)
        {
            case EACCES:
                fmt::print("Permission denied.\n");
                break;
            case ENAMETOOLONG:
                fmt::print("The path was to long.\n");
                break;
            case ENOENT:
                fmt::print("The path does not exist.\n");
                break;
            case ENOTDIR:
                fmt::print("The path is not a directory.\n");
                break;
            default:
                fmt::print("Not every error case was handled: {}.\n", errno);
                break;
        }
        return ErrorCode(errno, std::system_category());
    }

    return success();
}

ErrorCode Cli::exit(const std::vector<std::string>& args)
{
    m_running = false;
    return success();
}

ErrorCode Cli::clear(const std::vector<std::string>& args)
{
    bp::system("clear");
    return success();
}

inline ErrorCode success()
{
    return ErrorCode(0, std::generic_category());
}

inline ErrorCode failure()
{
    return ErrorCode(1, std::generic_category());
}

inline std::string advance()
{
    std::string command;
    std::getline(std::cin, command);
    return command;
}

}
