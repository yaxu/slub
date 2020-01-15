
#   Sfront, a SAOL to C translator
#   Copyright (C) 1998 John Lazzaro
#   Maintainers's address: lazzaro@cs.berkeley.edu;
#
#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation (Version 1, 1989).
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; see the file COPYING.  If not, write to
#the Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#


##
## set these network options to configure your session

NETWORK = -session "your_session" -passphrase "passwd"

##
## audio output driver, as shown below configured for linux. 

AOUT = -aout linux 

## Note: when running both an nmp_audio and nmp_null client on the
## same machine, scheduler competition might cause scratchy audio
## artifacts. In this case, run nmp_audio as root, or add the
## option -latency 0.005805 to AOUT above (remember to change it
## back when you are done -- keyboard response is very sluggish
## with this higher latency value).

##
## control driver, as shown below configured for ascii keyboard. 
## if you have a MIDI keyboard, the linmidi driver (OSS) is known to work,
## other MIDI jack control drivers are untested but should work too.

CIN = -cin ascii

##
## for AOUT and CIN selections that need libraries linked with sa.c

IOLINK = 

##
## end of audio/network/control configuration
##

FILENAME = nmp

SAOLFILE = $(FILENAME).saol
SASLFILE = $(FILENAME).sasl

CC = gcc
OPT = -O3
CFLAGS = $(OPT)
SFRONT = sfront

$(FILENAME): $(SAOLFILE) 
	$(SFRONT) $(NETWORK) $(AOUT) $(CIN) -orc $(SAOLFILE) -sco $(SASLFILE)
	$(CC) $(CFLAGS) sa.c -lm $(IOLINK) -o sa
	./sa 

clean: 
	rm -rf sa.c sa audio a*.wav $(MP4FILE) $(OUTFILE) *.info *~ safe



