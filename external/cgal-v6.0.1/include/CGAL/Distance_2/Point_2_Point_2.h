// Copyright (c) 1998-2004
// Utrecht University (The Netherlands),
// ETH Zurich (Switzerland),
// INRIA Sophia-Antipolis (France),
// Max-Planck-Institute Saarbruecken (Germany),
// and Tel-Aviv University (Israel).  All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Distance_2/include/CGAL/Distance_2/Point_2_Point_2.h $
// $Id: include/CGAL/Distance_2/Point_2_Point_2.h 50cfbde3b84 $
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Geert-Jan Giezeman
//                 Michel Hoffmann <hoffmann@inf.ethz.ch>
//                 Andreas Fabri <Andreas.Fabri@geometryfactory.com>

#ifndef CGAL_DISTANCE_2_POINT_2_POINT_2_H
#define CGAL_DISTANCE_2_POINT_2_POINT_2_H

#include <CGAL/Point_2.h>

namespace CGAL {
namespace internal {

template <class K>
inline typename K::FT
squared_distance(const typename K::Point_2& pt1,
                 const typename K::Point_2& pt2,
                 const K& k)
{
  typename K::Vector_2 vec = k.construct_vector_2_object()(pt2, pt1);
  return k.compute_squared_length_2_object()(vec);
}

} // namespace internal

template <class K>
inline typename K::FT
squared_distance(const Point_2<K>& pt1,
                 const Point_2<K>& pt2)
{
  return K().compute_squared_distance_2_object()(pt1, pt2);
}

} // namespace CGAL

#endif // CGAL_DISTANCE_2_POINT_2_POINT_2_H
