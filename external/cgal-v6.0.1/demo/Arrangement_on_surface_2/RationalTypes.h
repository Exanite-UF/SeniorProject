// Copyright (c) 2020 GeometryFactory Sarl (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Arrangement_on_surface_2/demo/Arrangement_on_surface_2/RationalTypes.h $
// $Id: demo/Arrangement_on_surface_2/RationalTypes.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s): Ahmed Essam <theartful.ae@gmail.com>

#ifndef ARRANGEMENT_DEMO_RATIONAL_TYPES_H
#define ARRANGEMENT_DEMO_RATIONAL_TYPES_H

#include <CGAL/Cartesian.h>
#include <CGAL/Point_2.h>
#ifdef CGAL_USE_CORE
  #include <CGAL/CORE_BigRat.h>
#else
  #include <CGAL/Exact_rational.h>
#endif

namespace demo_types
{

struct RationalTypes
{
#ifdef CGAL_USE_CORE
  typedef CORE::BigRat Rational;
#else
  typedef CGAL::Exact_rational Rational;
#endif

  typedef CGAL::Cartesian<Rational> Rat_kernel;
  typedef CGAL::Point_2<Rat_kernel> Rat_point_2;
};

} // namespace demo_types

#endif
