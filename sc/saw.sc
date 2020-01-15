(
var foo;
s = Server.local;

SynthDef("yax", {
    arg ts=1;
    var synth;
	synth = LFSaw.ar([100,151])
      * EnvGen.kr(Env.sine(1), doneAction: 2, timeScale: ts) ;
}).send(s);

foo = Synth("yax");
foo.set(\ts, 1);
)

(
  var s, response;
  s = Server.local;
  //s.boot;
  s.freeAll;
  s.latency = 0;

  SynthDef("sik-goo", { arg freq=440,formfreq=100,gate=0.0,bwfreq=800,ts=1;
                        var x;
                        x = LFSaw.ar(
                          SinOsc.kr([0.02, 0.015], 0, 10, freq)
                        );
                        x = EnvGen.kr(Env.sine(1), doneAction: 2, timeScale: ts) * x;
                        Out.ar(0, x);
                      }
          ).send(s);

  response = { 
    arg time, responder, message; 
    if (message[1] == 'on',
      {
        arg num, vel, ts;
        var foo;
  
        foo = Synth("sik-goo");

        num = message[2];
        vel = message[3];
        ts = message[4];
        
        foo.set(\freq, num.midicps / 4.0);
        foo.set(\ts, ts / 100);
        foo.set(\formfreq, vel / 127 * 1000);
        // "hello".postln;
      });
    };

    o = OSCresponder(nil, '/play', response);
    o.add;
)
