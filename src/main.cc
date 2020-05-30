#include <gtkmm.h>

#include "cdplayer.h"

int main (int argc, char **argv)
{
  CDPlayer cdplayer(argc, argv);

  cdplayer.run();

  return 0;
}