// Copyright (c) 2005  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Surface_mesher/include/CGAL/Point_with_surface_index_geom_traits.h $
// $Id: include/CGAL/Point_with_surface_index_geom_traits.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Laurent RINEAU

#ifndef CGAL_POINT_WITH_SURFACE_INDEX_GEOM_TRAITS_H
#define CGAL_POINT_WITH_SURFACE_INDEX_GEOM_TRAITS_H

#include <CGAL/license/Surface_mesher.h>

#define CGAL_DEPRECATED_HEADER "<CGAL/Point_with_surface_index_geom_traits.h>"
#define CGAL_DEPRECATED_MESSAGE_DETAILS \
  "The 3D Mesh Generation package (see https://doc.cgal.org/latest/Mesh_3/) should be used instead."
#include <CGAL/Installation/internal/deprecation_warning.h>

#include <CGAL/Point_with_surface_index.h>

namespace CGAL {

template <class GT>
class Point_with_surface_index_geom_traits : public GT
{
  typedef typename GT::Point_3 Old_point_3;

public:
  typedef Point_with_surface_index<Old_point_3> Point_3;

};  // end Point_with_surface_index_geom_traits

} // end namespace CGAL

#endif // CGAL_POINT_WITH_SURFACE_INDEX_GEOM_TRAITS_H
