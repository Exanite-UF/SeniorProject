// Copyright (c) 2016  GeometryFactory SARL (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Installation/include/CGAL/license/SearchStructures.h $
// $Id: include/CGAL/license/SearchStructures.h 50cfbde3b84 $
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s) : Andreas Fabri
//
// Warning: this file is generated, see include/CGAL/license/README.md

#ifndef CGAL_LICENSE_SEARCHSTRUCTURES_H
#define CGAL_LICENSE_SEARCHSTRUCTURES_H

#include <CGAL/config.h>
#include <CGAL/license.h>

#ifdef CGAL_SEARCHSTRUCTURES_COMMERCIAL_LICENSE

#  if CGAL_SEARCHSTRUCTURES_COMMERCIAL_LICENSE < CGAL_RELEASE_DATE

#    if defined(CGAL_LICENSE_WARNING)

       CGAL_pragma_warning("Your commercial license for CGAL does not cover "
                           "this release of the dD Range and Segment Trees package.")
#    endif

#    ifdef CGAL_LICENSE_ERROR
#      error "Your commercial license for CGAL does not cover this release \
              of the dD Range and Segment Trees package. \
              You get this error, as you defined CGAL_LICENSE_ERROR."
#    endif // CGAL_LICENSE_ERROR

#  endif // CGAL_SEARCHSTRUCTURES_COMMERCIAL_LICENSE < CGAL_RELEASE_DATE

#else // no CGAL_SEARCHSTRUCTURES_COMMERCIAL_LICENSE

#  if defined(CGAL_LICENSE_WARNING)
     CGAL_pragma_warning("\nThe macro CGAL_SEARCHSTRUCTURES_COMMERCIAL_LICENSE is not defined."
                          "\nYou use the CGAL dD Range and Segment Trees package under "
                          "the terms of the GPLv3+.")
#  endif // CGAL_LICENSE_WARNING

#  ifdef CGAL_LICENSE_ERROR
#    error "The macro CGAL_SEARCHSTRUCTURES_COMMERCIAL_LICENSE is not defined.\
            You use the CGAL dD Range and Segment Trees package under the terms of \
            the GPLv3+. You get this error, as you defined CGAL_LICENSE_ERROR."
#  endif // CGAL_LICENSE_ERROR

#endif // no CGAL_SEARCHSTRUCTURES_COMMERCIAL_LICENSE

#endif // CGAL_LICENSE_SEARCHSTRUCTURES_H
