// Copyright (c) 2008 INRIA(France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Intersections_3/include/CGAL/Intersections_3/internal/Plane_3_Plane_3_do_intersect.h $
// $Id: include/CGAL/Intersections_3/internal/Plane_3_Plane_3_do_intersect.h 50cfbde3b84 $
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Geert-Jan Giezeman,
//                 Sylvain Pion
//

#ifndef CGAL_INTERNAL_INTERSECTIONS_3_PLANE_3_PLANE_3_DO_INTERSECT_H
#define CGAL_INTERNAL_INTERSECTIONS_3_PLANE_3_PLANE_3_DO_INTERSECT_H

namespace CGAL {
namespace Intersections {
namespace internal {

template <class K>
inline typename K::Boolean
do_intersect(const typename K::Plane_3& plane1,
             const typename K::Plane_3& plane2,
             const K&)
{
  typedef typename K::RT RT;

  const RT& a = plane1.a();
  const RT& b = plane1.b();
  const RT& c = plane1.c();
  const RT& d = plane1.d();
  const RT& p = plane2.a();
  const RT& q = plane2.b();
  const RT& r = plane2.c();
  const RT& s = plane2.d();

  RT det = a*q-p*b;
  if(det != 0)
    return true;

  det = a*r-p*c;
  if(det != 0)
    return true;

  det = b*r-c*q;
  if(det != 0)
    return true;

  // degenerate case
  if(a!=0 || p!=0)
    return (a*s == p*d);

  if(b!=0 || q!=0)
    return (b*s == q*d);

  if(c!=0 || r!=0)
    return (c*s == r*d);

  return true;
}

} // namespace internal
} // namespace Intersections
} // namespace CGAL

#endif // CGAL_INTERNAL_INTERSECTIONS_3_PLANE_3_PLANE_3_DO_INTERSECT_H
