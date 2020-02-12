/*
 * SwimVncServer.cpp - implementation of SwimVncServer class
 *
 * Copyright (c) 2020 Tobias Junghans <tobydox@veyon.io>
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

extern "C" {
#include "rfb/rfb.h"
#include "rfb/rfbregion.h"
}

#include <QGuiApplication>
#include <QScreen>

#include "SwimVncServer.h"
#include "DeskDupEngine.h"
#include "PlatformInputDeviceFunctions.h"
#include "VeyonConfiguration.h"


struct SwimScreen
{
	~SwimScreen()
	{
		delete[] passwords[0];
		delete deskDupEngine;
	}

	DeskDupEngine* deskDupEngine{new DeskDupEngine};
	rfbScreenInfoPtr rfbScreen{nullptr};
	QSize size;
	QPoint pointerPos{};
	std::array<char *, 2> passwords{};
	int buttonMask{0};
	int bytesPerPixel{4};
};


static void handleClientGone( rfbClientPtr cl )
{
	vInfo() << cl->host;
}


static rfbNewClientAction handleNewClient( rfbClientPtr cl )
{
	cl->clientGoneHook = handleClientGone;

	vInfo() << "New client connection from host" << cl->host;

	return RFB_CLIENT_ACCEPT;
}


static void handleClipboardText( char* str, int len, rfbClientPtr cl )
{
	Q_UNUSED(cl)

	str[len] = '\0';
	qInfo() << "Got clipboard:" << str;
}


static void handleKeyEvent( rfbBool down, rfbKeySym keySym, rfbClientPtr cl )
{
	Q_UNUSED(cl)

	VeyonCore::platform().inputDeviceFunctions().synthesizeKeyEvent( keySym, down );
}



static void handlePointerEvent( int buttonMask, int x, int y, rfbClientPtr cl )
{
	const auto screen = reinterpret_cast<SwimScreen *>( cl->screen->screenData );

	DWORD flags = MOUSEEVENTF_ABSOLUTE;

	if( x != screen->pointerPos.x() || y != screen->pointerPos.y() )
	{
		flags |= MOUSEEVENTF_MOVE;
	}

	if ( (buttonMask & rfbButton1Mask) != (screen->buttonMask & rfbButton1Mask) )
	{
		if( GetSystemMetrics(SM_SWAPBUTTON) )
		{
			flags |= (buttonMask & rfbButton1Mask) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
		}
		else
		{
			flags |= (buttonMask & rfbButton1Mask) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
		}
	}

	if( (buttonMask & rfbButton2Mask) != (screen->buttonMask & rfbButton2Mask) )
	{
		flags |= (buttonMask & rfbButton2Mask) ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
	}

	if( (buttonMask & rfbButton3Mask) != (screen->buttonMask & rfbButton3Mask) )
	{
		if( GetSystemMetrics(SM_SWAPBUTTON) )
		{
			flags |= (buttonMask & rfbButton3Mask) ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
		}
		else
		{
			flags |= (buttonMask & rfbButton3Mask) ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
		}
	}

	int wheelDelta = 0;
	if( buttonMask & rfbWheelUpMask )
	{
		flags |= MOUSEEVENTF_WHEEL;
		wheelDelta = WHEEL_DELTA;
	}
	if( buttonMask & rfbWheelDownMask )
	{
		flags |= MOUSEEVENTF_WHEEL;
		wheelDelta = -WHEEL_DELTA;
	}

	// TODO: multi desktop support
	::mouse_event( flags,
				   DWORD(x * 65535 / screen->size.width()),
				   DWORD(y * 65535 / screen->size.height()),
				   DWORD(wheelDelta),
				   0 );

	screen->buttonMask = buttonMask;
	screen->pointerPos = { x, y };
}


SwimVncServer::SwimVncServer( QObject* parent ) :
	QObject( parent ),
	m_configuration( &VeyonCore::config() )
{
}



void SwimVncServer::prepareServer()
{
}



void SwimVncServer::runServer( int serverPort, const Password& password )
{
	while( true )
	{
		SwimScreen screen;

		if( initScreen( &screen ) == false ||
			initVncServer( serverPort, password, &screen ) == false )
		{
			break;
		}

		while( screen.size == DeskDupEngine::currentVirtualScreenGeometry() )
		{
			// any clients connected?
			if( screen.rfbScreen->clientHead )
			{
				const auto waitTime = clientUpdateRequestsPending( &screen ) ? RequestsPendingEventWaitTime :
																			 DefaultEventWaitTime;
				switch( waitForEvents( &screen, waitTime ) )
				{
				case Event::Screen:
					handleScreenChanges( &screen );
					break;
				default:
					break;
				}
			}
			else
			{
				QThread::msleep( IdleSleepTime );
			}

			rfbProcessEvents( screen.rfbScreen, 0 );
		}

		vInfo() << "Screen configuration changed";

		rfbShutdownServer( screen.rfbScreen, true );
		rfbScreenCleanup( screen.rfbScreen );
	}
}



bool SwimVncServer::initVncServer( int serverPort, const VncServerPluginInterface::Password& password,
								  SwimScreen* screen )
{
	vDebug();

	auto rfbScreen = rfbGetScreen( nullptr, nullptr,
								   screen->size.width(), screen->size.height(),
								   8, 3, screen->bytesPerPixel );

	if( rfbScreen == nullptr )
	{
		return false;
	}

	screen->passwords[0] = qstrdup( password.toByteArray().constData() );

	rfbScreen->desktopName = "SwimVNC";
	rfbScreen->frameBuffer = reinterpret_cast<char *>( screen->deskDupEngine->framebuffer() );
	rfbScreen->port = serverPort;
	rfbScreen->kbdAddEvent = handleKeyEvent;
	rfbScreen->ptrAddEvent = handlePointerEvent;
	rfbScreen->newClientHook = handleNewClient;
	rfbScreen->setXCutText = handleClipboardText;

	rfbScreen->authPasswdData = screen->passwords.data();
	rfbScreen->passwordCheck = rfbCheckPasswordByList;

	rfbScreen->serverFormat.redShift = 16;
	rfbScreen->serverFormat.greenShift = 8;
	rfbScreen->serverFormat.blueShift = 0;

	rfbScreen->serverFormat.redMax = 255;
	rfbScreen->serverFormat.greenMax = 255;
	rfbScreen->serverFormat.blueMax = 255;

	rfbScreen->serverFormat.trueColour = true;
	rfbScreen->serverFormat.bitsPerPixel = uint8_t(screen->bytesPerPixel * 8);

	rfbScreen->alwaysShared = true;
	rfbScreen->handleEventsEagerly = true;
	rfbScreen->deferUpdateTime = 5;

	rfbScreen->screenData = screen;

	rfbInitServer( rfbScreen );

	rfbMarkRectAsModified( rfbScreen, 0, 0, rfbScreen->width, rfbScreen->height );

	screen->rfbScreen = rfbScreen;

	return true;
}



bool SwimVncServer::initScreen( SwimScreen* screen )
{
	if( screen->deskDupEngine )
	{
		screen->size = DeskDupEngine::currentVirtualScreenGeometry();
		screen->bytesPerPixel = QGuiApplication::primaryScreen()->depth() / 8;

		vDebug() << screen->size << screen->bytesPerPixel;

		return screen->deskDupEngine->start( m_configuration.primaryMonitorOnly() );
	}

	return false;
}



static void handleChangeRecord( rfbScreenInfoPtr rfbScreen, const DeskDupEngine::ChangesRecord& changesRecord )
{
	const auto& rect = changesRecord.rect;

	switch( DeskDupEngine::Change(changesRecord.type) )
	{
	case DeskDupEngine::Change::ScreenToScreen:
		// TODO
		vDebug() << "ScreenToScreen" << rect.left << rect.top << rect.right << rect.bottom;
		break;
	case DeskDupEngine::Change::SolidFill:
	case DeskDupEngine::Change::Textout:
	case DeskDupEngine::Change::Blend:
	case DeskDupEngine::Change::Trans:
	case DeskDupEngine::Change::Plg:
	case DeskDupEngine::Change::Blit:
		rfbMarkRectAsModified( rfbScreen, rect.left, rect.top, rect.right, rect.bottom );
		break;
	case DeskDupEngine::Change::Pointer:
		// TODO
		vDebug() << "Pointer" << rect.left << rect.top << rect.right << rect.bottom;
		break;
	case DeskDupEngine::Change::Unknown:
		vWarning() << "unknown DeskDupEngine change type" << changesRecord.type;
		break;
	}
}



bool SwimVncServer::handleScreenChanges( SwimScreen* screen )
{
	if( screen->deskDupEngine == nullptr || screen->rfbScreen == nullptr )
	{
		vInfo() << "Invalid screen state";
		return false;
	}

	const auto previousCounter = screen->deskDupEngine->previousCounter();
	auto counter = screen->deskDupEngine->changesBuffer()->counter;

	if( previousCounter == counter ||
		counter < 1 ||
		counter > DeskDupEngine::MaxChanges )
	{
		return true;
	}

	const auto* changes = screen->deskDupEngine->changesBuffer()->changes;

	if( previousCounter < counter )
	{
		for( ULONG i = previousCounter+1; i <= counter; ++i )
		{
			handleChangeRecord( screen->rfbScreen, changes[i] );
		}
	}
	else
	{
		for( ULONG i = previousCounter + 1; i < DeskDupEngine::MaxChanges; ++i )
		{
			handleChangeRecord( screen->rfbScreen, changes[i] );
		}

		for( ULONG i = 1; i <= counter; ++i )
		{
			handleChangeRecord( screen->rfbScreen, changes[i] );
		}
	}

	screen->deskDupEngine->setPreviousCounter( counter );

	return true;
}



bool SwimVncServer::clientUpdateRequestsPending( const SwimScreen* screen ) const
{
	for( auto clientPtr = screen->rfbScreen->clientHead; clientPtr != nullptr; clientPtr = clientPtr->next )
	{
		if( sraRgnEmpty( clientPtr->requestedRegion ) == false )
		{
			return true;
		}
	}

	return false;
}



SwimVncServer::Event SwimVncServer::waitForEvents( const SwimScreen* screen, DWORD waitTime )
{
	std::array<HANDLE, 2> events{ screen->deskDupEngine->screenEvent(), screen->deskDupEngine->pointerEvent() };

	const auto result = WaitForMultipleObjects( events.size(), events.data(), false, waitTime );

	switch( result )
	{
	case WAIT_TIMEOUT:
	case WAIT_FAILED:
		return Event::None;
	case WAIT_OBJECT_0+0:
		ResetEvent( events[0] );
		return Event::Screen;
	case WAIT_OBJECT_0+1:
		ResetEvent( events[1] );
		return Event::Pointer;
	default:
		vWarning() << "unknown wait result" << result;
		break;
	}

	return Event::None;
}


IMPLEMENT_CONFIG_PROXY(SwimVncConfiguration)
