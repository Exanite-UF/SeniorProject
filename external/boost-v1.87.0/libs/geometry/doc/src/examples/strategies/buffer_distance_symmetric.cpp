// Boost.Geometry (aka GGL, Generic Geometry Library)
// QuickBook Example

// Copyright (c) 2014-2024 Barend Gehrels, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//[buffer_distance_symmetric
//` Shows how the distance_symmetric strategy can be used as a DistanceStrategy to create symmetric buffers

#include <boost/geometry.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/geometries.hpp>
/*<-*/ #include "../examples_utils/create_svg_buffer.hpp" /*->*/

int main()
{
    using point = boost::geometry::model::d2::point_xy<double>;
    using linestring = boost::geometry::model::linestring<point>;
    using polygon = boost::geometry::model::polygon<point>;

    // Declare the symmetric distance strategy
    boost::geometry::strategy::buffer::distance_symmetric<double> distance_strategy(0.5);

    // Declare other strategies
    boost::geometry::strategy::buffer::side_straight side_strategy;
    boost::geometry::strategy::buffer::join_round join_strategy;
    boost::geometry::strategy::buffer::end_round end_strategy;
    boost::geometry::strategy::buffer::point_circle point_strategy;

    // Declare/fill a multi linestring
    boost::geometry::model::multi_linestring<linestring> ml;
    boost::geometry::read_wkt("MULTILINESTRING((3 5,5 10,7 5),(7 7,11 10,15 7,19 10))", ml);

    // Create the buffered geometry with left/right the same distance
    boost::geometry::model::multi_polygon<polygon> result;
    boost::geometry::buffer(ml, result,
                distance_strategy, side_strategy,
                join_strategy, end_strategy, point_strategy);
    /*<-*/ create_svg_buffer("buffer_distance_symmetric.svg", ml, result); /*->*/

    return 0;
}

//]

