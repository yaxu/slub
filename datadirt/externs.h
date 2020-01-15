
extern int audio_init (void);

extern int audio_trigger (char *sample_name, 
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
                          float delay2,
			  float cutoff,
			  float resonance
                         );

extern int server_init(void);

extern float effect_waveshaper(float x);


