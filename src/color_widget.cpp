/*
 * Copyright (c) 2008-2015:  G-CSC, Goethe University Frankfurt
 * Copyright (c) 2006-2008:  Steinbeis Forschungszentrum (STZ Ölbronn)
 * Copyright (c) 2006-2015:  Sebastian Reiter
 * Author: Sebastian Reiter
 *
 * This file is part of ProMesh.
 * 
 * ProMesh is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License version 3 (as published by the
 * Free Software Foundation) with the following additional attribution
 * requirements (according to LGPL/GPL v3 §7):
 * 
 * (1) The following notice must be displayed in the Appropriate Legal Notices
 * of covered and combined works: "Based on ProMesh (www.promesh3d.com)".
 * 
 * (2) The following bibliography is recommended for citation and must be
 * preserved in all covered files:
 * "Reiter, S. and Wittum, G. ProMesh -- a flexible interactive meshing software
 *   for unstructured hybrid grids in 1, 2, and 3 dimensions. In preparation."
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 */

#include <QtWidgets>
#include "color_widget.h"

ColorWidget::ColorWidget(QWidget* parent) : QFrame(parent)
{
	m_color = Qt::black;
}

void ColorWidget::setColor(const QColor& color)
{
	m_color = color;
	update();
	emit colorChanged(color);
}

void ColorWidget::paintEvent(QPaintEvent* event)
{
	QFrame::paintEvent(event);
	QPainter painter(this);
	painter.setBrush(QBrush(m_color, Qt::SolidPattern));
	painter.drawRect(rect());
}

void ColorWidget::mouseReleaseEvent(QMouseEvent* event)
{
	QColorDialog* editor = new QColorDialog(m_color, this);
	connect(editor, SIGNAL(colorSelected(QColor)),
			this, SLOT(setColor(QColor)));
	editor->exec();
}
