// Copyright (c) 2006  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Surface_mesher/include/CGAL/Surface_mesh_complex_2_in_triangulation_3.h $
// $Id: include/CGAL/Surface_mesh_complex_2_in_triangulation_3.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Laurent Rineau

#ifndef CGAL_SURFACE_MESH_COMPLEX_2_IN_TRIANGULATION_3_H
#define CGAL_SURFACE_MESH_COMPLEX_2_IN_TRIANGULATION_3_H

#include <CGAL/license/Surface_mesher.h>

#define CGAL_DEPRECATED_HEADER "<CGAL/Surface_mesh_complex_2_in_triangulation_3.h>"
#define CGAL_DEPRECATED_MESSAGE_DETAILS \
  "The 3D Mesh Generation package (see https://doc.cgal.org/latest/Mesh_3/) should be used instead."
#include <CGAL/Installation/internal/deprecation_warning.h>

#include <CGAL/disable_warnings.h>

#include <CGAL/Complex_2_in_triangulation_3.h>

namespace CGAL {

/**  Simple forward class for CGAL::Complex_2_in_triangulation_3<Tr>.
 *
 *   CGAL::Surface_mesher_complex_2_in_triangulation_3<Tr> is an alias
 *   for CGAL::Complex_2_in_triangulation_3<Tr>.
 */
template <class Tr>
class Surface_mesh_complex_2_in_triangulation_3 :
    public Complex_2_in_triangulation_3<Tr>
{
public:
  Surface_mesh_complex_2_in_triangulation_3 (Tr& tr)
    : Complex_2_in_triangulation_3<Tr>(tr)
  {
  }
}; // end Surface_mesh_complex_2_in_triangulation_3

} // end namespace CGAL

#include <CGAL/enable_warnings.h>

#endif // CGAL_SURFACE_MESH_COMPLEX_2_IN_TRIANGULATION_3_H
