/*
 * ComputerSelectPanel.cpp - provides a view for a network object tree
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

#include <QFileDialog>
#include <QKeyEvent>
#include <QMessageBox>

#include "ComputerSelectModel.h"
#include "ComputerSelectPanel.h"
#include "ComputerManager.h"
#include "NetworkObjectModel.h"
#include "LocationDialog.h"
#include "VeyonConfiguration.h"

#include "ui_ComputerSelectPanel.h"


ComputerSelectPanel::ComputerSelectPanel( ComputerManager& computerManager, ComputerSelectModel* model, QWidget* parent ) :
	QWidget(parent),
	ui(new Ui::ComputerSelectPanel),
	m_computerManager( computerManager ),
	m_model( model )
{
	ui->setupUi(this);

	// capture keyboard events for tree view
	ui->treeView->installEventFilter( this );

	// set computer tree model as data model
	ui->treeView->setModel( m_model );

	// set default sort order
	ui->treeView->sortByColumn( 0, Qt::AscendingOrder );

	ui->addLocationButton->setVisible( VeyonCore::config().showCurrentLocationOnly() &&
									   VeyonCore::config().allowAddingHiddenLocations() );

	ui->filterLineEdit->setHidden( VeyonCore::config().hideComputerFilter() );

	connect( ui->filterLineEdit, &QLineEdit::textChanged,
			 this, &ComputerSelectPanel::updateFilter );
}



ComputerSelectPanel::~ComputerSelectPanel()
{
	delete ui;
}



bool ComputerSelectPanel::eventFilter( QObject *watched, QEvent *event )
{
	if( watched == ui->treeView &&
		event->type() == QEvent::KeyPress &&
		dynamic_cast<QKeyEvent*>(event) != nullptr &&
		dynamic_cast<QKeyEvent*>(event)->key() == Qt::Key_Delete )
	{
		removeLocation();
		return true;
	}

	return QWidget::eventFilter( watched, event );
}



void ComputerSelectPanel::addLocation()
{
	LocationDialog dialog( m_computerManager.networkObjectModel(), this );
	if( dialog.exec() && dialog.selectedLocation().isEmpty() == false )
	{
		m_computerManager.addLocation( dialog.selectedLocation() );
	}
}



void ComputerSelectPanel::removeLocation()
{
	auto model = m_computerManager.computerTreeModel();
	const auto index = ui->treeView->selectionModel()->currentIndex();

	if( index.isValid() &&
		model->data( index, NetworkObjectModel::TypeRole ).value<NetworkObject::Type>() == NetworkObject::Type::Location )
	{
		m_computerManager.removeLocation( model->data( index, NetworkObjectModel::NameRole ).toString() );
	}
}



void ComputerSelectPanel::saveList()
{
	QString fileName = QFileDialog::getSaveFileName( this, tr( "Select output filename" ),
													 QDir::homePath(), tr( "CSV files (*.csv)" ) );
	if( fileName.isEmpty() == false )
	{
		if( m_computerManager.saveComputerAndUsersList( fileName ) == false )
		{
			QMessageBox::critical( this, tr( "File error"),
								   tr( "Could not write the computer and users list to %1! "
									   "Please check the file access permissions." ).arg( fileName ) );
		}
	}
}



void ComputerSelectPanel::updateFilter()
{
	const auto filter = ui->filterLineEdit->text();
	auto model = ui->treeView->model();

	if( filter.isEmpty() )
	{
		m_model->setFilterWildcard( filter );

		for( int i = 0; i < model->rowCount(); ++i )
		{
			const auto index = model->index( i, 0 );
			ui->treeView->setExpanded( index, m_expandedGroups.contains( index ) );
		}

		m_previousFilter.clear();
	}
	else
	{
		if( m_previousFilter.isEmpty() )
		{
			m_expandedGroups.clear();

			for( int i = 0; i < model->rowCount(); ++i )
			{
				const auto index = model->index( i, 0 );
				if( ui->treeView->isExpanded( index ) )
				{
					m_expandedGroups.append( index );
				}
			}
		}

		m_previousFilter = filter;

		m_model->setFilterWildcard( filter );
		ui->treeView->expandAll();
	}
}
