# Boost.Geometry
# Copyright (c) 2024, Oracle and/or its affiliates.
# Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
# Use, modification and distribution is subject to the Boost Software License,
# Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

foreach(item IN ITEMS
    #many_ring_buffer
    #multi_point_growth
    recursive_polygons_buffer
  )
  boost_geometry_add_unit_test("robustness" ${item})
  target_include_directories(${BOOST_GEOMETRY_UNIT_TEST_NAME}
    PRIVATE
      "${PROJECT_SOURCE_DIR}/test/robustness"
      .)
endforeach()
