// Copyright (c) 2013 INRIA Sophia-Antipolis (France),
//               2014-2015 GeometryFactory (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Mesh_2/include/CGAL/Mesh_2/Uniform_sizing_field_2.h $
// $Id: include/CGAL/Mesh_2/Uniform_sizing_field_2.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s) : Jane Tournois, Pierre Alliez
//

#ifndef CGAL_MESH_2_UNIFORM_SIZING_FIELD_H
#define CGAL_MESH_2_UNIFORM_SIZING_FIELD_H

#include <CGAL/license/Mesh_2.h>


#include <CGAL/Mesh_2/Sizing_field_2.h>

namespace CGAL {

namespace Mesh_2 {

template <typename Tr>
class Uniform_sizing_field_2
  : public virtual Sizing_field_2<Tr>
{
  typedef typename Tr::Geom_traits::Point_2       Point_2;
  typedef typename Tr::Geom_traits::FT            FT;

public:
  // Vertices of mesh triangulation do not need to be updated
  static const bool is_vertex_update_needed = false;

public:
  Uniform_sizing_field_2(Tr& ) {}

public:
  FT operator()(const Point_2&) const { return FT(1); }
};


} // end namespace Mesh_2

} //namespace CGAL

#endif // CGAL_MESH_2_UNIFORM_SIZING_FIELD_H
