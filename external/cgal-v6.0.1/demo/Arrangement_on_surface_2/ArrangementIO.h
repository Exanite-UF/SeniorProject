// Copyright (c) 2020 GeometryFactory Sarl (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Arrangement_on_surface_2/demo/Arrangement_on_surface_2/ArrangementIO.h $
// $Id: demo/Arrangement_on_surface_2/ArrangementIO.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s): Ahmed Essam <theartful.ae@gmail.com>

#ifndef ARRANGEMENT_DEMO_IO_H
#define ARRANGEMENT_DEMO_IO_H

#include <fstream>
#include <vector>

namespace demo_types
{
enum class TraitsType : int;
}

namespace CGAL
{
class Object;
}

struct ArrangementIO
{
  std::pair<CGAL::Object, demo_types::TraitsType> read(std::ifstream&);
  bool
  write(const std::pair<CGAL::Object, demo_types::TraitsType>&, std::ofstream&);
};

#endif // ARRANGEMENT_DEMO_IO_H
