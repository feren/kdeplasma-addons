/*
 *   Copyright 2012 by Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef WALLPAPERQML_H
#define WALLPAPERQML_H

#include <plasma/wallpaper.h>

class QDeclarativeItem;

class WallpaperQml : public Plasma::Wallpaper
{
    Q_OBJECT
    public:
        WallpaperQml(QObject* parent, const QVariantList& args);

        virtual void paint(QPainter* painter, const QRectF& exposedRect);

    private slots:
        void resizeWallpaper();
        void shouldRepaint(const QList< QRectF >& rects);

    private:
        QGraphicsScene* m_scene;
        QDeclarativeItem* m_item;
};

#endif

