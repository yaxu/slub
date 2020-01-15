(
  var s, response, buffers;
  s = Server.local;
  "SC_JACK_DEFAULT_OUTPUTS".setenv("alsa_pcm:playback_1,alsa_pcm:playback_2");
  s.waitForBoot({

  buffers = Dictionary.new;


SynthDef("sine", { 
	arg freq=400, ts = 1, volume = 1, direction = 0, noise = 0.1,
	    sin = 1, tri = 0, saw = 0, clip = 0, pan = 0;
	var line = Line.kr(freq, freq + (direction * freq), ts);

	x = 0;
	
	// Maybe xfade these randomly with values as probabilities?
	x = x + if (sin > 0, SinOsc.ar(line, 0, 0.1) * sin, 0);
	x = x + if (tri > 0, LFTri.ar(line, 0, 0.1)  * tri, 0);
	x = x + if (saw > 0, LFSaw.ar(line, 0, 0.1)  * saw, 0);

	x = x * EnvGen.kr(Env.perc,
		doneAction: 2, timeScale: ts
	);

	// x = DelayN.ar(x, 0.02, 0.02, 1, BrownNoise.ar(x.distort, 0) * 0.4);

	y = PinkNoise.ar(0.25);
	
	y = y * EnvGen.kr(Env.perc, timeScale: ts * noise);

	x = x + y;
	x = if (clip > 0, x.clip2(clip), x);
	x = x * volume;
	x = Pan2.ar(x, pan);

	Out.ar(0, x);
}
).send(s);


	r = { 
		arg time, responder, message; 
		var event = 
		{Synth("sine", 
			[
				\freq,      message[1],
				\ts,        message[2],
				\volume,    message[3],
				\direction, message[4],
				\noise,     message[5],
				\sin,       message[6],
				\tri,       message[7],
				\saw,       message[8],
				\clip,      message[9],
				\pan,       message[10],
			]
		)
		};
		
		SystemClock.sched(time - Date.getDate.rawSeconds, event, nil);
	};
	
	o = OSCresponder(nil, '/nice', r);
	o.add;


  SynthDef("synthy", { arg freq=440,formfreq=100,gate=0.0,bwfreq=200,ts=1,lgain=1,rgain=1,
                       crackle=0, browndel=0, filter=0;
                        var x;
                        x = Formant.ar(
				       [freq - 0.1, freq + 0.1],
				       formfreq,
				       bwfreq
				      );
                        x = x + if (crackle > 1.5, Crackle.ar(crackle), 0);
                        x = x * [lgain, rgain];
                        x = if (browndel > 0, 
                                DelayN.ar(x, 0.02, 0.02, 1, BrownNoise.ar(x.distort, 0) * browndel), 
                                x
                               );
//                        x = if (filter > 0 && filter < 44000,
//                                BPF.ar(x, filter, 0.5, 0.3),
//                                x
//                               );
                        x = EnvGen.kr(Env.perc,
                                      doneAction: 2, timeScale: ts
                                     ) * x;
                        Out.ar(0, x);
                      }
          ).send(s);

  SynthDef("noisebox", 
    { 
       arg lgain=1, rgain=1, ts=1, browndel=0, filter=44000, envtype=0;
       x = GrayNoise.ar(0.25);
       
       x = 
               EnvGen.kr(Env.new([0,1,0],[0.4,0.4],'sine'),
                         doneAction: 2, timeScale: ts
                        ) * x;
                       
       x = Pan2.ar(x, SinOsc.kr(200, 0, 0.5), 1);
//       x = if ((filter >= 0 && filter < 44000),
//               BPF.ar(x, filter, 0.5, 0.3),
//               x
//              );
       x = if (browndel > 0, 
               DelayN.ar(x, 0.02, 0.02, 1, BrownNoise.ar(x.distort, 0) * browndel), 
                         x
                        );
       x = x * [lgain, rgain];
       Out.ar(0, x);
    }
  ).send(s);

  SynthDef('player',
           { arg out=0, bufnum=0, rate=1, lgain=1, rgain=1, ts=1, crackle=1, browndel=0, filter=44000;
             var x, y, buffer, delayedSignal, mixedSignal;
             x = PlayBuf.ar([1, 1], bufnum, rate);
             x = x + if (crackle > 1.5, Crackle.ar(crackle), 0);
    	     x = x * [lgain, rgain];
             x = if (browndel > 0, 
                     DelayN.ar(x, 0.02, 0.02, 1, BrownNoise.ar(x.distort, 0) * browndel), 
                     x
                    );
//             x = if (filter > 0,
//                     BPF.ar(x, filter, 0.5, 0.3),
//                     x
//                    );
             x = EnvGen.kr(Env.perc, doneAction: 2, 
				     timeScale: ts) * x;
	     Out.ar(0, x);
           }
          ).send(s);

  SynthDef('player2',
           { arg out=0, bufnum=0, rate=1, pan=0, ts=1, crackle=1, browndel=0, filter=44000;
             var x, y, buffer, delayedSignal, mixedSignal;
             x = PlayBuf.ar(1, bufnum, rate);
			 x = Pan2.ar(x, pan);
             x = x + if (crackle > 1.5, Crackle.ar(crackle), 0);
             x = if (browndel > 0, 
                     DelayN.ar(x, 0.02, 0.02, 1, BrownNoise.ar(x.distort, 0) * browndel), 
                     x
                    );
//             x = if (filter > 0,
//                     BPF.ar(x, filter, 0.5, 0.3),
//                     x
//                    );
             x = EnvGen.kr(Env.perc, doneAction: 2, 
				     timeScale: ts) * x;
			 Out.ar(0, x);
           }
          ).send(s);


  response = { 
    arg time, responder, message; 
    var mybuf, sample, lgain, rgain, crackle, noise, ts, browndel, rate, filter;
    if (message[1] == 'on',
      {
        sample   = message[2];
        lgain    = message[3];
        rgain    = message[4];
        crackle  = message[5];
        noise    = message[6];
        ts       = message[7];
        browndel = message[8];
        rate     = message[9];
        rate = ((rate - 100) / 100);
        filter   = message[10];

        mybuf = buffers.at(sample);
        if (mybuf == nil, 
            {mybuf = Buffer.read(s, sample);
             buffers.add(sample -> mybuf);
            }
           );
	
        SystemClock.sched(time - Date.getDate.rawSeconds,
          {Synth.new("player", [
				\lgain, lgain / 100, 
				\rgain, rgain / 100, 
				\crackle, crackle / 100,
				\browndel, browndel / 100,
			  \ts, ts / 100,
				\rate, rate,
				\out, 
				0, 
				\bufnum, mybuf.bufnum,
                \filter, filter
               ]
		     ); 
		    nil
          }
        );
      });
    };

    o = OSCresponder(nil, '/trigger', response);
    o.add;

  response = { 
    arg time, responder, message; 
    var mybuf, sample, pan, crackle, noise, ts, browndel, rate, filter;
    if (message[1] == 'on',
      {
        sample   = message[2];
        pan      = message[3];
        crackle  = message[4];
        noise    = message[5];
        ts       = message[6];
        browndel = message[7];
        rate     = message[8];
        rate = ((rate - 100) / 100);
        filter   = message[9];

        mybuf = buffers.at(sample);
        if (mybuf == nil, 
            {mybuf = Buffer.read(s, sample);
             buffers.add(sample -> mybuf);
            }
           );
	
        SystemClock.sched(time - Date.getDate.rawSeconds,
          {Synth.new("player2", [
				\pan,  pan,
				\crackle, crackle / 100,
				\browndel, browndel / 100,
			  \ts, ts / 100,
				\rate, rate,
				\out, 
				0, 
				\bufnum, mybuf.bufnum,
                \filter, filter
               ]
		     ); 
		    nil
          }
        );
      });
    };

    o = OSCresponder(nil, '/trigger2', response);
    o.add;


  response = { 
    arg time, responder, message; 
    if (message[1] == 'on',
      {
        SystemClock.sched(time - Date.getDate.rawSeconds,
          {Synth("synthy", [\num,      message[2],
                             \formfreq, message[3] / 127 * 1000,
                             \bwfreq,   message[4] / 127 * 1000,
                             \ts,       message[5] / 100,
                             \freq,     message[2].midicps / 4.0,
			     \lgain,    message[6] / 100,
			     \rgain,    message[7] / 100,
                             \crackle,  message[8] / 100,
                             \browndel, message[9] / 100,
                             \filter,   message[10]
                            ]
                ); nil;
          };
        );
      });
    };

    o = OSCresponder(nil, '/play', response);
    o.add;

  response = { 
    arg time, responder, message; 
    if (message[1] == 'on',
      {
        SystemClock.sched(time - Date.getDate.rawSeconds,
          {Synth("noisebox",
                 [\lgain,    message[2] / 100,
			      \rgain,    message[3] / 100,
                  \ts,       message[4] / 100,
                  \browndel, message[5] / 100,
                  \filter,   message[6],
                  \envtype,      message[7]
                 ]
                ); nil;
          };
        );
      });
    };

    o = OSCresponder(nil, '/noise', response);
    o.add;

    "[broken started]".postln;
});)


