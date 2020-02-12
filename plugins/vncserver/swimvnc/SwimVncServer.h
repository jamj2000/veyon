/*
 * SwimVncServer.h - declaration of SwimVncServer class
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

#pragma once

#include "PluginInterface.h"
#include "VncServerPluginInterface.h"
#include "SwimVncConfiguration.h"

class DeskDupEngine;

struct SwimScreen;

class SwimVncServer : public QObject, VncServerPluginInterface, PluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "io.veyon.Veyon.Plugins.SwimVncServer")
	Q_INTERFACES(PluginInterface VncServerPluginInterface)
public:
	explicit SwimVncServer( QObject* parent = nullptr );

	Plugin::Uid uid() const override
	{
		return QStringLiteral("245e1634-5acb-44c5-9438-0cc804290534");
	}

	QVersionNumber version() const override
	{
		return QVersionNumber( 1, 0 );
	}

	QString name() const override
	{
		return QStringLiteral( "SwimVncServer" );
	}

	QString description() const override
	{
		return tr( "SwimVNC" );
	}

	QString vendor() const override
	{
		return QStringLiteral( "Veyon Community" );
	}

	QString copyright() const override
	{
		return QStringLiteral( "Tobias Junghans" );
	}

	Plugin::Flags flags() const override
	{
		return Plugin::ProvidesDefaultImplementation;
	}

	QWidget* configurationWidget() override
	{
		return nullptr;
	}

	void prepareServer() override;

	void runServer( int serverPort, const Password& password ) override;

	int configuredServerPort() override
	{
		return -1;
	}

	Password configuredPassword() override
	{
		return {};
	}

private:
	static constexpr auto IdleSleepTime = 100;
	static constexpr DWORD DefaultEventWaitTime = 100;
	static constexpr DWORD RequestsPendingEventWaitTime = 5;

	enum class Event {
		None,
		Screen,
		Pointer
	};

	bool initVncServer( int serverPort, const VncServerPluginInterface::Password& password,
						SwimScreen* screen );

	bool initScreen( SwimScreen* screen );
	bool handleScreenChanges( SwimScreen* screen );

	bool clientUpdateRequestsPending( const SwimScreen* screen ) const;

	Event waitForEvents( const SwimScreen* screen, DWORD waitTime );

	SwimVncConfiguration m_configuration;

};
