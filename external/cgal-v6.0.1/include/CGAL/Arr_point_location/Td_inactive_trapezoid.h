// Copyright (c) 2005,2006,2007,2009,2010,2011 Tel-Aviv University (Israel).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Arrangement_on_surface_2/include/CGAL/Arr_point_location/Td_inactive_trapezoid.h $
// $Id: include/CGAL/Arr_point_location/Td_inactive_trapezoid.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s)         : Oren Nechushtan <theoren@math.tau.ac.il>
//               updated by: Michal Balas <balasmic@post.tau.ac.il>

#ifndef CGAL_TD_INACTIVE_TRAPEZOID_H
#define CGAL_TD_INACTIVE_TRAPEZOID_H

#include <CGAL/license/Arrangement_on_surface_2.h>


/*! \file
 * Definition of the Td_inactive_trapezoid class.
 */

#include <CGAL/Arr_point_location/Trapezoidal_decomposition_2.h>
#include <variant>


#ifdef CGAL_TD_DEBUG
#define CGAL_TD_INLINE
#else
#define CGAL_TD_INLINE inline
#endif

namespace CGAL {

/*! \class
 */
class Td_inactive_trapezoid
{
public:
  /*! Operator==. */
  inline bool operator== (const Td_inactive_trapezoid& t2) const
  {
    return (this == &t2);
  }
};

} //namespace CGAL

#endif
