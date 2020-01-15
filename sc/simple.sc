// by alexatslabdotorg
// http://yaxu.org/

// simple.sc - a rather naive but working SuperCollider patch containing 
// a synthesiser and sample player
//
// Copyright (C) Alex McLean 2004
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



(
  var s, response, buffers;
  s = Server.local;
  s.freeAll;
  s.latency = 0;
  buffers = Dictionary.new;

  SynthDef("synthy", { arg freq=440,formfreq=100,gate=0.0,bwfreq=200,ts=1,lgain=1,rgain=1,
                       crackle=0, browndel=0, envtype;
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
                        x = EnvGen.kr(Env.perc,
                                      doneAction: 2, timeScale: ts
                                     ) * x;
                        Out.ar(0, x);
                      }
          ).send(s);

  SynthDef('player',
           { arg out=0, bufnum=0, rate=1, lgain=1, rgain=1, ts=1, crackle=1, browndel=0;
             var x, y, buffer, delayedSignal, mixedSignal;
             x = PlayBuf.ar([1, 1], bufnum, rate);
             x = x + if (crackle > 1.5, Crackle.ar(crackle), 0);
	     x = x * [lgain, rgain];
             x = if (browndel > 0, 
                     DelayN.ar(x, 0.02, 0.02, 1, BrownNoise.ar(x.distort, 0) * browndel), 
                     x
                    );
             x = EnvGen.kr(Env.perc, doneAction: 2, 
				     timeScale: ts) * x;
	     Out.ar(0, x);
           }
          ).send(s);


  response = { 
    arg time, responder, message; 
    var mybuf, sample, lgain, rgain, crackle, noise, ts, browndel, rate, envtype;
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
        envtype  = message[10];

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
				\rate, rate / 100,
				\envtype,  envtype,
				\out, 
				0, 
				\bufnum, mybuf.bufnum]
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
                             \envtype,  message[10]
                            ]
                ); nil;
          };
        );
      });
    };

    o = OSCresponder(nil, '/play', response);
    o.add;

    "[started]".postln;
)
