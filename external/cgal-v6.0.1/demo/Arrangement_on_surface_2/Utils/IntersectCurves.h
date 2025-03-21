// Copyright (c) 2020 GeometryFactory Sarl (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Arrangement_on_surface_2/demo/Arrangement_on_surface_2/Utils/IntersectCurves.h $
// $Id: demo/Arrangement_on_surface_2/Utils/IntersectCurves.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s): Ahmed Essam <theartful.ae@gmail.com>

#ifndef ARRANGEMENT_DEMO_INTERSECT_CURVES
#define ARRANGEMENT_DEMO_INTERSECT_CURVES

#include <CGAL/Object.h>

// provides the same functionality as Traits::Intersect_2, but precompiled for
// all arrangements for better compilation speeds elsewhere
template <typename Traits_>
class Intersect_curves
{
public:
  using Traits = Traits_;
  using Intersect_2 = typename Traits::Intersect_2;
  using X_monotone_curve_2 = typename Traits::X_monotone_curve_2;

  Intersect_curves(const Traits*);

  void operator()(
    const X_monotone_curve_2& cv1, const X_monotone_curve_2& cv2,
    std::vector<CGAL::Object>& output);

private:
  Intersect_2 intersect;
};

#endif
