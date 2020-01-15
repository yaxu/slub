
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
## sets network session to special mirror session

NETWORK = -session mirror

##
## Audio output driver, as shown below configured for linux.
## To run one of the other UNIX platform tests, change AOUT to be
## the correct driver name, and see "IOLINK" comment below.
##
## AOUT = -aout irix     ## For SGI IRIX
## AOUT = -aout hpux     ## For HPUX
## AOUT = -aout freebsd  ## For FreeBSD
## 
## See the README file for Macintosh OS X compilation instructions.
##

AOUT = -aout linux 


##
## control driver, as shown below configured for ascii keyboard. 
## if you have a MIDI keyboard, the linmidi driver (OSS) is known to work,
## other MIDI jack control drivers are untested but should work too.

CIN = -cin ascii


##
## for AOUT and CIN selections that need libraries linked with sa.c
## Linux needs no linking options. For other platforms:
## 
## IOLINK = -laudio   ## For SGI IRIX 
## IOLINK = -lAlib    ## For HPUX
## IOLINK =           ## FreeBSD needs no linking options.
##

IOLINK = 


##
## end of audio/network/control configuration
##

FILENAME = mirror

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



