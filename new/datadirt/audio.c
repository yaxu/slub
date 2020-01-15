#include <jack/jack.h>
#include <sndfile.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#include <alsa/asoundlib.h>
#include <sched.h>

#include "audio.h"

/**/

int set_realtime_priority(void) {
  struct sched_param schp;
  /*
   * set the process to realtime privs
   */
  memset(&schp, 0, sizeof(schp));
  schp.sched_priority = sched_get_priority_max(SCHED_FIFO);
  
  if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0) {
    perror("sched_setscheduler");
    return -1;
  }
  
  return 0;
}

/**/

float effect_waveshaper(float x, float a) {
  
  float z, s, b;
  z = M_PI * a;
  s = 1/sin(z);
  b = 1/a;
  
  if (x > b) {
    x = 1;
  }
  else {
    x = sin(z*x)*s;
  }
  return(x);
}

/**/

void compress
    (
        float  *wav_in,     // signal
        int     n,          // N samples
        double  threshold,  // threshold (0 .. 1)
        double  slope,      // slope angle (0 .. 1)
        double  tla,        // lookahead  (ms)
        double  twnd,       // window time (ms)
        double  tatt,       // attack time  (ms)
        double  trel        // release time (ms)
    )
{
    int sr = 44100;
    typedef float   stereodata[2];
    stereodata*     wav = (stereodata*) wav_in; // our stereo signal
    tla *= 1e-3;                // lookahead time to seconds
    twnd *= 1e-3;               // window time to seconds
    tatt *= 1e-3;               // attack time to seconds
    trel *= 1e-3;               // release time to seconds

    // attack and release "per sample decay"
    double  att = (tatt == 0.0) ? (0.0) : exp (-1.0 / (sr * tatt));
    double  rel = (trel == 0.0) ? (0.0) : exp (-1.0 / (sr * trel));

    // envelope
    double  env = 0.0;

    // sample offset to lookahead wnd start
    int     lhsmp = (int) (sr * tla);

    // samples count in lookahead window
    int     nrms = (int) (sr * twnd);
    
    double summ, smp;
    
    int i, j;
    
    // for each sample...
    for (i = 0; i < n; ++i)
    {
        // now compute RMS
        summ = 0;

        // for each sample in window
        for (j = 0; j < nrms; ++j)
        {
            int     lki = i + j + lhsmp;
            smp;

            // if we in bounds of signal?
            // if so, convert to mono
            if (lki < n)
                smp = 0.5 * wav[lki][0] + 0.5 * wav[lki][1];
            else
                smp = 0.0;      // if we out of bounds we just get zero in smp
            
            summ += smp * smp;  // square em..
        }
        
        double  rms = sqrt (summ / nrms);   // root-mean-square

        // dynamic selection: attack or release?
        double  theta = rms > env ? att : rel;

        // smoothing with capacitor, envelope extraction...
        // here be aware of pIV denormal numbers glitch
        env = (1.0 - theta) * rms + theta * env;

        // the very easy hard knee 1:N compressor
        double  gain = 1.0;
        if (env > threshold)
            gain = gain - (env - threshold) * slope;

        // result - two hard kneed compressed channels...
        float  leftchannel = wav[i][0] * gain;
        float  rightchannel = wav[i][1] * gain;
    }
}

/**/

float envelope_value (t_envelope *envelope, float point) {
  float start, stop, difference, tween_amount, tween;
  
  start = envelope->values[(int) point];
  if ((((int) point) + 1) >= envelope->size) {
    tween = 0;
  }
  else {
    /* linear interpolation */
    tween_amount = (point - (int) point);
    stop = envelope->values[((int) point) + 1];
    difference = stop - start;
    tween = difference * tween_amount;
  }
  
  return(start + tween);
}

/**/

void print_envelope(t_envelope *envelope) {
  int i, c;
  float value, step;
  step = ((float) envelope->size - 1) / 24.0;
  for (i = 0; i < 25; ++i) {
    value = envelope_value(envelope, step * i);
    value *= 70;
    printf("%0.2f|", step * i);
    for (c = 0; c < value; ++c) {
      printf("x");
    }
    printf("\n");
  }
}

