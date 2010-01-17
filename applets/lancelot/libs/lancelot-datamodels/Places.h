/*
 *   Copyright (C) 2007 Ivan Cukic <ivan.cukic(at)kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2,
 *   or (at your option) any later version, as published by the Free
 *   Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef LANCELOTAPP_MODELS_PLACES_H
#define LANCELOTAPP_MODELS_PLACES_H

#include <lancelot/lancelot_export.h>

#include "XbelModel.h"

namespace Lancelot {
namespace Models {

/**
 * Dolphin's places
 */
class LANCELOT_EXPORT Places : public XbelModel {
    Q_OBJECT
public:
    Places();
    virtual ~Places();
};

} // namespace Models
} // namespace Lancelot

#endif /* LANCELOTAPP_MODELS_PLACES_H */
