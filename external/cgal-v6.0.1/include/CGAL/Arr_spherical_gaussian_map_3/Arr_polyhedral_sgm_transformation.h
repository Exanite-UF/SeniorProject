// Copyright (c) 2009,2010,2011 Tel-Aviv University (Israel).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Arrangement_on_surface_2/include/CGAL/Arr_spherical_gaussian_map_3/Arr_polyhedral_sgm_transformation.h $
// $Id: include/CGAL/Arr_spherical_gaussian_map_3/Arr_polyhedral_sgm_transformation.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s)     : Naama mayer         <naamamay@post.tau.ac.il>


#ifndef CGAL_ARR_POLYHEDRAL_SGM_TRANSFORMATION_H
#define CGAL_ARR_POLYHEDRAL_SGM_TRANSFORMATION_H

#include <CGAL/license/Arrangement_on_surface_2.h>


namespace CGAL {

/* This function rotates the face when the arrangement is Arr_polyhedral_sgm */

template <class Arrangement, class Transformation_3>
class Arr_polyhedral_sgm_transformation
{
public:

  typedef typename Arrangement::Face_handle   Face_handle;

  void rotate_face(Face_handle f, const Transformation_3 & aff)
  {
    //Transform all the vertices of the original polyhedron.
    f->set_point(aff.transform(f->point()));
  }
};

} //namespace CGAL

#endif
