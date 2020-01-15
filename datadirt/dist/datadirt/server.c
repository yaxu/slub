/* Copyright (C) 2005 Alex McLean - alex@slab.org - http://yaxu.org/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

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
  
  /* lo_server_thread_add_method(st, NULL, NULL, generic_handler, NULL); */

  lo_server_thread_add_method(st, "/trigger", "sfffffsfffssffff",
                              trigger_handler, 
                              NULL
                             );
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
  float pan_to = argv[4]->f;
  float volume = argv[5]->f;
  char *envelope_name = strdup((char *) argv[6]);
  float anafeel_strength  = argv[7]->f;
  float anafeel_frequency = argv[8]->f;
  float accellerate = argv[9]->f;
  char *vowel_s = (char *) argv[10];
  char *scale_name = strdup((char *) argv[11]);
  float loops   = argv[12]->f;
  float duration   = argv[13]->f;
  float delay   = argv[14]->f;
  float delay2   = argv[15]->f;

  int vowel = -1;
  switch(vowel_s[0]) {
  case 'a': case 'A': vowel = 0; break;
  case 'e': case 'E': vowel = 1; break;
  case 'i': case 'I': vowel = 2; break;
  case 'o': case 'O': vowel = 3; break;
  case 'u': case 'U': vowel = 4; break;
  }
    
  audio_trigger(sample_name,
                speed,
                shape,
                pan,
                pan_to,
                volume,
                envelope_name,
                anafeel_strength,
                anafeel_frequency,
                accellerate,
                vowel,
                scale_name,
                loops,
                duration,
		delay,
		delay2
               );
  return 0;
}

