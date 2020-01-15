ecasound -i jack_auto -o stdout | sox -t raw -r 44100 -s -w -c 2 - -t wav - |lame -b 128 - - | ./stream.pl