/**/

void build_envelope_table (t_envelope *envelope, int table_size) {
  int i;
  float step;
  t_table *table = (t_table *) malloc(sizeof(t_table));
  table->size = table_size;
  
  table->values = (float *) malloc(sizeof(float) * table->size);
  step = ((float) envelope->size - 1) / (float) (table->size - 1);
  for (i = 0; i < table->size; ++i) {
    table->values[i] = envelope_value(envelope, i * step);
  }
  envelope->table = table;
}

/**/

t_envelope *build_envelope (int size, float *values) {
  int i;
  t_envelope *envelope = (t_envelope *) malloc(sizeof(t_envelope));

  envelope->values = (float *) malloc(sizeof(float) * size);
  
  envelope->size = size;
  
  for (i = 0; i < size; ++i) {
    envelope->values[i] = values[i];
  }
  envelope->table = (t_table *) NULL;

  return(envelope);
}

/**/

t_sample *find_sample (char *samplename) {
  int c;
  t_sample *sample = NULL;
  
  for(c = 0; c < sample_count; ++c) {
    if(strcmp(samples[c]->name, samplename) == 0) {
      sample = samples[c];
      break;
    }
  }
  return(sample);
}

/**/

t_sample *get_sample(char *samplename) {
  SNDFILE *sndfile;
  char path[MAXPATHSIZE];

  t_sample *sample;
  sf_count_t count;
  float *frames;
  SF_INFO *info;
  
  sample = find_sample(samplename);
  
  if (sample == NULL) {
    /* load it from disk */
    snprintf(path, MAXPATHSIZE -1, "%s/%s", SAMPLEROOT, samplename);
    warn("load %s\n", path);
    info = (SF_INFO *) calloc(1, sizeof(SF_INFO));
    
    
    if ((sndfile = (SNDFILE *) sf_open(path, SFM_READ, info)) == NULL) {
      free(info);
    }
    else {
      frames = (float *) malloc(sizeof(float) * info->frames);
      count  = sf_read_float(sndfile, frames, info->frames);
      if (count == info->frames) {
        sample = (t_sample *) calloc(1, sizeof(t_sample));
        strncpy(sample->name, samplename, MAXPATHSIZE - 1);
        sample->info = info;
        sample->frames = frames;
        samples[sample_count++] = sample;
      }
      else {
        free(info);
        free(frames);
        perror("Didn't the right number of frames");
      }
    }
    
  }
  return(sample);
}

/**/

void process_frame(float *l, float *r) {
  int c;

  float start, stop, difference, tween_amount, tween, lvolume, rvolume;
  t_head *head;
  
  float loudest;
  
  for (c=0; c < MAXHEADS; c++) {
    head = &heads[c];
    if (head->active) {
      start = head->sample->frames[(int) head->position];
      if (head->env_volume) {
        lvolume = rvolume = 
          envelope_value(head->env_volume, 
                         (head->position / head->sample->info->frames) 
                         * head->env_volume->size
                         );
      }
      else {
        lvolume = rvolume = 1;
      }
      lvolume *= head->volume;
      rvolume *= head->volume;
      if (head->pan < 0) {
        lvolume *= (1 + head->pan);
      }
      if (head->pan > 0) {
        rvolume *= (1 - head->pan);
      }
      
      if ((((int) head->position) + 1) >= head->sample->info->frames) {
        tween = 0;
      }
      else {
        /* linear interpolation */
        stop = head->sample->frames[((int) head->position) + 1];
        tween_amount = (head->position - (int) head->position);
        difference = stop - start;
        tween = difference * tween_amount;
      }
      
      head->position += head->speed;
      *l += ((start + tween) * lvolume);
      if (head->shape > 0.0) {
        *l = effect_waveshaper(*l, head->shape);
      }
      
      if (head->sample->info->channels > 1) {
        /* Assume it's a stereo sample */
        if (head->position >= head->sample->info->frames) {
          head->position = 0;
          head->active = 0;
          continue;
        }
        if (head->position < 0) {
          head->position = 0;
          head->active = 0;
          continue;
        }
        
        start = head->sample->frames[(int) head->position];
        if ((((int) head->position) + 1) >= head->sample->info->frames) {
          tween = 0;
        }
        else {
          stop = head->sample->frames[((int) head->position) + 1];
          tween_amount = (head->position - (int) head->position);
          difference = stop - start;
          tween = difference * tween_amount;
        }
        
        head->position += head->speed;
      }
      *r += ((start + tween) * rvolume);
      if (head->shape > 0.0) {
        *r = effect_waveshaper(*r, head->shape);
      }
      
      if (head->position >= head->sample->info->frames) {
        head->position = 0;
        head->active = 0;
        continue;
      }
      if (head->position < 0) {
        head->position = 0;
        head->active = 0;
        continue;
      }
    }
  }
  
  compressor += (float) COMPRESSORGAIN / 100000;

  /*  Get the loudest of the two channels (for the compressor, which is mono) */
  loudest = (fabs(*r) > fabs(*l) 
             ? fabs(*r)
             : fabs(*l) 
            );
  if (fabs(loudest * compressor) > 1.0) {
    compressor = compressor / fabs(loudest * compressor);
  }
  *l *= compressor;
  *r *= compressor;

  if (*l >  1.0) { 
    *l =  1.0;
  }
  else if (*l <  -1.0) { 
    *l =  -1.0;
  }
  if (*r >  1.0) { 
    *r =  1.0;
  }
  else if (*r <  -1.0) { 
    *r =  -1.0;
  }
    
}

