// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2011-2024 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2015.
// Modifications copyright (c) 2015 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[relation
//` Shows how to calculate the spatial relation between a point and a polygon

#include <iostream>
#include <string>

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
/*<-*/ #include "create_svg_two.hpp" /*->*/

int main()
{
    using point_type = boost::geometry::model::d2::point_xy<double>;
    using polygon_type = boost::geometry::model::polygon<point_type>;

    polygon_type poly;
    boost::geometry::read_wkt(
        "POLYGON((2 1.3,2.4 1.7,2.8 1.8,3.4 1.2,3.7 1.6,3.4 2,4.1 3,5.3 2.6,5.4 1.2,4.9 0.8,2.9 0.7,2 1.3)"
            "(4.0 2.0, 4.2 1.4, 4.8 1.9, 4.4 2.2, 4.0 2.0))", poly);

    point_type p(4, 1);

    boost::geometry::de9im::matrix matrix = boost::geometry::relation(p, poly);
    std::string code = matrix.str();

    std::cout << "relation: " << code << std::endl;
    /*<-*/ create_svg("relation.svg", poly, p); /*->*/
    return 0;
}

//]

//[relation_output
/*`
Output:
[pre
relation: 0FFFFF212

[$img/algorithms/within.png]
]

*/
//]
