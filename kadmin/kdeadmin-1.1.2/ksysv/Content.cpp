/*

    Requires the Qt widget libraries, available at no cost at
    http://www.troll.no

    Copyright (C) 1997 Peter Putzer
                       putzer@kde.org

    This program is free software; you can redistribute it and/or modify
    it under the terms of version 2 of the GNU General Public License
    as published by the Free Software Foundation.


    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/     

#include "Content.h"

// include meta-object code
#include "Content.moc"

KSVContent::KSVContent( QWidget* parent ) : QWidget( parent, "Content" ) {
  // nothing
}

KSVContent::~KSVContent() {
  // nothing
}

void KSVContent::resizeEvent( QResizeEvent* e ) {
  QWidget::resizeEvent(e);

  emit sizeChanged();
}
