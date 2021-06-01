#include "cli.h"
#include <fmt/core.h>

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        fmt::print("usage: cli [username] [machine name]\n");
        return EXIT_FAILURE;
    }

    Cli::Cli cli{argv[1], argv[2]};

    return cli.run().value();
}
