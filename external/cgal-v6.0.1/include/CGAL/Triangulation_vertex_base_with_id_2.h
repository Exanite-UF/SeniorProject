// Copyright (c) 2007  GeometryFactory (France).  All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Triangulation_2/include/CGAL/Triangulation_vertex_base_with_id_2.h $
// $Id: include/CGAL/Triangulation_vertex_base_with_id_2.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Andreas Fabri, Fernando Cacciola

#ifndef CGAL_TRIANGULATION_VERTEX_BASE_WITH_ID_2_H
#define CGAL_TRIANGULATION_VERTEX_BASE_WITH_ID_2_H

#include <CGAL/license/Triangulation_2.h>


#include <CGAL/Triangulation_vertex_base_2.h>

namespace CGAL {

template < typename GT,
           typename Vb = Triangulation_vertex_base_2<GT> >
class Triangulation_vertex_base_with_id_2
  : public Vb
{
  int _id;

public:
  typedef typename Vb::Face_handle                   Face_handle;
  typedef typename Vb::Point                         Point;

  template < typename TDS2 >
  struct Rebind_TDS {
    typedef typename Vb::template Rebind_TDS<TDS2>::Other          Vb2;
    typedef Triangulation_vertex_base_with_id_2<GT, Vb2>   Other;
  };

  Triangulation_vertex_base_with_id_2()
    : Vb() {}

  Triangulation_vertex_base_with_id_2(const Point & p)
    : Vb(p) {}

  Triangulation_vertex_base_with_id_2(const Point & p, Face_handle c)
    : Vb(p, c) {}

  Triangulation_vertex_base_with_id_2(Face_handle c)
    : Vb(c) {}

  int id() const { return _id; }
  int&       id()       { return _id; }
};

} //namespace CGAL

#endif // CGAL_TRIANGULATION_VERTEX_BASE_WITH_ID_2_H
