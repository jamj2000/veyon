/*
 * PluginManager.h - header for the PluginManager class
 *
 * Copyright (c) 2017-2020 Tobias Junghans <tobydox@veyon.io>
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

#include <QObject>

#include "Plugin.h"
#include "PluginInterface.h"

class QPluginLoader;

class VEYON_CORE_EXPORT PluginManager : public QObject
{
	Q_OBJECT
public:
	explicit PluginManager( QObject* parent = nullptr );
	~PluginManager();

	void loadPlatformPlugins();
	void loadPlugins();
	void upgradePlugins();

	const PluginInterfaceList& pluginInterfaces() const
	{
		return m_pluginInterfaces;
	}

	const QObjectList& pluginObjects() const
	{
		return m_pluginObjects;
	}

	void registerExtraPluginInterface( QObject* pluginObject );

	PluginUidList pluginUids() const;

	template<class InterfaceType, class FilterArgType = InterfaceType>
	InterfaceType* find( const std::function<bool (const FilterArgType *)>& filter = []() { return true; } )
	{
		for( auto object : qAsConst(m_pluginObjects) )
		{
			auto pluginInterface = qobject_cast<InterfaceType *>( object );
			if( pluginInterface && filter( qobject_cast<FilterArgType *>( object ) ) )
			{
				return pluginInterface;
			}
		}

		return nullptr;
	}

	QString pluginName( Plugin::Uid pluginUid ) const;

private:
	void initPluginSearchPath();
	void loadPlugins( const QString& nameFilter );

	PluginInterfaceList m_pluginInterfaces{};
	QObjectList m_pluginObjects{};
	QList<QPluginLoader *> m_pluginLoaders{};
	bool m_noDebugMessages{false};

};
