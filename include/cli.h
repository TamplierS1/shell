#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <system_error>

#include "boost/filesystem.hpp"

using ErrorCode = std::error_code;

namespace Cli
{
namespace bf = boost::filesystem;

// TODO: add environment variables (and maybe implement a parser for the commands).
// TODO: add history.
// TODO: add `echo` command (impossible without adding variables).
// TODO: add customisable user and machine name.

class Cli
{
    // Returns `err_c` and takes `std::vector<std::string>&` as an argument.
    using Command = std::function<ErrorCode(Cli*, const std::vector<std::string>&)>;

private:
    ErrorCode cd(const std::vector<std::string>& args);
    ErrorCode exit(const std::vector<std::string>& args);
    ErrorCode clear(const std::vector<std::string>& args);
    // Takes the directory names you want to add to the path variable.
    ErrorCode path(const std::vector<std::string>& args);

public:
    Cli(std::string_view username, std::string_view machine_name,
        std::string_view directory = "~/")
        : m_path{"/bin", "/usr/bin"}
        , m_prev_command_result(0, std::generic_category())
        , m_directory(directory)
        , m_username(username)
        , m_machine_name(machine_name)
        , m_builtin_commands{{"cd", &Cli::cd},
                             {"exit", &Cli::exit},
                             {"clear", &Cli::clear},
                             {"path", &Cli::path}}
    {
    }

    ErrorCode run();

private:
    ErrorCode execute(const std::string& command);
    ErrorCode run_to_stdout(const std::string& binary,
                            const std::vector<std::string>& args);
    ErrorCode run_to_file(const std::string& binary, const std::vector<std::string>& args,
                          const std::string& filename);
    ErrorCode run_to_process(const std::string& binary,
                             const std::vector<std::string>& args,
                             const std::string& command);

    inline void print_interface() const;

    std::vector<bf::path> m_path;
    ErrorCode m_prev_command_result;
    std::string m_directory;
    const std::string m_username;
    const std::string m_machine_name;
    std::vector<std::string> m_history;

    bool m_running = true;

    std::unordered_map<std::string, Command> m_builtin_commands;
};

inline ErrorCode success();
inline ErrorCode failure();
inline std::string advance();

}
