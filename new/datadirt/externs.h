
extern int audio_init (void);
extern int audio_trigger (char *sample_name, 
                          float speed, 
                          float shape,
                          float pan,
                          float volume,
                          char *envelope_name
                         );
extern int server_init(void);

extern float effect_waveshaper(float x);
