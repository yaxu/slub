
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

FILENAME = ascii

SAOLFILE = $(FILENAME).saol
SASLFILE = $(FILENAME).sasl

##
## By default, this is set up to work for Linux. To run one
## of the other UNIX platform tests, change OUTFILE to be
## the correct driver name, and see "IOLINK" comment below.
##
## OUTFILE = irix     ## For SGI IRIX
## OUTFILE = hpux     ## For HPUX
## OUTFILE = freebsd  ## For FreeBSD
## 
## See the README file for Macintosh OS X compilation instructions.
##

OUTFILE = linux

CC = gcc
OPT = -O3
CFLAGS = $(OPT)
SFRONT = sfront 

OUTMODE = -aout $(OUTFILE)
INMODE = 
CNMODE = -cin ascii


## for INFILE/OUTFILE selections that need libraries
## IOLINK adds linking options for soundcard drivers. The
## default linux option uses no linking options.
##
## 
## IOLINK = -laudio   ## For SGI IRIX 
## IOLINK = -lAlib    ## For HPUX
## IOLINK =           ## FreeBSD needs no linking options.
##

IOLINK = 

##
## makes a raw 16-bit signed integer audio file
##
 
$(OUTFILE): $(SAOLFILE) 
	$(SFRONT) $(OUTMODE) $(CNMODE) -orc $(SAOLFILE) -sco $(SASLFILE)
	$(CC) $(CFLAGS) sa.c -lm $(IOLINK) -o sa
	./sa 

clean: 
	rm -rf sa.c sa audio a*.wav $(MP4FILE) $(OUTFILE) *.info *~ safe



