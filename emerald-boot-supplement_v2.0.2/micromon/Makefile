# vim: set ts=4 sw=4 noexpandtab
#
#  Copyright (C) 2011 Jeff Kent <jakent@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

.SECONDARY:
.PHONY: all
all: core

.PHONY: core
core:
	cd core; $(MAKE)

.PHONY: distclean_local
distclean_local:
	rm -f *.pyc *.pyo

.PHONY: distclean
distclean: distclean_local
	cd core; $(MAKE) distclean
	cd micromon; $(MAKE) distclean

.PHONY: clean
clean: distclean_local
	cd core; $(MAKE) clean
	cd micromon; $(MAKE) clean

