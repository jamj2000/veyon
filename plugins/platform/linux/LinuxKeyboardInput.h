/*
 * LinuxKeyboardInput.h - declaration of LinuxKeyboardInput class
 *
 * Copyright (c) 2019-2020 Tobias Junghans <tobydox@veyon.io>
 *
 * This file is part of Veyon - https://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#pragma once

#include <QString>

// clazy:excludeall=rule-of-three

class LinuxKeyboardInput
{
public:
	using FakeKey = struct FakeKey;
	using Display = struct _XDisplay;
	using KeySym = uint32_t;

	LinuxKeyboardInput();
	~LinuxKeyboardInput();

	void pressKey( KeySym keysym );
	void releaseKey( KeySym keysym );

	void pressAndReleaseKey( KeySym keysym );

	void pressAndReleaseKey( const QByteArray& utf8Data );

	void sendString( const QString& string );

private:
	Display* m_display;
	FakeKey* m_fakeKeyHandle;

};