int process_jack_out(jack_nframes_t nframes, void *arg) {
  int i;
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
    out_l[i] = 0;
    out_r[i] = 0;
    process_frame(&out_l[i], &out_r[i]);
  }
  
  return 0;
}

/**/

t_group *add_group(char *group_name) {
  t_group *group;
  group = (t_group *) malloc(sizeof(t_group));
  group->name = (char *) malloc(strlen(group_name) + 1);
  strcpy(group->name, group_name);

  groups[group_count++] = (t_group *) group;
  
  return(group);
}

/**/

int group_exists(char *group_name) {
  int i;
  int result = 0;
  if (group_count > 0) {
    for(i = 0; i < group_count; ++i) {
      if (strcmp(groups[i]->name, group_name) == 0) {
        result = 1;
        break;
      }
    }
  }
  return(result);
}

/**/

void jack_error (const char *desc) {
  fprintf(stderr, "JACK error: %s\n", desc);
}

/**/

void build_envelopes() {
  float tri[3]        = {0, 1, 0};
  float percussive[19] = 
    {0, 1, 1, 1, 1, 1, 0.75, 0.7, 0.65, 0.6, 0.55, 
     0.5, 0.4, 0.3, 0, 0.3, 0.3, 0.25, 0};

  float valley[5]     = {0, 1, 0.5, 1, 0};

  env_tri        = build_envelope(3, tri       );
  print_envelope(env_tri);
  
  env_percussive = build_envelope(19, percussive);
  print_envelope(env_percussive);
  
  env_valley     = build_envelope(5, valley    );
  print_envelope(env_valley);
  
}

/**/

