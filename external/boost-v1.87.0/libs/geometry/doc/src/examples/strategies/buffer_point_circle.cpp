// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2014-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[buffer_point_circle
//` Shows how the point_circle strategy can be used as a PointStrategy to create circular buffers around points

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/geometries.hpp>
/*<-*/ #include "../examples_utils/create_svg_buffer.hpp" /*->*/

int main()
{
    using point = boost::geometry::model::d2::point_xy<double>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the point_circle strategy
    boost::geometry::strategy::buffer::point_circle point_strategy(360);

    // Declare other strategies
    boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(0.7);
    boost::geometry::strategy::buffer::join_round join_strategy;
    boost::geometry::strategy::buffer::end_round end_strategy;
    boost::geometry::strategy::buffer::side_straight side_strategy;

    // Declare/fill of a multi point
    boost::geometry::model::multi_point<point> mp;
    boost::geometry::read_wkt("MULTIPOINT((3 3),(3 4),(4 4),(7 3))", mp);

    // Create the buffer of a multi point
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(mp, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
    /*<-*/ create_svg_buffer("buffer_point_circle.svg", mp, result); /*->*/

    return 0;
}

//]

