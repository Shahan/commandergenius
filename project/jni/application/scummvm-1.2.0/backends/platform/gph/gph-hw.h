/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL: https://scummvm.svn.sourceforge.net/svnroot/scummvm/scummvm/tags/release-1-2-0/backends/platform/gph/gph-hw.h $
 * $Id: gph-hw.h 52648 2010-09-09 09:48:32Z djwillis $
 *
 */

/*
 * GP2X Wiz: Hardware Stuff.
 *
 */

#ifndef GPH_HW_H
#define GPH_HW_H

namespace WIZ_HW {

extern int volumeLevel;

extern void	deviceInit();
extern void	deviceDeinit();
extern void	mixerMoveVolume(int);

} /* namespace WIZ_HW */

namespace GPH {

extern int tapmodeLevel;

extern void	ToggleTapMode();

} /* namespace GPH */

#endif //GPH_HW_H