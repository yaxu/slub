/*

       CAUDIO - software sampler
(c) copyright - 2001
  adrian ward - slub
 ade@slub.org - http://www.slub.org/

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

the complete license may be found here:
http://www.gnu.org/copyleft/gpl.html

*/

#include <math.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <sndfile.h>           // libsndfile is required

// Customizable parameters
#define MAXSAMPLES     20      // Max number of sample banks to be loaded
#define MAXCHANNELS    10      // Max number of sounds playing at once
#define HISS           30.0    // % hiss you want to simulate
#define COMPRESSORGAIN 10.0    // Speed of compressor gain (higher = faster)

// Sound system constants
#define BUFFERSIZE 100        
#define FORMAT     AFMT_S16_LE
#define CHANNELS   1           // Stereo
#define SAMPLERATE 44100       // 44.1kHz


// stereo struct for easy handling of stereo sample pairs
typedef struct {
 float l;
 float r; 
 float mono; // optionally used
} stereo;

// playbackChannel struct for playback channels (ie, a single channel 'head')
typedef struct {
 float position;
 float speed;
 float volume;
 float pan;
 int   bank;
} playbackChannel;


// Globals for audio system
int       devHandle;
pthread_t audioThreadHnd;

// Globals for playback channels
int             channelPtr = 0;
playbackChannel channels[MAXCHANNELS];
int             maxChannels = 0;

// Globals for sample banks
int       sampleBankPtr;
short    *sampleBank    [MAXSAMPLES];
int       sampleBankSize[MAXSAMPLES];

// Globals for compressor
float     compressor;



void bomb(char *msg) {
 printf("FATAL ERROR: %s\n", msg);
 exit(-1);
}

void loadSample(char *file) {

 SNDFILE *waveFile;
 SF_INFO waveFileInfo;
 int     memReq;

 printf("Loading %s\n",file);

 if ((waveFile=sf_open_read(file, &waveFileInfo)) == NULL) { bomb("Couldnt open audio sample\n"); }

 memReq=waveFileInfo.samples*waveFileInfo.channels*(waveFileInfo.pcmbitwidth/8);

 sampleBank[sampleBankPtr] = malloc(memReq);
 if (sf_read_short(waveFile,sampleBank[sampleBankPtr],waveFileInfo.samples) != waveFileInfo.samples) {
  printf("WARNING: Didn't read entire sample file into memory!\n");
 }

 printf("Sample %s has the following structure:\n  samplerate=%i\n  samples=%i\n  channels=%i\n  pcmbitwidth=%i\n  format=%i\n  sections=%i\n",
          file,
          waveFileInfo.samplerate,
          waveFileInfo.samples,
          waveFileInfo.channels,
          waveFileInfo.pcmbitwidth,
          waveFileInfo.format,
          waveFileInfo.sections);

 sampleBankSize[sampleBankPtr]=waveFileInfo.samples;

 sf_close(waveFile);

 printf("Bank %i loaded, occupying %i bytes\n",sampleBankPtr,memReq);

 sampleBankPtr++;
 if (sampleBankPtr==MAXSAMPLES) {
  bomb("Ran out of sample banks, cannot load any more.");
 }

}

