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

#include "config.h"

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

#ifdef USE_JACK
#include <jack/jack.h>
#endif

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

const double coeff[5][11]= {
{ 3.11044e-06,
8.943665402,    -36.83889529,    92.01697887,    -154.337906,    181.6233289,
-151.8651235,   89.09614114,    -35.10298511,    8.388101016,    -0.923313471  
},
{4.36215e-06,
8.90438318,    -36.55179099,    91.05750846,    -152.422234,    179.1170248,  
-149.6496211,87.78352223,    -34.60687431,    8.282228154,    -0.914150747
},
{ 3.33819e-06,
8.893102966,    -36.49532826,    90.96543286,    -152.4545478,    179.4835618,
-150.315433,    88.43409371,    -34.98612086,    8.407803364,    -0.932568035  
},
{1.13572e-06,
8.994734087,    -37.2084849,    93.22900521,    -156.6929844,    184.596544,   
-154.3755513,    90.49663749,    -35.58964535,    8.478996281,    -0.929252233
},
{4.09431e-07,
8.997322763,    -37.20218544,    93.11385476,    -156.2530937,    183.7080141,  
-153.2631681,    89.59539726,    -35.12454591,    8.338655623,    -0.910251753
}
};

float formant_filter(float *in, t_head *head, int channel) {
  float res = 
    (float) ( coeff[head->formant_vowelnum][0] * (*in) +
              coeff[head->formant_vowelnum][1] * head->formant_history[channel][0] +  
              coeff[head->formant_vowelnum][2] * head->formant_history[channel][1] +
              coeff[head->formant_vowelnum][3] * head->formant_history[channel][2] +
              coeff[head->formant_vowelnum][4] * head->formant_history[channel][3] +
              coeff[head->formant_vowelnum][5] * head->formant_history[channel][4] +
              coeff[head->formant_vowelnum][6] * head->formant_history[channel][5] +
              coeff[head->formant_vowelnum][7] * head->formant_history[channel][6] +
              coeff[head->formant_vowelnum][8] * head->formant_history[channel][7] +
              coeff[head->formant_vowelnum][9] * head->formant_history[channel][8] +
              coeff[head->formant_vowelnum][10] * head->formant_history[channel][9] 
             );

  head->formant_history[channel][9] = head->formant_history[channel][8];
  head->formant_history[channel][8] = head->formant_history[channel][7];
  head->formant_history[channel][7] = head->formant_history[channel][6];
  head->formant_history[channel][6] = head->formant_history[channel][5];
  head->formant_history[channel][5] = head->formant_history[channel][4];
  head->formant_history[channel][4] = head->formant_history[channel][3];
  head->formant_history[channel][3] = head->formant_history[channel][2];
  head->formant_history[channel][2] = head->formant_history[channel][1];                    
  head->formant_history[channel][1] = head->formant_history[channel][0];
  head->formant_history[channel][0] = (double) res;
  return res;
}


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
  char error[62];
  t_sample *sample;
  sf_count_t count;
  float *frames;
  SF_INFO *info;
  
  sample = find_sample(samplename);
  
  if (sample == NULL) {
    /* load it from disk */
    snprintf(path, MAXPATHSIZE -1, "%s/%s", SAMPLEROOT, samplename);
    info = (SF_INFO *) calloc(1, sizeof(SF_INFO));
    
    if ((sndfile = (SNDFILE *) sf_open(path, SFM_READ, info)) == NULL) {
      free(info);
    }
    else {
      frames = (float *) malloc(sizeof(float) * info->frames);
      snprintf(error, (size_t) 61, "hm: %d\n", sf_error(sndfile));
      perror(error);
      count  = sf_read_float(sndfile, frames, info->frames);
      snprintf(error, (size_t) 61, "hmm: %d vs %d %d\n", (int) count, (int) info->frames, sf_error(sndfile));
      perror(error);
      
      if (count == info->frames) {
        sample = (t_sample *) calloc(1, sizeof(t_sample));
        strncpy(sample->name, samplename, MAXPATHSIZE - 1);
        sample->info = info;
        sample->frames = frames;
        samples[sample_count++] = sample;
      }
      else {
	snprintf(error, (size_t) 61, "didn't get the right number of frames: %d vs %d %d\n", (int) count, (int) info->frames, sf_error(sndfile));
        perror(error);
        free(info);
        free(frames);
      }
    }
  }
  return(sample);
}

