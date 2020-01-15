#include <errno.h>
#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sndfile.h>              /*  libsndfile is required */
#include <sched.h>
#include <jack/jack.h>

#include "foo.h"


/***/

t_group *add_group(char *group_name) {
  t_group *group;
  group = (t_group *) malloc(sizeof(t_group));
  group->name = (char *) malloc(strlen(group_name) + 1);
  strcpy(group->name, group_name);

  groups[group_count++] = (t_group *) group;
  
  return(group);
}

/***/

t_listener *add_listener(char *listener_name) {
  t_listener *listener;

  listener = (t_listener *) malloc(sizeof(t_listener));
  listener->name = (char *) malloc(strlen(listener_name) + 1);
  strcpy(listener->name, listener_name);

  listeners[listener_count++] = (t_listener *) listener;
  
  return(listener);
}


int process_jack_out(jack_nframes_t nframes, void *arg) {
  int i;
  stereo tempSample;
  t_group *group;
  char *group_name;

  jack_default_audio_sample_t *out_l;
  jack_default_audio_sample_t *out_r;

  group = arg;
  group_name = group->name;
  
  
  out_l = 
    (jack_default_audio_sample_t *) 
    jack_port_get_buffer(group->left_port, nframes);

  out_r =
    (jack_default_audio_sample_t *) 
    jack_port_get_buffer(group->right_port, nframes);

  for (i=0; i<nframes; i++) {
    /* left */
    out_l[i] = (jack_default_audio_sample_t) tempSample.l / 2;
    /* right */
    out_r[i] = (jack_default_audio_sample_t) tempSample.r / 2;
  }
  
  return 0;
}


int process_jack_in(jack_nframes_t nframes, void *arg) {
  t_listener *listener;
  listener = arg;
  
  
  
  return 0;
}

int
srate (jack_nframes_t nframes, void *arg)

{
  printf ("the sample rate is now %" PRIu32 "/sec\n", nframes);
  return 0;
}

void
error (const char *desc)
{
  fprintf (stderr, "JACK error: %s\n", desc);
}

void
jack_shutdown (void *arg)
{
  exit (1);
}

void
jack_init (void) {

  /* tell the JACK server to call error() whenever it
     experiences an error.
  */
  jack_set_error_function (error);
  init_jack_group("default");
}

int
init_jack_group (char *group_name)
{
  jack_client_t *client;
  const char **ports;
  char *jack_name;
  t_group *group;
 
  if (group_exists(group_name)) {
    printf("group %s already exists\n", group_name);
    return(0);
  }
  
  group = add_group(group_name);
  
  jack_name = 
    (char *) malloc(strlen(JACK_GROUP_PREFIX) + strlen(group_name) + 1);
  
  strcpy(jack_name, JACK_GROUP_PREFIX);
  strcat(jack_name, group_name);
  
  /* try to become a client of the JACK server */
  client = jack_client_new(jack_name);
  if ((client  == 0)) {
    fprintf (stderr, "jack server not running?\n");
    return(1);
  }
  
  /* tell the JACK server to call `process_jack_out()' whenever
     there is work to be done.
  */
  jack_set_process_callback (client, process_jack_out, (void *) group);
  
  /* tell the JACK server to call `srate()' whenever
     the sample rate of the system changes.
  */
  jack_set_sample_rate_callback (client, srate, 0);

  /* tell the JACK server to call `jack_shutdown()' if
     it ever shuts down, either entirely, or if it
     just decides to stop calling us.
  */
  jack_on_shutdown (client, jack_shutdown, 0);

  /* display the current sample rate. once the client is activated 
     (see below), you should rely on your own sample rate
     callback (see above) for this value.
  */
  printf ("engine sample rate: %" PRIu32 "\n",
	  jack_get_sample_rate (client));

  /* create output ports */
  group->left_port = 
    jack_port_register(client, "left", 
		       JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0
		      );
  group->right_port =
    jack_port_register(client, "right", 
		       JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0
		      );

  /* tell the JACK server that we are ready to roll */
  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client");
    return 1;
  }

  /* connect the ports. Note: you can't do this before
     the client is activated, because we can't allow
     connections to be made to clients that aren't
     running.
  */
  ports = 
    jack_get_ports(client, NULL, NULL, JackPortIsPhysical|JackPortIsInput);
  if (ports == NULL) {
    fprintf(stderr, "Cannot find any physical playback ports\n");
    exit(1);
  }

  if (jack_connect (client, jack_port_name (group->left_port), ports[0])) {
    fprintf (stderr, "cannot connect left port\n");
  }

  if (jack_connect (client, jack_port_name (group->right_port), ports[1])) {
    fprintf (stderr, "cannot connect right port\n");
  }
  
  free (ports);
  return 0;
}

int
init_jack_listener (char *sample_name, char *listener_name)
{
  jack_client_t *client;
  char *jack_name;
  t_listener *listener;
  const char **ports;
 
  if (listener_exists(listener_name)) {
    printf("listener %s already exists\n", listener_name);
    return(0);
  }
  
  listener = add_listener(listener_name);
  
  jack_name = 
    (char *) malloc(strlen(JACK_LISTENER_PREFIX) + strlen(listener_name) + 1);
  
  strcpy(jack_name, JACK_LISTENER_PREFIX);
  strcat(jack_name, listener_name);
  
  /* try to become a client of the JACK server */
  client = jack_client_new(jack_name);
  if ((client  == 0)) {
    fprintf (stderr, "jack server not running?\n");
    return(1);
  }
  
  /* tell the JACK server to call `process_jack_in()' whenever
     there is work to be done.
  */
  jack_set_process_callback (client, process_jack_in, (void *) listener);
  
  /* tell the JACK server to call `srate()' whenever
     the sample rate of the system changes.
  */
  jack_set_sample_rate_callback (client, srate, 0);

  /* tell the JACK server to call `jack_shutdown()' if
     it ever shuts down, either entirely, or if it
     just decides to stop calling us.
  */
  jack_on_shutdown (client, jack_shutdown, 0);

  /* display the current sample rate. once the client is activated 
     (see below), you should rely on your own sample rate
     callback (see above) for this value.
  */
  printf ("engine sample rate: %" PRIu32 "\n",
	  jack_get_sample_rate (client));

  /* create input port */
  listener->mono_port = 
    jack_port_register(client, "canal", 
		       JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0
		      );

  /* tell the JACK server that we are ready to roll */
  if (jack_activate (client)) {
    fprintf (stderr, "cannot activate client");
    return 1;
  }
  
  ports = 
    jack_get_ports(client, NULL, NULL, JackPortIsOutput);
  if (ports == NULL) {
    fprintf(stderr, "Cannot find any physical input ports\n");
  }
  else {
    if (jack_connect (client, jack_port_name (listener->mono_port), ports[0])) {
      fprintf (stderr, "cannot connect mono port\n");
    }
  }

  return 0;
}

