
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

FILENAME = linbuzz

SAOLFILE = $(FILENAME).saol
SASLFILE = $(FILENAME).sasl
MP4FILE  = $(FILENAME).mp4

INFILE = 
OUTFILE = all


CC = gcc
OPT = -O3
CFLAGS = $(OPT)

# a relative path just so it works
# out of the box in this directory.
# Use an absolute path, or set the
# SFRONT_INCLUDE_PATH environment
# variable to the absolute path,
# so that the -Is flag is not needed.

LIBPATH = ../../../lib
LIBMODE = -gcc -cpp -Is $(LIBPATH)

SFRONT = sfront 


## try 0.004 if clicks happen

OUTMODE = -latency 0.002 -aout linux
INMODE = 
CNMODE = -cin linmidi

## for INFILE/OUTFILE selections that need libraries

IOLINK = 

## for INFILE/OUTFILE std selection:  > foo < bar

REDIRECT = 

##
## makes a raw 16-bit signed integer audio file
##
 
$(OUTFILE): $(SAOLFILE) 
	$(SFRONT) $(LIBMODE) $(OUTMODE) $(CNMODE) -orc $(SAOLFILE) -sco $(SASLFILE)
	$(CC) $(CFLAGS) sa.c -lm $(IOLINK) -o sa
	./sa $(REDIRECT)

## tests bitstream creation code, by creating an MP4 file, decoding
## it, and comparing audio out with audio created by original ASCII
## files. doesn't work with stdin/stdout.

mp4test	: $(OUTFILE)
	rm -rf $(MP4FILE) sa.c
	mv $(OUTFILE) safe
	$(SFRONT) $(LIBMODE) -symtab -orc $(SAOLFILE) -bitout $(MP4FILE)
	$(SFRONT) $(LIBMODE) $(OUTMODE) $(INMODE) -bit $(MP4FILE)
	$(CC) $(CFLAGS) sa.c -lm $(IOLINK) -o sa
	./sa $(REDIRECT)
	cmp $(OUTFILE) safe

## tests a new sfront against an old one. assume safe is the audio
## created by old sfront. 

compare	: $(OUTFILE)
	cmp $(OUTFILE) safe

safe	: $(OUTFILE)
	cp $(OUTFILE) safe

## for performance testing
##

timing	: 
	./sa $(REDIRECT)

##
##
##

## only works on HPUX, for WAV files

play  : $(OUTFILE)
	splayer $(OUTFILE)

clean: 
	rm -rf sa.c sa audio a*.wav $(MP4FILE) $(OUTFILE) *.info *~ safe