/**/

float scale_speed(t_head *head, float *scale) {
  float speed = head->speed;
  int rev = speed > 0 ? 0 : 1;
  float result;
  float multiplier;

  speed += (rev ? 1 : -1);

  result = scale[(int) fmodf(fabs(speed), 12)];
  multiplier = pow(2, (int) (speed / 12));
  result *= multiplier;

  if (rev) {
    result = 0 - result;
  }
  head->speed = result;
  return(result);
}

/**/

void add_delay(t_line *line, float sample, float delay, float feedback) {
  float rand_no = rand() / RAND_MAX;
  int point = (line->point + (int) ( (delay + (rand_no * 0.2)) * MAXLINE )) % MAXLINE;
  line->samples[point] += (sample * feedback);
}

/**/

float shift_delay(t_line *line) {
  float result = line->samples[line->point];
  line->samples[line->point] = 0;
  line->point = (line->point + 1) % MAXLINE;
  return(result);
}

/**/

void process_frame(float *l, float *r) {
  int c;

  float start, stop, difference, tween_amount, tween, lvolume, rvolume;
  float tmp, pan, progress, erp;
  t_head *head;
  
  float loudest;
  
  for (c=0; c < MAXHEADS; c++) {
    head = &heads[c];
    if (head->active) {
      start = head->sample->frames[((int) head->position)
                                   % head->sample->info->frames
                                  ];
      progress = (head->position / head->duration);
      if (head->env_volume) {
        lvolume = rvolume = 
          envelope_value(head->env_volume, 
                         progress
                         * head->env_volume->size
                         );
      }
      else {
        lvolume = rvolume = 1;
      }
      lvolume *= (head->volume / 100);
      rvolume *= (head->volume / 100);
      
      pan = head->pan;
      if (head->pan_to != pan) {
        pan += (progress * (head->pan_to - pan));
      }
      
      if (pan < 0) {
        lvolume *= (1 + pan);
      }
      if (head->pan > 0) {
        rvolume *= (1 - pan);
      }
      
      if ((((int) head->position) + 1) >= head->duration) {
        tween = 0;
      }
      else {
        /* linear interpolation */
        stop = head->sample->frames[(((int) head->position) + 1)
                                    % head->sample->info->frames
                                   ];
        tween_amount = (head->position - (int) head->position);
        difference = stop - start;
        tween = difference * tween_amount;
      }
      
      if (head->anafeel_strength > 0 && head->anafeel_frequency > 0) {
	tmp = sin(
		  ((fmodf(head->position,
			  head->anafeel_frequency
			  )
		    / head->anafeel_frequency
		    )
		   * M_PI
		   )
		  );
	tmp *= head->anafeel_strength;
	erp = (head->anafeel_strength / 2) - tmp;
	tmp = erp;
        head->speed += erp;
	if (head->foo++ % 128 == 0) {
	  printf("foo: %f %f %f %f\n", 
		 tmp, head->speed, erp, head->position
		);
	}
      }
      
      if (head->accellerate != 0) {
        head->speed += head->accellerate;
      }
        
      head->position += head->speed;
      tmp = ((start + tween) * lvolume);
      if (head->shape > 0.0) {
        tmp = effect_waveshaper(tmp, head->shape);
      }
      
      if (head->formant_vowelnum >= 0) {
        tmp = formant_filter(&tmp, head, 0);
      }
      
      *l += tmp;
      if (head->delay != 0) {
	add_delay(line_l, tmp, head->delay, 0.8);
      }

      if (head->sample->info->channels > 1) {
        /* Assume it's a stereo sample */
        if (head->position >= head->duration) {
          head->position = 0;
          head->active = 0;
          continue;
        }
        if (head->position < 0) {
          head->position = 0;
          head->active = 0;
          continue;
        }
        
        start = head->sample->frames[((int) head->position)
                                     % head->sample->info->frames
                                    ];
        if ((((int) head->position) + 1) >= head->duration) {
          tween = 0;
        }
        else {
          stop = head->sample->frames[(((int) head->position) + 1)
                                      % head->sample->info->frames
                                     ];
          tween_amount = (head->position - (int) head->position);
          difference = stop - start;
          tween = difference * tween_amount;
        }
        
        head->position += head->speed;
      }
      
      tmp = ((start + tween) * rvolume);
      if (head->shape > 0.0) {
        tmp = effect_waveshaper(tmp, head->shape);
      }

      if (head->formant_vowelnum >= 0) {
        tmp = formant_filter(&tmp, head, 1);
      }

      *r += tmp;
      if (head->delay != 0) {
	add_delay(line_r, tmp, head->delay, 0.8);
      }
      
      if (head->position >= head->duration) {
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
  
  tmp = shift_delay(line_l) + shift_delay(line_l2);

  if (line_feedback_delay != 0) {
    add_delay(line_l, tmp, line_feedback_delay, 0.35);
  }

  if (line_feedback_delay2 != 0) {
    add_delay(line_l2, tmp, line_feedback_delay2, 0.4);
  }

  *l += tmp;
  
  
  tmp = shift_delay(line_r) + shift_delay(line_r2);
  *r += tmp;

  if (line_feedback_delay != 0) {
    add_delay(line_r, tmp, line_feedback_delay, 0.4);
  }

  if (line_feedback_delay2 != 0) {
    add_delay(line_r2, tmp, line_feedback_delay2, 0.3);
  }
  
  compressor += (float) COMPRESSORGAIN / 100000;

  /*  Get the loudest of the two channels (for the compressor, which is mono) */
  loudest = (fabs(*r) > fabs(*l) 
             ? fabs(*r)
             : fabs(*l) 
            );
  if (fabs(loudest * compressor) > 1) {
    compressor = compressor / fabs(loudest * compressor);
  }
  //  printf("compressor: %f\n", compressor);
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

#ifdef USE_JACK

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

#endif

/**/

void build_envelopes() {
  float tri[3]        = {0, 1, 0};
  float percussive[19] = 
    {0, 1, 1, 1, 1, 1, 0.75, 0.7, 0.65, 0.6, 0.55, 
     0.5, 0.4, 0.3, 0.3, 0.3, 0.3, 0.25, 0};
  float slope[3] = 
    {1, 0.5, 0};
  float chop[64] = 
    {1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
     1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
     1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0,
     1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0
    };
  float valley[5]     = {0, 1, 0.5, 1, 0};

  env_tri        = build_envelope(3,  tri       );
  env_percussive = build_envelope(19, percussive);
  env_valley     = build_envelope(5,  valley    );
  env_slope      = build_envelope(3,  slope     );
  env_chop       = build_envelope(64, chop      );
  print_envelope(env_tri);
  print_envelope(env_percussive);
  print_envelope(env_valley);
  print_envelope(env_slope);
  print_envelope(env_chop);
}

/**/

void alsa_loop() {
  snd_pcm_t *pcm_handle;
  snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
  snd_pcm_hw_params_t *hwparams;
  char *pcm_name;
  int rate = 44100; /* Sample rate */
  unsigned int exact_rate;   /* Sample rate returned by
                              * snd_pcm_hw_params_set_rate_near
                              */ 
  int periods = 4;  /* Number of periods */
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

  data = (unsigned char *)malloc(periodsize);
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

void *dsp_loop(void *arg) {
  /*  3 fragments (???), 4kb buffer (8192=2 ^ 0x0D) */
  int setting   = 0x0003000B; 
  int schannels = 1;
  int format    = AFMT_S16_LE;
  int samplerate = 44100;
  int fh;
  int i;
  stereo *reads;
  stereo *writes;
  short *buf, *bufp;
  float l, r;
  l = r = 0;
  if ((fh=open(DEVDSP, O_WRONLY)) == -1) { 
    printf("Problem opening %s", DEVDSP);
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
      
      writes = &delayline[delaypoint++];
      writes->l = writes->r = 0;
      delaypoint %= DELAYSIZE;
      reads = &delayline[delaypoint];

      process_frame(&writes->l, &writes->r);
      
      *bufp++ = (short) (reads->l * 32767.0);
      *bufp++ = (short) (reads->r * 32767.0);
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
  compressor   = 0.5;

  bzero(delayline, sizeof(delayline));
  bzero(heads, sizeof(heads));
  delaypoint = 0;
  
  line_l = (t_line *) calloc(1, sizeof(t_line));
  line_r = (t_line *) calloc(1, sizeof(t_line));
  line_l2 = (t_line *) calloc(1, sizeof(t_line));
  line_r2 = (t_line *) calloc(1, sizeof(t_line));
  line_feedback_delay = 0;
  line_feedback_delay2 = 0;
  build_envelopes();
  
  /* tell the JACK server to call error() whenever it
     experiences an error.
  */

#ifdef USE_JACK
  jack_set_error_function (jack_error);
  init_jack_group("default");
#else
  alsa_thread();
#endif

  return(1);
}

/**/

#ifdef USE_JACK
int
jack_srate (jack_nframes_t nframes, void *arg) {
  printf("the sample rate is now %" PRIu32 "/sec\n", nframes);
  return 0;
}
#endif

/**/

void
jack_shutdown (void *arg)
{
  printf("bye.\n");
  exit (1);
}

/**/
#ifdef USE_JACK
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
#endif
/**/

extern int audio_trigger(char *sample_name, 
                         float speed, 
                         float shape,
                         float pan,
                         float pan_to,
                         float volume,
                         char *envelope_name,
                         float anafeel_strength,
                         float anafeel_frequency,
                         float accellerate,
                         int   vowel,
                         char *scale_name,
                         float loops,
                         float duration,
                         float delay,
                         float delay2
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
    if (duration && duration > 0) {
      head->duration = duration;
    }
    else {
      head->duration = head->sample->info->frames * loops;
    }

    head->delay         = delay;
    if (delay != 0) {
      line_feedback_delay = delay;
    }

    if (delay2 != 0) {
      line_feedback_delay2 = delay2;
      printf("delay2: %f \n", delay2);
    }
    
    head->position   = ((speed < 0) ? head->duration : 0);
    head->speed      = speed;

    if (strcmp(scale_name, "equal") == 0) {
      scale_speed(head, scale_equal);
    }
    
    if (strcmp(envelope_name, "percussive") == 0) {
      head->env_volume = env_percussive;
    }
    else if (strcmp(envelope_name, "valley") == 0) {
      head->env_volume = env_valley;
    }
    else if (strcmp(envelope_name, "tri") == 0) {
      head->env_volume = env_tri;
    }
    else if (strcmp(envelope_name, "slope") == 0) {
      head->env_volume = env_slope;
    }
    else if (strcmp(envelope_name, "chop") == 0) {
	head->env_volume = env_chop;
    }

    
    head->shape      = shape;
    head->pan        = pan;
    head->pan_to     = pan_to;
    head->volume     = volume;
    head->anafeel_strength  = anafeel_strength;
    head->anafeel_frequency = anafeel_frequency;
    head->accellerate       = accellerate;
    head->foo = 0;
    head->formant_vowelnum = vowel;
    
    if (vowel >= 0) {
      for (c = 0; c < 10; c++) {
        head->formant_history[0][c] = 0;
        head->formant_history[1][c] = 0;
      }
    }
  }
  else {
    perror("darn");
  }

  return(1);
}

