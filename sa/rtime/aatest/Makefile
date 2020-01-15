
#   Sfront, a SAOL to C translator
#   Copyright (C) 1998 John Lazzaro
#   Maintainers's address: lazzaro@cs.berkeley.edu;
#
# License also applies to SAOL and SASL files. MIDI file is public 
# domain, courtesy of the http://www.mutopiaproject.org project
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

FILENAME = aatest

SAOLFILE = $(FILENAME).saol
SASLFILE = $(FILENAME).sasl
MIDIFILE = $(FILENAME).mid
MP4FILE  = $(FILENAME).mp4

INFILE = 

##
## By default, this is set up to work for Linux. To run one
## of the other UNIX platform tests, change OUTFILE to be
## the correct driver name, and see "IOLINK" comment below.
##
## OUTFILE = irix     ## For SGI IRIX
## OUTFILE = hpux     ## For HPUX
## OUTFILE = freebsd  ## For FreeBSD
## 
## See the README file in this directory for Microsoft Windows
## and Macintosh OS X compilation instructions.
##

OUTFILE = linux

CC = gcc
OPT = -O3
CFLAGS = $(OPT)
SFRONT = sfront 
PLAYER = play
CMP = cmp

OUTMODE = -aout $(OUTFILE)
INMODE = 

## IOLINK adds linking options for soundcard drivers. The
## default linux option uses no linking options.
##
## 
## IOLINK = -laudio   ## For SGI IRIX 
## IOLINK = -lAlib    ## For HPUX
## IOLINK =           ## FreeBSD needs no linking options.
##

IOLINK = 

## for INFILE/OUTFILE std selection:  > foo < bar

REDIRECT = 

##
## makes a raw 16-bit signed integer audio file
##
 
$(OUTFILE): $(SAOLFILE) $(MIDIFILE)
	$(SFRONT) $(OUTMODE) $(INMODE) -orc $(SAOLFILE) -sco $(SASLFILE) -midi $(MIDIFILE)
	$(CC) $(CFLAGS) sa.c -lm $(IOLINK) -o sa
	./sa $(REDIRECT)

## tests bitstream creation code, by creating an MP4 file, decoding
## it, and comparing audio out with audio created by original ASCII
## files. doesn't work with stdin/stdout.

mp4test	: $(OUTFILE)
	rm -rf $(MP4FILE) sa.c
	mv $(OUTFILE) safe
	$(SFRONT)  -orc $(SAOLFILE) -midi $(MIDIFILE) -sco $(SASLFILE) -bitout $(MP4FILE)
	$(SFRONT) $(OUTMODE) $(INMODE) -bit $(MP4FILE)
	$(CC) $(CFLAGS) sa.c -lm $(IOLINK) -o sa
	./sa $(REDIRECT)
	$(CMP) $(OUTFILE) safe

## tests a new sfront against an old one. assume safe is the audio
## created by old sfront. 

compare	: $(OUTFILE)
	$(CMP) $(OUTFILE) safe

safe	: $(OUTFILE)
	cp $(OUTFILE) safe

## for performance testing
##

timing	: 
	/usr/bin/time -p ./sa 

##
##
##

play  : $(OUTFILE)
	$(PLAYER) $(OUTFILE)

clean: 
	rm -rf sa.c sa audio a*.wav $(MP4FILE) $(OUTFILE) *.info *~ safe