void alsa_loop() {
  snd_pcm_t *pcm_handle;
  snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
  snd_pcm_hw_params_t *hwparams;
  char *pcm_name;
  int rate = 44100; /* Sample rate */
  int exact_rate;   /* Sample rate returned by
                     * snd_pcm_hw_params_set_rate_near
                     */ 
  int dir;          /* exact_rate == rate --> dir = 0 
                     * exact_rate < rate  --> dir = -1
                     * exact_rate > rate  --> dir = 1 
                     */
  int periods = 2;  /* Number of periods */
  snd_pcm_uframes_t periodsize = 8192; /* Periodsize (bytes) */
  unsigned char *data;
  float l, r;
  short sl, sr;
  int i, c, frames;
  int err;
  
  /* TODO - make this configurable */
  pcm_name = strdup("hw:2");
  snd_pcm_hw_params_alloca(&hwparams);
  if (snd_pcm_open(&pcm_handle, pcm_name, stream, 0) < 0) {
    fprintf(stderr, "Error opening PCM device %s\n", pcm_name);
    exit(1);
  }
  if (snd_pcm_hw_params_any(pcm_handle, hwparams) < 0) {
    fprintf(stderr, "Can not configure this PCM device.\n");
    exit(1);
  }
  if (snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
    fprintf(stderr, "Error setting access.\n");
    exit(1);
  }
  /* Set sample format */
  if ((err = snd_pcm_hw_params_set_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_LE)) < 0) {
    fprintf(stderr, "Error setting format (%s).\n", snd_strerror (err));
    exit(1);
  }
  
  /* Set sample rate. If the exact rate is not supported */
  /* by the hardware, use nearest possible rate.         */ 
  exact_rate = rate;
  if (snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &exact_rate, 0) < 0) {
    fprintf(stderr, "Error setting rate.\n");
    exit(1);
  }
  if (rate != exact_rate) {
    fprintf(stderr, "The rate %d Hz is not supported by your hardware.\n==> Using %d Hz instead.\n", rate, exact_rate);
  }
  
  /* Set number of channels */
  if (snd_pcm_hw_params_set_channels(pcm_handle, hwparams, 2) < 0) {
    fprintf(stderr, "Error setting channels.\n");
    exit(1);
  }
  
  /* Set number of periods. Periods used to be called fragments. */ 
  if (snd_pcm_hw_params_set_periods(pcm_handle, hwparams, periods, 0) < 0) {
    fprintf(stderr, "Error setting periods.\n");
    exit(1);
  }
  /* Set buffer size (in frames). The resulting latency is given by */
  /* latency = periodsize * periods / (rate * bytes_per_frame)     */
  if (snd_pcm_hw_params_set_buffer_size(pcm_handle, hwparams, 
                                        (periodsize * periods)>>2) < 0
      ) {
    fprintf(stderr, "Error setting buffersize.\n");
    exit(1);
  }
  /* Apply HW parameter settings to */
  /* PCM device and prepare device  */
  if (snd_pcm_hw_params(pcm_handle, hwparams) < 0) {
    fprintf(stderr, "Error setting HW params.\n");
    exit(1);
  }

  data = (short *)malloc(periodsize);
  frames = periodsize >> 2;
  while (1) {
    c = 0;
    for(i = 0; i < (frames / 2); i++) {
      l = r = 0;
      process_frame(&l, &r);
      sl = (short) l * 32767;
      sr = (short) r * 32767;
      data[4 * i    ] = (unsigned char) sl;
      data[4 * i + 1] = sl >> 8;
      data[4 * i + 2] = (unsigned char) sr;
      data[4 * i + 3] = sr >> 8;
    }
    while ((snd_pcm_writei(pcm_handle, data, frames)) < 0) {
      snd_pcm_prepare(pcm_handle);
      fprintf(stderr, "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
    }
  }
}

/**/

void dsp_loop() {
  /*  3 fragments (???), 4kb buffer (8192=2 ^ 0x0D) */
  int setting   = 0x0003000B; 
  int schannels = 1;
  int format    = AFMT_S16_LE;
  int samplerate = 44100;
  int fh;
  int i;
  short *buf, *bufp;
  float l, r;
  l = r = 0;
  if ((fh=open("/dev/dsp",O_WRONLY)) == -1) { 
    printf("Problem opening /dev/dsp");
    exit(1);
  }
  if (ioctl(fh,SNDCTL_DSP_SETFRAGMENT,&setting   ) == -1) { 
    printf("Failed SNDCTL_DSP_SETFRAGMENT"); 
    exit(1);
  }
  if (ioctl(fh,SNDCTL_DSP_STEREO, &schannels ) == -1) {
    printf("Failed SNDCTL_DSP_STEREO");
    exit(1);
  }
  if (ioctl(fh,SNDCTL_DSP_SETFMT     ,&format    ) == -1) { 
    printf("Failed SNDCTL_DSP_SETFMT");
    exit(1);
  }
  if (ioctl(fh,SNDCTL_DSP_SPEED      ,&samplerate) == -1) {
    printf("Failed SNDCTL_DSP_SPEED");
    exit(1);
  }
  
  buf = malloc(BUFFERSIZE * 2 * 2L);
 
  set_realtime_priority();
  
  while (1) {
    bufp = buf;
    for (i=0; i<BUFFERSIZE; i++) {
      l = r = 0;
      process_frame(&l, &r);
      *bufp++ = (short) (l * 32767.0);
      *bufp++ = (short) (r * 32767.0);
    }
    write(fh, buf, BUFFERSIZE * 2 * 2);
  }
}

