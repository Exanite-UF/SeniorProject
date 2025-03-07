// Copyright (c) 2005-2006  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// Partially supported by the IST Programme of the EU as a Shared-cost
// RTD (FET Open) Project under Contract No  IST-2000-26473
// (ECG - Effective Computational Geometry for Curves and Surfaces)
// and a STREP (FET Open) Project under Contract No  IST-006413
// (ACS -- Algorithms for Complex Shapes)
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Algebraic_kernel_for_spheres/include/CGAL/global_functions_on_roots_and_polynomials_2_3.h $
// $Id: include/CGAL/global_functions_on_roots_and_polynomials_2_3.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s) : Monique Teillaud <Monique.Teillaud@sophia.inria.fr>
//             Sylvain Pion
//             Pedro Machado
//             Julien Hazebrouck
//             Damien Leroy

#ifndef CGAL_CURVED_KERNEL_GLOBAL_FUNCTIONS_ON_ROOTS_AND_POLYNOMIALS_2_3_H
#define CGAL_CURVED_KERNEL_GLOBAL_FUNCTIONS_ON_ROOTS_AND_POLYNOMIALS_2_3_H

#include <CGAL/license/Circular_kernel_3.h>


namespace CGAL {

template < class AK >
inline
typename AK::Polynomial_for_spheres_2_3
construct_polynomial_sphere_2_3(const typename AK::FT& xc,
                               const typename AK::FT& yc,
                               const typename AK::FT& zc,
                               const typename AK::FT& r_sq)
{ return AK().construct_polynomial_sphere_2_3_object()(xc, yc, zc, r_sq); }

} //namespace CGAL

#endif // CGAL_CURVED_KERNEL_GLOBAL_FUNCTIONS_ON_ROOTS_AND_POLYNOMIALS_2_3_H
