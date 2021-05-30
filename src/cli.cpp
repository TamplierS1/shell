#include <iostream>
#include <filesystem>

#include "boost/process.hpp"
#include "fmt/core.h"
#include "fmt/color.h"
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
    m_history.push_back(command);

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
            // Redirect the command output to a file.
            if (argument == ">")
            {
                stream >> argument;
                return run_to_file(binary, args, argument);
            }
            else if (argument == "|")
            {
                // Redirect the command output to another command.
                stream >> argument;
                return run_to_process(binary, args, argument);
            }

            args.emplace_back(argument);
        }
    }

    return run_to_stdout(binary, args);
}

ErrorCode Cli::run_to_stdout(const std::string& binary,
                             const std::vector<std::string>& args)
{
    if (m_builtin_commands.contains(binary))
    {
        return m_builtin_commands[binary](this, args);
    }
    else
    {
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
}

ErrorCode Cli::run_to_file(const std::string& binary,
                           const std::vector<std::string>& args,
                           const std::string& filename)
{
    if (m_builtin_commands.contains(binary))
    {
        fmt::print("Builtin-commands cannot be redirected.\n");
        return failure();
    }

    try
    {
        bf::path path_to_binary = bp::search_path(binary, m_path);
        if (path_to_binary.empty())
        {
            fmt::print("Failed to find command '{}'.\n", binary);
            return failure();
        }

        bp::child process{path_to_binary, args, bp::std_out > filename,
                          bp::std_err > filename};
        process.wait();
        return ErrorCode{process.exit_code(), std::system_category()};
    }
    catch (std::system_error& e)
    {
        fmt::print(e.what());
        return e.code();
    }
}

ErrorCode Cli::run_to_process(const std::string& binary,
                              const std::vector<std::string>& args,
                              const std::string& command)
{
    if (m_builtin_commands.contains(binary))
    {
        fmt::print("Builtin-commands cannot be redirected.\n");
        return failure();
    }

    try
    {
        bf::path path_to_binary1 = bp::search_path(binary, m_path);
        if (path_to_binary1.empty())
        {
            fmt::print("Failed to find command '{}'.\n", binary);
            return failure();
        }

        // Parse the second command.
        const std::string& binary2 = command.substr(0, command.find(' '));

        // Extract arguments.
        std::vector<std::string> args2;

        size_t arg_start_indx = command.find(' ');
        if (arg_start_indx != std::string::npos)
        {
            std::stringstream stream{command.substr(command.find(' '))};
            std::string argument;

            while (stream.good())
            {
                stream >> argument;
                // Redirect the command output to a file.
                if (argument == ">")
                {
                    stream >> argument;
                    return run_to_file(binary2, args2, argument);
                }
                else if (argument == "|")
                {
                    // Redirect the command output to another command.
                    stream >> argument;
                    return run_to_process(binary2, args2, argument);
                }

                args2.emplace_back(argument);
            }
        }

        bf::path path_to_binary2 = bp::search_path(binary2, m_path);
        if (path_to_binary2.empty())
        {
            fmt::print("Failed to find command '{}'.\n", binary2);
            return failure();
        }

        bp::pipe p;
        bp::child process1{path_to_binary1, args, bp::std_out > p};
        bp::child process2{path_to_binary2, args2, bp::std_in < p};

        process1.wait();
        process2.wait();
        return ErrorCode{process1.exit_code(), std::system_category()};
    }
    catch (std::system_error& e)
    {
        fmt::print(e.what());
        return e.code();
    }
}

inline void Cli::print_interface() const
{
    fmt::print(fmt::emphasis::bold | fg(fmt::color::gold), "{}", m_username);
    fmt::print("@");
    fmt::print(fmt::emphasis::bold | fg(fmt::color::gold), "{}: ", m_machine_name);
    fmt::print(fmt::emphasis::bold | fg(fmt::color::dodger_blue), "{}", m_directory);
    fmt::print(fg(fmt::color::red), " [{}]", m_prev_command_result.value());
    fmt::print(" $ ");
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

ErrorCode Cli::path(const std::vector<std::string>& args)
{
    for (auto& arg : args)
    {
        if (!bf::is_directory(arg))
        {
            fmt::print("{} is not a directory.\n", arg);
            continue;
        }
        m_path.emplace_back(arg);
    }

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
