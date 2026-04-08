/**
 * @file main.cc
 * @author Nir Jacobson
 * @date 2026-04-08
 */

#include "application.h"

int main (int argc, char **argv)
{
    Application app(argc, argv);

    app.run();

    return 0;
}