// qjackctlSystemTray.h
//
/****************************************************************************
   Copyright (C) 2003-2004, rncbc aka Rui Nuno Capela. All rights reserved.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*****************************************************************************/

#ifndef __qjackctlSystemTray_h
#define __qjackctlSystemTray_h

#include <qlabel.h>
#include <qpixmap.h>


//----------------------------------------------------------------------------
// qjackctlSystemTray -- Custom system tray widget.

class QPopupMenu;

class QSystemTray : public QLabel
{
    Q_OBJECT

public:

    // Constructor.
    QSystemTray(QWidget *pParent = 0, const char *pszName = 0);
    // Default destructor.
    ~QSystemTray();

    // Set system tray icon overlay.
    void setPixmapOverlay(const QPixmap& pmOverlay);
	void setPixmap(const QPixmap& pm);
	QPopupMenu* contextMenu();

signals:

    // Clicked signal.
    void clicked();
    // Context menu signal.
    void contextMenuRequested(const QPoint& pos);

protected:

    // Overriden mouse event method.
    void mousePressEvent(QMouseEvent *);
	QPopupMenu* contextmenu;
};


#endif  // __qjackctlSystemTray_h

// end of qjackctlSystemTray.h
