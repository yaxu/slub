#!/bin/bash
cd ~
killall jackd scsynth.real sclang.real tm.pl
/yaxu/lc/tm/tm.pl &
/usr/bin/jackd -R -t1000 -dalsa -dhw:0 -r44100 -p256 -n2 -P &
sleep 1
sclang /yaxu/sc/broken.sc -
