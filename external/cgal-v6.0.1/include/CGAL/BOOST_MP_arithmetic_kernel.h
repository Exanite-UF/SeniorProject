// Copyright (c) 2017 Inria.
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Arithmetic_kernel/include/CGAL/BOOST_MP_arithmetic_kernel.h $
// $Id: include/CGAL/BOOST_MP_arithmetic_kernel.h 50cfbde3b84 $
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
// Author: Marc Glisse <marc.glisse@inria.fr>

#ifndef CGAL_BOOST_MP_ARITHMETIC_KERNEL_H
#define CGAL_BOOST_MP_ARITHMETIC_KERNEL_H

#include <CGAL/Arithmetic_kernel/Arithmetic_kernel_base.h>
#include <CGAL/Get_arithmetic_kernel.h>

#include <CGAL/boost_mp.h>

#ifdef CGAL_USE_BOOST_MP

#ifdef CGAL_USE_CORE
#include <CGAL/CORE_arithmetic_kernel.h>
#endif

//Currently already included in boost_mp.h
//#include <boost/multiprecision/cpp_int.hpp>
//#ifdef CGAL_USE_GMP
//#include <boost/multiprecision/gmp.hpp>
//#endif

// FIXME: the could be several kernels based on Boost.Multiprecision.
namespace CGAL {
/** \ingroup CGAL_Arithmetic_kernel
 *  \brief The Boost.Multiprecision set of exact number types
 */

#if !defined(CGAL_USE_CORE) || defined(CGAL_CORE_USE_GMP_BACKEND)
struct BOOST_cpp_arithmetic_kernel : internal::Arithmetic_kernel_base {
  typedef boost::multiprecision::cpp_int Integer;
  typedef boost::multiprecision::cpp_rational Rational;
};
#else
typedef CORE_arithmetic_kernel BOOST_cpp_arithmetic_kernel;
#endif

#ifdef CGAL_USE_GMP
#if !defined(CGAL_USE_CORE) || !defined(CGAL_CORE_USE_GMP_BACKEND)
struct BOOST_gmp_arithmetic_kernel : internal::Arithmetic_kernel_base {
  typedef boost::multiprecision::mpz_int Integer;
  typedef boost::multiprecision::mpq_rational Rational;
};
#else
typedef CORE_arithmetic_kernel BOOST_gmp_arithmetic_kernel;
#endif
#endif

template <class T1, class T2, class T3, class T4, class T5>
struct Get_arithmetic_kernel<boost::multiprecision::detail::expression<T1,T2,T3,T4,T5> >
: Get_arithmetic_kernel<typename boost::multiprecision::detail::expression<T1,T2,T3,T4,T5>::result_type> {};

template <>
struct Get_arithmetic_kernel<boost::multiprecision::cpp_int> {
  typedef BOOST_cpp_arithmetic_kernel Arithmetic_kernel;
};
template <>
struct Get_arithmetic_kernel<boost::multiprecision::cpp_rational> {
  typedef BOOST_cpp_arithmetic_kernel Arithmetic_kernel;
};
#ifdef CGAL_USE_GMP
template <>
struct Get_arithmetic_kernel<boost::multiprecision::mpz_int> {
  typedef BOOST_gmp_arithmetic_kernel Arithmetic_kernel;
};
template <>
struct Get_arithmetic_kernel<boost::multiprecision::mpq_rational> {
  typedef BOOST_gmp_arithmetic_kernel Arithmetic_kernel;
};
#endif
} //namespace CGAL
#endif // CGAL_USE_BOOST_MP
#endif
