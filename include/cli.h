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

class Cli
{
    // Returns `err_c` and takes `std::vector<std::string>&` as an argument.
    using Command = std::function<ErrorCode(Cli*, const std::vector<std::string>&)>;

private:
    ErrorCode cd(const std::vector<std::string>& args) noexcept;
    ErrorCode exit([[maybe_unused]] const std::vector<std::string>& args) noexcept;
    ErrorCode clear([[maybe_unused]] const std::vector<std::string>& args) noexcept;
    // Takes the directory names you want to add to the path variable.
    ErrorCode path(const std::vector<std::string>& args) noexcept;
    ErrorCode echo(const std::vector<std::string>& args) noexcept;
    // Don't add $ before the variable name.
    ErrorCode export_var(const std::vector<std::string>& args) noexcept;

public:
    Cli(const std::string& username, std::string_view machine_name,
        std::string_view directory = "~/")
        : m_path{"/bin", "/usr/bin"}
        , m_prev_command_result(0, std::generic_category())
        , m_directory(directory)
        , m_username(username)
        , m_machine_name(machine_name)
        , m_builtin_commands{{"cd", &Cli::cd},       {"exit", &Cli::exit},
                             {"clear", &Cli::clear}, {"path", &Cli::path},
                             {"echo", &Cli::echo},   {"export", &Cli::export_var}}
        , m_environment{{"$HOME", "/home/" + username}, {"$PATH", "/bin /usr/bin"}}
    {
    }

    ErrorCode run();

private:
    ErrorCode execute(const std::string& command) noexcept;
    ErrorCode run_to_stdout(const std::string& binary,
                            const std::vector<std::string>& args);
    ErrorCode run_to_file(const std::string& binary, const std::vector<std::string>& args,
                          const std::string& filename);
    ErrorCode run_to_process(const std::string& binary,
                             const std::vector<std::string>& args,
                             const std::string& command);
    bool resolve_vars(std::vector<std::string>& args) const;
    std::string resolve_var(const std::string& variable) const;

    void update_path(const std::string& dir) noexcept;
    inline void print_interface() const;

    std::vector<bf::path> m_path;
    ErrorCode m_prev_command_result;
    std::string m_directory;
    const std::string m_username;
    const std::string m_machine_name;

    bool m_running = true;

    std::unordered_map<std::string, Command> m_builtin_commands;
    // Environment variables.
    std::unordered_map<std::string, std::string> m_environment;
};

inline ErrorCode success() noexcept;
inline ErrorCode failure() noexcept;
inline std::string advance() noexcept;

}
