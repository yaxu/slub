
#   Sfront, a SAOL to C translator
#   Copyright (C) 1998 John Lazzaro
#   To test networking without using a soundcard.
#   Maintainers's address: lazzaro@cs.berkeley.edu;
#
#License below also covers SAOL and SASL programs in this directory.
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
## use null output, -timesync sends out MIDI at the right time

AOUT = -aout null -timesync

##
## control driver, as shown below configured for ascii keyboard. 

CIN = -cin ascii

##
## for AOUT and CIN selections that need libraries linked with sa.c
## when compiling under Solaris, add these options to IOLINK 
##
## -lnsl -lsocket


##
## end of audio/network/control configuration
##

FILENAME = silence

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



