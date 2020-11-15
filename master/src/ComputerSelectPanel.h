/*
 * ComputerSelectPanel.h - provides a view for a network object tree
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

#include <QModelIndexList>
#include <QWidget>

namespace Ui {
class ComputerSelectPanel;
}

class ComputerManager;
class ComputerSelectModel;

class ComputerSelectPanel : public QWidget
{
	Q_OBJECT
public:
	explicit ComputerSelectPanel( ComputerManager& computerManager, ComputerSelectModel* model, QWidget* parent = nullptr );
	~ComputerSelectPanel() override;

	bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
	void addLocation();
	void removeLocation();
	void saveList();
	void updateFilter();

private:
	Ui::ComputerSelectPanel *ui;
	ComputerManager& m_computerManager;
	ComputerSelectModel* m_model;
	QString m_previousFilter;
	QModelIndexList m_expandedGroups;

};
