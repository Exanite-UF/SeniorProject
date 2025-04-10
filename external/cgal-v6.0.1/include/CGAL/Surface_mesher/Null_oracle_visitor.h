// Copyright (c) 2005  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Surface_mesher/include/CGAL/Surface_mesher/Null_oracle_visitor.h $
// $Id: include/CGAL/Surface_mesher/Null_oracle_visitor.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Laurent Rineau


#ifndef CGAL_SURFACE_MESHER_NULL_ORACLE_VISITOR_H
#define CGAL_SURFACE_MESHER_NULL_ORACLE_VISITOR_H

#include <CGAL/license/Surface_mesher.h>

#define CGAL_DEPRECATED_HEADER "<CGAL/Surface_mesher/Null_oracle_visitor.h>"
#define CGAL_DEPRECATED_MESSAGE_DETAILS \
  "The 3D Mesh Generation package (see https://doc.cgal.org/latest/Mesh_3/) should be used instead."
#include <CGAL/Installation/internal/deprecation_warning.h>

namespace CGAL {

  namespace Surface_mesher {

  /** \interface OracleVisitor
      \brief Concept of a visitor of oracles.

      \fn void new_point(Point& p)
      Called before a new Point is returned by the oracle.
      \param p The point that has just been computed and will be returned.
  */

  /** Trivial model of the OracleVisitor concept. */
  struct Null_oracle_visitor
  {
    template <class P>
    void new_point(P&) const
    {
    }
  };

  }  // namespace Surface_mesher

} // namespace CGAL


#endif  // CGAL_SURFACE_MESHER_NULL_ORACLE_VISITOR_H