stereo renderPlaybackChannels() {
 
 stereo  tempSample;
 short  *tempPtr;
 float   monoSample;
 int      i;
 int      c=0;

 tempSample.l = (((float)rand())/(((float)RAND_MAX)*1000.0)/100.0) * HISS;
 tempSample.r = (((float)rand())/(((float)RAND_MAX)*1000.0)/100.0) * HISS;
 monoSample   = 0.0;
  
 for (i = 0; i < MAXCHANNELS; i++) {

  if (channels[i].speed > 0.000001) {
   channels[i].position   += channels[i].speed;
   channels[i].volume *= 0.99994;

   tempPtr=sampleBank[channels[i].bank];
   tempPtr+=(int) channels[i].position;

   monoSample = (((float) *tempPtr) / 32767) * channels[i].volume;
//   monoSample *=(sin(channels[i].position*0.1)); // ring modulator!

   tempSample.l      += monoSample * (  channels[i].pan);
   tempSample.r      += monoSample * (1-channels[i].pan);

   c++;
  }

  if ((channels[i].volume < 0.0001) || (channels[i].position>=sampleBankSize[channels[i].bank])) {
   channels[i].speed = 0;
  }

 }

 compressor += (float) COMPRESSORGAIN / 100000;

 // Get the loudest of the two channels (for the compressor, which is mono)
 monoSample=tempSample.l;
 if (fabs(tempSample.r)>fabs(monoSample)) { monoSample=tempSample.r; }

 if (fabs(monoSample * compressor) > 1.0) {
  compressor = compressor / fabs(monoSample * compressor);
 }

 tempSample.l *= compressor;
 tempSample.r *= compressor;

 if (tempSample.l >  1.0) { tempSample.l =  1.0; }
 if (tempSample.l < -1.0) { tempSample.l = -1.0; }
 if (tempSample.r >  1.0) { tempSample.r =  1.0; }
 if (tempSample.r < -1.0) { tempSample.r = -1.0; }

 maxChannels = c;

 return tempSample;

}

void * audioThread(void *arg) {

 short* outPtr;
 short* outDataStartPtr;
 stereo tempSample;
 int    i;
 
 outDataStartPtr = malloc(BUFFERSIZE*2*2L); // Why 2L??
 while (1==1) {
  outPtr = outDataStartPtr;
  for (i=0; i<BUFFERSIZE; i++) {

   tempSample = renderPlaybackChannels();

   *outPtr++ = tempSample.l * 32700.0; // L
   *outPtr++ = tempSample.r * 32700.0; // R

  }
  write(devHandle,outDataStartPtr,BUFFERSIZE*2*2);
 }

}

void init() {

  int setting    = 0x0003000C; // 3 fragments (???), 4kb buffer (4096=2 ^ 0x0C)
  int channels   = CHANNELS;
  int format     = FORMAT;
  int sampleRate = SAMPLERATE;

  compressor = 1.0;

  printf("CAUDIO software sampler and granular synthesizer\n");
  printf("(c) Copyright 2001 Adrian Ward/Slub\n\n");

  printf("Opening /dev/dsp\n");
  if ((devHandle=open("/dev/dsp",O_WRONLY)) == -1) { bomb("Problem opening /dev/dsp"); }

  printf("Configuring audio\n");
  if (ioctl(devHandle,SNDCTL_DSP_SETFRAGMENT,&setting   ) == -1) { bomb("Failed SNDCTL_DSP_SETFRAGMENT"); }
  if (ioctl(devHandle,SNDCTL_DSP_STEREO     ,&channels  ) == -1) { bomb("Failed SNDCTL_DSP_STEREO");      }
  if (ioctl(devHandle,SNDCTL_DSP_SETFMT     ,&format    ) == -1) { bomb("Failed SNDCTL_DSP_SETFMT");      }
  if (ioctl(devHandle,SNDCTL_DSP_SPEED      ,&sampleRate) == -1) { bomb("Failed SNDCTL_DSP_SPEED");       }
  
  printf("Creating audio thread\n");
  if (pthread_create(&audioThreadHnd, NULL, audioThread, NULL) != 0) { bomb("Couldn't create thread"); }
  if (pthread_detach(audioThreadHnd)                        != 0) { bomb("Couldn't detach thread"); }

  printf("Ready\n\nType numbers between 0 and 10...\n");

}


int main() {

  char tempString[255];

  init();
  loadSample("synth.wav");

  while (1==1) {

   printf("%i channels>",maxChannels);
   fgets(tempString,250,stdin);

   channels[channelPtr].position=0.0;
   channels[channelPtr].speed   =0.1 + (atof(tempString) / 10.0);
   channels[channelPtr].volume  =0.9;
   channels[channelPtr].pan     =(float)rand()/(float)RAND_MAX;
   channels[channelPtr].bank    =0;

   channelPtr++;
   if (channelPtr==MAXCHANNELS) { channelPtr=0; }

  }
  
  return 0;

}



