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

ErrorCode Cli::execute(const std::string& command) noexcept
{
    const std::string& binary = command.substr(0, command.find(' '));
    // Extract arguments.
    std::vector<std::string> args;

    size_t arg_start_indx = command.find(' ');
    if (arg_start_indx != std::string::npos)
    {
        std::stringstream stream{command.substr(command.find(' '))};
        std::string argument;
        bool is_first = true;

        while (stream.good())
        {
            stream >> argument;
            // Redirect the command output to a file.
            if (!is_first)
            {
                if (argument == ">")
                {
                    stream >> argument;
                    // NOTE: you cannot use env variables after `>` or `|`.
                    if (!resolve_vars(args))
                    {
                        return failure();
                    }

                    return run_to_file(binary, args, argument);
                }
                else if (argument == "|")
                {
                    // Redirect the command output to another command.
                    stream >> argument;
                    // NOTE: you cannot use env variables after `>` or `|`.
                    if (!resolve_vars(args))
                    {
                        return failure();
                    }

                    return run_to_process(binary, args, argument);
                }
            }

            args.emplace_back(argument);
            is_first = false;
        }
    }

    if (!resolve_vars(args))
    {
        return failure();
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

bool Cli::resolve_vars(std::vector<std::string>& args) const
{
    bool succeded = true;
    for (auto& arg : args)
    {
        if (arg[0] != '$')
            continue;

        try
        {
            arg = m_environment.at(arg);
        }
        catch (std::out_of_range& e)
        {
            fmt::print("{} is not an environment variable.\n", arg);
            succeded = false;
        }
    }

    return succeded;
}

std::string Cli::resolve_var(const std::string& variable) const
{
    std::string result;

    try
    {
        result = m_environment.at(variable);
    }
    catch (std::out_of_range& e)
    {
        fmt::print("{} is not an environment variable.\n", variable);
    }

    return result;
}

void Cli::update_path(const std::string& dir) noexcept
{
    m_path.emplace_back(dir);
    m_environment["$PATH"] = m_environment["$PATH"] + " " + dir;
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

ErrorCode Cli::cd(const std::vector<std::string>& args) noexcept
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

ErrorCode Cli::exit([[maybe_unused]] const std::vector<std::string>& args) noexcept
{
    m_running = false;
    return success();
}

ErrorCode Cli::clear([[maybe_unused]] const std::vector<std::string>& args) noexcept
{
    bp::system("clear");
    return success();
}

ErrorCode Cli::path(const std::vector<std::string>& args) noexcept
{
    for (auto& arg : args)
    {
        // Print the path variable.
        if (arg == "-s")
        {
            for (auto& dir : m_path)
            {
                fmt::print("{}\n", dir.string());
            }
            return success();
        }

        if (!bf::is_directory(arg))
        {
            fmt::print("{} is not a directory.\n", arg);
            continue;
        }

        update_path(arg);
    }

    return success();
}

ErrorCode Cli::echo(const std::vector<std::string>& args) noexcept
{
    for (auto& arg : args)
    {
        if (arg[0] == '$')
        {
            std::string result = resolve_var(arg);
            if (result.empty())
                return failure();
            fmt::print("{}", result);
        }
        else
        {
            fmt::print("{}", arg);
        }
    }

    return success();
}

ErrorCode Cli::export_var(const std::vector<std::string>& args) noexcept
{
    if (args.size() != 2)
    {
        fmt::print("usage: export [variable name] [variable value]\n");
        return failure();
    }

    m_environment.emplace("$" + args[0], args[1]);

    return success();
}

inline ErrorCode success() noexcept
{
    return ErrorCode(0, std::generic_category());
}

inline ErrorCode failure() noexcept
{
    return ErrorCode(1, std::generic_category());
}

inline std::string advance() noexcept
{
    std::string command;
    std::getline(std::cin, command);
    return command;
}

}
