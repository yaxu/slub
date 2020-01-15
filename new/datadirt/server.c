#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <lo/lo.h>

#include "server.h"
#include "externs.h"

void error(int num, const char *m, const char *path);

int trigger_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, void *data, void *user_data);

int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data);

/**/

extern int server_init(void) {
  /* start a new server on port 7770 */
  lo_server_thread st = lo_server_thread_new("7770", error);
  
  //lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL);

  lo_server_thread_add_method(st, "/trigger", "sffffs", trigger_handler, NULL);
  lo_server_thread_start(st);
  
  return(1);
}

/**/

void error(int num, const char *msg, const char *path) {
    printf("liblo server error %d in path %s: %s\n", num, path, msg);
}

/**/

int generic_handler(const char *path, const char *types, lo_arg **argv,
		    int argc, void *data, void *user_data)
{
    int i;
    
    printf("path: <%s>\n", path);
    for (i=0; i<argc; i++) {
      printf("arg %d '%c' ", i, types[i]);
      lo_arg_pp(types[i], argv[i]);
      printf("\n");
    }
    printf("\n");

    return 1;
}

/**/

int trigger_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, void *data, void *user_data) {
  
  char *sample_name = strdup((char *) argv[0]);
  float speed  = argv[1]->f;
  float shape  = argv[2]->f;
  float pan    = argv[3]->f;
  float volume = argv[4]->f;
  char *envelope_name = strdup((char *) argv[5]);
  audio_trigger(sample_name, speed, shape, pan, volume, envelope_name);
  return 0;
}

