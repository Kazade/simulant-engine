// ----------------------------------------------------------------------------
//
//
// OpenSteer -- Steering Behaviors for Autonomous Characters
//
// Copyright (c) 2002-2003, Sony Computer Entertainment America
// Original author: Craig Reynolds <craig_reynolds@playstation.sony.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
//
// ----------------------------------------------------------------------------
//
//
// Draw
//
// This is a first stab at a graphics module for OpenSteerDemo.  It is intended
// to encapsulate all functionality related to 3d graphics as well as windows
// and graphics input devices such as the mouse.
//
// However this is purely an OpenGL-based implementation.  No special effort
// has been made to keep the "OpenGL way" from leaking through.  Attempting to
// port this to another graphics substrate may run into modularity problems.
//
// In any case, all calls to the underlying graphics substrate should be made
// from this module only.
//
// 10-04-04 bk:  put everything into the OpenSteer namespace
// 06-25-02 cwr: created 
//
//
// ----------------------------------------------------------------------------


#ifndef OPENSTEER_DRAW_H
#define OPENSTEER_DRAW_H


#include "../../../types.h"
#include "AbstractVehicle.h"


// ------------------------------------------------------------------------
// for convenience, names of a few common RGB colors as kglt::Vec3 values
// (XXX I know, I know, there should be a separate "Color" class XXX)

namespace OpenSteer {


    const kglt::Vec3 gBlack   (0, 0, 0);
    const kglt::Vec3 gWhite   (1, 1, 1);

    const kglt::Vec3 gRed     (1, 0, 0);
    const kglt::Vec3 gYellow  (1, 1, 0);
    const kglt::Vec3 gGreen   (0, 1, 0);
    const kglt::Vec3 gCyan    (0, 1, 1);
    const kglt::Vec3 gBlue    (0, 0, 1);
    const kglt::Vec3 gMagenta (1, 0, 1);

    const kglt::Vec3 gOrange (1, 0.5f, 0);

    inline kglt::Vec3 grayColor (const float g) {return kglt::Vec3 (g, g, g);}

    const kglt::Vec3 gGray10 = grayColor (0.1f);
    const kglt::Vec3 gGray20 = grayColor (0.2f);
    const kglt::Vec3 gGray30 = grayColor (0.3f);
    const kglt::Vec3 gGray40 = grayColor (0.4f);
    const kglt::Vec3 gGray50 = grayColor (0.5f);
    const kglt::Vec3 gGray60 = grayColor (0.6f);
    const kglt::Vec3 gGray70 = grayColor (0.7f);
    const kglt::Vec3 gGray80 = grayColor (0.8f);
    const kglt::Vec3 gGray90 = grayColor (0.9f);

} // namespace OpenSteer


// ----------------------------------------------------------------------------
#endif // OPENSTEER_DRAW_H