/**/

void alsa_thread() {
  pthread_t audio_thread_handle;
  if (pthread_create(&audio_thread_handle, NULL, dsp_loop, NULL) != 0) { 
    perror("Couldn't create thread");
    exit(1);
  }
  
  if (pthread_detach(audio_thread_handle)!= 0) {
    perror("Couldn't detach thread");
    exit(1);
  }
}


/**/

extern int audio_init(void) {
  sample_count = 0;
  head_count   = 0;
  compressor   = 1;
  
  bzero(heads, sizeof(heads));
  build_envelopes();
  
  /* tell the JACK server to call error() whenever it
     experiences an error.
  */

  /* jack_set_error_function (jack_error);
   * init_jack_group("default");
   */
  alsa_thread();
  return(1);
}

/**/

int
jack_srate (jack_nframes_t nframes, void *arg) {
  printf("the sample rate is now %" PRIu32 "/sec\n", nframes);
  return 0;
}

/**/

void
jack_shutdown (void *arg)
{
  printf("bye.\n");
  exit (1);
}

/**/

int
init_jack_group (char *group_name) {
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
    fprintf(stderr, "jack server not running?\n");
    exit(1);
  }
  
  /* tell the JACK server to call `process_jack_out()' whenever
     there is work to be done.
  */
  jack_set_process_callback(client, process_jack_out, (void *) group);

  /* tell the JACK server to call `jack_srate()' whenever
     the sample rate of the system changes.
  */
  jack_set_sample_rate_callback(client, jack_srate, 0);

  /* tell the JACK server to call `jack_shutdown()' if
     it ever shuts down, either entirely, or if it
     just decides to stop calling us.
  */
  jack_on_shutdown(client, jack_shutdown, 0);

  /* display the current sample rate. once the client is activated 
     (see below), you should rely on your own sample rate
     callback (see above) for this value.
  */
  printf("engine sample rate: %" PRIu32 "\n",
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
  if (jack_activate(client)) {
    fprintf(stderr, "cannot activate client");
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

  if (jack_connect(client, jack_port_name(group->left_port), ports[0])) {
    fprintf(stderr, "cannot connect left port\n");
  }

  if (jack_connect(client, jack_port_name(group->right_port), ports[1])) {
    fprintf(stderr, "cannot connect right port\n");
  }

  free(ports);
  return 0;
}

/**/

extern int audio_trigger (char *sample_name, 
                          float speed, 
                          float shape,
                          float pan,
                          float volume,
                          char *envelope_name
                         ) {
  t_head *head = NULL;
  int c;
  
  if (speed == 0) {
    return(0);
  }
  
  for (c = 0; c < MAXHEADS; ++c) {
    if (heads[c].active == 0) {
      head = &heads[c];
      break;
    }
  }
  
  if (head == NULL) {
    /* steal the oldest active one */
    head = &heads[0];
    for (c = 1; c < MAXHEADS; ++c) {
      if (heads[c].position > head->position) {
        head = &heads[c];
      }
    }
    head->active = 0;
  }

  if (head != NULL && (head->sample = get_sample(sample_name))) {
    head->active     = 1;
    head->position   = ((speed < 0) ? head->sample->info->frames : 0);
    head->speed      = speed;
    printf("envname: %s\n", envelope_name);
    if (strcmp(envelope_name, "percussive") == 0) {
      head->env_volume = env_percussive;
    }
    else if (strcmp(envelope_name, "valley")) {
      head->env_volume = env_valley;
    }
    else if (strcmp(envelope_name, "tri")) {
      head->env_volume = env_tri;
    }
    head->shape      = shape;
    head->pan        = pan;
    head->volume     = volume;
  }
  else {
    perror("darn");
  }

  return(1);
}
