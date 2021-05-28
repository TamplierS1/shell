#include "cli.h"

int main()
{
    Cli::Cli cli{"azamat", "aza_machine"};

    return cli.run().value();
}
