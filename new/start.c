#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include "externs.h"

/**/

int
main(int argc, char **argv) {
  server_init();
  audio_init();
  
  while(1) {
    sleep(1);
  }
  return(0);
}


