/*
 * FeatureMessage.h - header for a message encapsulation class for features
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

#include <QVariant>

#include "Feature.h"

class QIODevice;

class VEYON_CORE_EXPORT FeatureMessage
{
public:
	using FeatureUid = Feature::Uid;
	using Command = qint32;
	using Arguments = QVariantMap;

	static constexpr unsigned char RfbMessageType = 41;

	enum SpecialCommands
	{
		DefaultCommand = 0,
		InvalidCommand = -1,
		InitCommand = -2,
	};

	explicit FeatureMessage( FeatureUid featureUid = {}, Command command = InvalidCommand ) :
		m_featureUid( featureUid ),
		m_command( command ),
		m_arguments()
	{
	}

	explicit FeatureMessage( const FeatureMessage& other ) :
		m_featureUid( other.featureUid() ),
		m_command( other.command() ),
		m_arguments( other.arguments() )
	{
	}

	~FeatureMessage() = default;

	FeatureMessage& operator=( const FeatureMessage& other )
	{
		m_featureUid = other.featureUid();
		m_command = other.command();
		m_arguments = other.arguments();

		return *this;
	}

	const FeatureUid& featureUid() const
	{
		return m_featureUid;
	}

	Command command() const
	{
		return m_command;
	}

	const Arguments& arguments() const
	{
		return m_arguments;
	}

	template<typename T = int>
	FeatureMessage& addArgument( T index, const QVariant& value )
	{
		m_arguments[QString::number( static_cast<int>( index ) )] = value;
		return *this;
	}

	template<typename T = int>
	QVariant argument( T index ) const
	{
		return m_arguments[QString::number( static_cast<int>( index ) )];
	}

	bool send( QIODevice* ioDevice ) const;

	bool isReadyForReceive( QIODevice* ioDevice );

	bool receive( QIODevice* ioDevice );

private:
	FeatureUid m_featureUid;
	Command m_command;
	Arguments m_arguments;

} ;
