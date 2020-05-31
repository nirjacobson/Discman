#include <gtkmm.h>
#include <curlpp/cURLpp.hpp>

#include "cdplayer.h"

int main (int argc, char **argv)
{
  cURLpp::initialize();

  CDPlayer cdplayer(argc, argv);

  cdplayer.run();

  cURLpp::terminate();

  return 0;
}