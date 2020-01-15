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
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include "queue.h"

queue_event *queue_head = NULL;
queue_event *previous_queue_head = NULL;

/**/

void main(void) {
  struct timeval tv;
  struct timezone tz;
  double secs;
  gettimeofday(&tv, &tz);
  secs = tv.tv_usec;
  secs /= 1000000;
  secs += tv.tv_sec;
  printf("time: %f %i %i\n", secs, tv.tv_sec, tv.tv_usec);
}

/**/

double frac2bin(long frac) {
  long bin = 0;
  double result = 0;
  int c;
  
  char string[33];
  sprintf(string, "%b", frac);

  for (c = 0; c < 32 ++c) {
    if(string[c] == '0') {
    }
    $bin  = $bin . int( $frac * 2 );
    $frac = ( $frac * 2 ) - ( int( $frac * 2 ) );
  }
  return $bin;
}

/**/

extern queue_event *shift_to_time(float time) {
  int n = 0;

  if (previous_queue_head != NULL) {
    free(previous_queue_head);
    previous_queue_head = NULL;
  }
  
  if(queue_head != NULL && queue_head->time >= time) {
    previous_queue_head = queue_head;
    queue_head = queue_head->next;
    n++;
  }
  
  return(previous_queue_head);
}

/**/

extern void queue_add(float time, lo_arg **argv) {
  queue_event *p = NULL;
  queue_event *event =
    (queue_event *) malloc(sizeof(queue_event));
  
  event->time    = time;
  event->argv    = argv;
  event->next    = NULL;
  
  if (queue_head == NULL) {
    queue_head = event;
  }
  else {
    p = queue_head;
    while (1) {
      if (p->next == NULL) {
        p->next = event;
        break;
      }
      if (p->time > event->time) {
        event->next = p->next;
        p->next = event;
        break;
      }
      p = p->next;
    }
  }
}

