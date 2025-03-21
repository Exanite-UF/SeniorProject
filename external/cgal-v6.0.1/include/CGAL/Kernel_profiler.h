// Copyright (c) 2004  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Filtered_kernel/include/CGAL/Kernel_profiler.h $
// $Id: include/CGAL/Kernel_profiler.h 50cfbde3b84 $
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Sylvain Pion

#ifndef CGAL_KERNEL_PROFILER_H
#define CGAL_KERNEL_PROFILER_H

// This file contains the definition of a kernel traits profiler.

#include <CGAL/basic.h>
#include <typeinfo>

namespace CGAL {

// Primitive wrapper which handles the profiling.
template < typename P >
struct Primitive_profiler
  : public P
{
    typedef typename P::result_type  result_type;

// #define CGAL_KERNEL_PROFILER CGAL_PROFILER(CGAL_PRETTY_FUNCTION);
#define CGAL_KERNEL_PROFILER \
        CGAL_PROFILER(typeid(static_cast<const P&>(*this)).name())

    Primitive_profiler(const P& p = P())
      : P(p) {}

    template <class ... A>
    result_type
    operator()(A&& ... a) const
    {
        CGAL_KERNEL_PROFILER;
        return P::operator()(std::forward<A>(a)...);
    }
};

// We inherit all geometric objects from K, and just replace the primitives.
template < typename K >
struct Kernel_profiler
  : public K
{
#define CGAL_prof_prim(X, Y) \
    typedef Primitive_profiler<typename K::X> X; \
    X Y() const { return X(static_cast<const K&>(*this).Y()); }

#define CGAL_Kernel_pred(X, Y)  CGAL_prof_prim(X, Y)
#define CGAL_Kernel_cons(X, Y)  CGAL_prof_prim(X, Y)

#include <CGAL/Kernel/interface_macros.h>
};

} //namespace CGAL

#endif // CGAL_KERNEL_PROFILER_H
