#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <system_error>

using ErrorCode = std::error_code;

class Cli
{
    // Returns `err_c` and takes `std::vector<std::string>&` as an argument.
    using Command = std::function<ErrorCode(Cli*, const std::vector<std::string>&)>;

private:
    ErrorCode cd(const std::vector<std::string>& args);
    ErrorCode exit(const std::vector<std::string>& args);

public:
    Cli(std::string_view username, std::string_view machine_name,
        std::string_view directory = "~/")
        : m_prev_command_result(0, std::generic_category())
        , m_directory(directory)
        , m_username(username)
        , m_machine_name(machine_name)
    {
        m_builtin_commands.emplace("cd", &Cli::cd);
        m_builtin_commands.emplace("exit", &Cli::exit);
    }

    ErrorCode run();

private:
    ErrorCode execute(const std::string& command);

    [[nodiscard]] std::string advance() const;
    void print_interface() const;

    ErrorCode m_prev_command_result;
    std::string m_directory;
    const std::string m_username;
    const std::string m_machine_name;

    std::unordered_map<std::string, Command> m_builtin_commands;
};
