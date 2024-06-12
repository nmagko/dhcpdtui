# Makefile
# Copyright (C) 2018  Victor C Salas Pumacayo (aka nmag)
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.

CC         = gcc
CCOPTIONS  = -fPIC -Wno-format-zero-length
DEFINES	   = -DHAVE_COLOR #-D_DEBUG -D_INFO
INCLUDES   = 
CCFLAGS    = -O2 $(CCOPTIONS) -Wall -I. $(INCLUDES) $(DEFINES)
CLIBRARIES = -ldialog -luregex -lncursesw -lm
PREFIX     = .
INSTALL    = install
STRIP      = strip
UNAME      = $(shell uname)

all: objects binary

objects:
	$(CC) $(CCFLAGS) -c dhcpdtui.c

binary:
	$(CC) $(CCFLAGS) \
	-o dhcpdtui dhcpdtui.o \
	$(CLIBRARIES)
	$(STRIP) dhcpdtui

install:
	$(INSTALL) dhcpdtui /usr/local/bin/

clean:
	rm -f *~
	rm -f *.o *.a
	rm -f lib*.so.*
	rm -f dhcpdtui
