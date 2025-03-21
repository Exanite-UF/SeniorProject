// Boost.Geometry (aka GGL, Generic Geometry Library)
// Unit Test

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.
// Copyright (c) 2008-2012 Bruno Lalande, Paris, France.
// Copyright (c) 2009-2012 Mateusz Loskot, London, UK.

// This file was modified by Oracle on 2014-2024.
// Modifications copyright (c) 2014-2024 Oracle and/or its affiliates.
// Contributed and/or modified by Vissarion Fysikopoulos, on behalf of Oracle
// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Parts of Boost.Geometry are redesigned from Geodan's Geographic Library
// (geolib/GGL), copyright (c) 1995-2010 Geodan, Amsterdam, the Netherlands.

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "test_get_turns.hpp"
#include <boost/geometry/geometries/geometries.hpp>


template <typename T>
void test_all()
{
    typedef bg::model::point<T, 2, bg::cs::cartesian> pt;
    typedef bg::model::linestring<pt> ls;
    typedef bg::model::polygon<pt> poly;

    test_geometry<ls, poly>("LINESTRING(15 5,24 5,20 2,19 0,13 -4,1 0,10 0,13 3,15 7,16 10,10 10,8 10,4 6,2 8,1 10)",
                            "POLYGON((0 0,5 5,0 10,20 10,20 2,19 0,0 0)(10 3,15 3,15 7,10 7,10 3))",
                            expected("miu+")("iuu+")("tcc+")("tuu=")("mcu+")("mic=")("muu+")
                                    ("tiu+")("mcu+")("mic=")("mcc+")("miu=")("mxu+"));

    test_geometry<ls, poly>("LINESTRING(5 0,5 5,10 5)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            "miu+", "mxu+");
    test_geometry<ls, poly>("LINESTRING(0 0,5 5,10 0)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            "tiu+", "txu+");
    test_geometry<ls, poly>("LINESTRING(0 0,5 0,5 5,10 5,10 0)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            expected("tcu+")("mic=")("mcc+")("txu="));
    test_geometry<ls, poly>("LINESTRING(10 0,5 0,5 5,10 5,10 10)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            expected("tcc+")("miu=")("mcu+")("txc="));

    test_geometry<ls, poly>("LINESTRING(0 0,10 0,10 10)",
                            "POLYGON((0 0,5 5,0 10,20 10,20 2,19 0,0 0)(10 3,15 3,15 7,10 7,10 3))",
                            expected("tcu+")("mic=")("mcu+")("mic=")("mxu+"));

    test_geometry<ls, poly>("LINESTRING(11 1,10 0,0 0)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            "tcc+", "txu=");
    test_geometry<ls, poly>("LINESTRING(0 0,10 0,11 1)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            "tcu+", "tuc=");
    test_geometry<ls, poly>("LINESTRING(10 0,0 0,-1 1)", "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            "tcc+", "tuu=");

    // true hole
    test_geometry<ls, poly>("LINESTRING(9 1,10 5,9 9)",
                            "POLYGON((0 0,0 10,10 10,10 5,10 0,0 0)(2 2,10 5,2 8,2 2))",
                            expected("tiu+")("tiu+"));
    test_geometry<ls, poly>("LINESTRING(10 1,10 5,10 9)",
                            "POLYGON((0 0,0 10,10 10,10 5,10 0,0 0)(2 2,10 5,2 8,2 2))",
                            expected("mcu+")("ecc=")("tiu+")("mxc="));

    // fake hole
    test_geometry<ls, poly>("LINESTRING(9 1,10 5,9 9)",
                            "POLYGON((0 0,0 10,10 10,10 5,2 8,2 2,10 5,10 0,0 0))",
                            expected("tuu+")("tiu+"));
    test_geometry<ls, poly>("LINESTRING(10 1,10 5,10 9)",
                            "POLYGON((0 0,0 10,10 10,10 5,2 8,2 2,10 5,10 0,0 0))",
                            expected("mcu+")("tuc=")("tcu+")("mxc="));

    // true hole
    test_geometry<ls, poly>("LINESTRING(10 1,10 5,2 2)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            expected("mcu+")("mic=")("tcu+")("txc="));
    test_geometry<ls, poly>("LINESTRING(10 1,10 5,2 8)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(10 5,2 8,2 2,10 5))",
                            expected("mcu+")("mic=")("tcc+")("txu="));

    // SPIKE - NON-ENDPOINT - NON-OPPOSITE

    // spike - neq eq
    test_geometry<ls, poly>("LINESTRING(2 2,4 4,1 1)", "POLYGON((0 0,4 4,6 3,6 0,0 0))",
                            expected("mcc+")("txu=")("tcu=")("mxc="));
    // spike - eq eq
    test_geometry<ls, poly>("LINESTRING(0 0,4 4,1 1)", "POLYGON((0 0,4 4,6 3,6 0,0 0))",
                          expected("tcc+")("txu=")("tcu=")("mxc="));
    // spike - eq neq
    test_geometry<ls, poly>("LINESTRING(0 0,3 3,1 1)", "POLYGON((0 0,4 4,6 3,6 0,0 0))",
                            expected("tcc+")("mxu=")("mcu=")("mxc="));
    // spike - neq neq
    test_geometry<ls, poly>("LINESTRING(1 1,3 3,2 2)", "POLYGON((0 0,4 4,6 3,6 0,0 0))",
                            expected("mcc+")("mxu=")("mcu=")("mxc="));
    // spike - out neq
    test_geometry<ls, poly>("LINESTRING(0 0,3 3,2 2)", "POLYGON((1 1,4 4,6 3,6 0,1 1))",
                            expected("mcc+")("mxu=")("mcu=")("mxc="));
    // spike - out eq
    test_geometry<ls, poly>("LINESTRING(0 0,4 4,2 2)", "POLYGON((1 1,4 4,6 3,6 0,1 1))",
                            expected("mcc+")("txu=")("tcu=")("mxc="));
    // spike - out out/eq
    test_geometry<ls, poly>("LINESTRING(0 0,4 4,2 2)", "POLYGON((1 0,4 4,6 3,1 0))",
                            expected("tuu+"));
    test_geometry<ls, poly>("LINESTRING(0 0,4 4,2 2)", "POLYGON((0 1,4 4,6 3,6 0,-1 -1,0 1))",
                            expected("tiu+"));
    // spike - out out/neq
    test_geometry<ls, poly>("LINESTRING(0 0,4 4,2 2)", "POLYGON((4 0,4 5,6 3,4 0))",
                            expected("muu+"));
    test_geometry<ls, poly>("LINESTRING(0 0,4 4,2 2)", "POLYGON((0 4,5 4,6 3,6 0,-1 -1,0 4))",
                            expected("miu+"));

    test_geometry<ls, poly>("LINESTRING(0 1,1 1,0 1)", "POLYGON((0 0,3 3,3 0,0 0))",
                            expected("muu+"));
    test_geometry<ls, poly>("LINESTRING(0 1,3 3,0 1)", "POLYGON((0 0,3 3,3 0,0 0))",
                            expected("tuu+"));
    test_geometry<ls, poly>("LINESTRING(0 1,0 0,0 1)", "POLYGON((0 0,3 3,3 0,0 0))",
                            expected("tuu+"));

    // SPIKE - NON-ENDPOINT - OPPOSITE

    // opposite - eq eq
    test_geometry<ls, poly>("LINESTRING(6 6,4 4,0 0,2 2)", "POLYGON((-1 -1,0 0,4 4,6 3,-1 -1))",
                            expected("tcu+")("txc=")("tcc=")("mxu="));
    // opposite - neq eq
    test_geometry<ls, poly>("LINESTRING(6 6,4 4,0 0,2 2)", "POLYGON((-1 -1,0 0,5 5,6 3,-1 -1))",
                            expected("mcu+")("txc=")("tcc=")("mxu="));
    // opposite - eq, neq
    test_geometry<ls, poly>("LINESTRING(6 6,4 4,0 0,2 2)", "POLYGON((-2 -2,-1 -1,4 4,6 3,-2 -2))",
                            expected("tcu+")("mxc=")("mcc=")("mxu="));
    // opposite - neq neq
    test_geometry<ls, poly>("LINESTRING(6 6,4 4,0 0,2 2)", "POLYGON((-2 -2,-1 -1,3 3,6 3,-2 -2))",
                            expected("mcu+")("mxc=")("mcc=")("mxu="));
    // opposite - neq neq
    test_geometry<ls, poly>("LINESTRING(6 6,4 4,0 0,2 2)", "POLYGON((-2 -2,-1 -1,3 3,5 5,6 3,-2 -2))",
                            expected("mcu+")("mxc=")("mcc=")("mxu="));

    // spike vs internal
    test_geometry<ls, poly>("LINESTRING(0 1,1 1,0 1)", // --
                            "POLYGON((1 0,1 1,2 1,1 0))",
                            expected("tuu+"));
    test_geometry<ls, poly>("LINESTRING(1 2,1 1,1 2)", // |
                            "POLYGON((1 0,1 1,2 1,1 0))",
                            expected("tuu+"));
    test_geometry<ls, poly>("LINESTRING(0 2,1 1,0 2)",
                            "POLYGON((1 0,1 1,2 1,1 0))",
                            expected("tuu+"));
    test_geometry<ls, poly>("LINESTRING(2 0,1 1,2 0)",
                            "POLYGON((1 0,1 1,2 1,2 0,1 0))",
                            expected("tiu+")("tiu+")("txu+")); // TODO: should spike point be duplicated?
    test_geometry<ls, poly>("LINESTRING(0 0,1 1,0 0)", // /
                            "POLYGON((1 0,1 1,2 1,1 0))",
                            expected("tuu+"));
    test_geometry<ls, poly>("LINESTRING(2 2,1 1,2 2)", // /
                            "POLYGON((1 0,1 1,2 1,1 0))",
                            expected("tuu+"));

    test_geometry<ls, poly>("LINESTRING(2 1,1 1,2 1)",
                            "POLYGON((1 0,1 1,2 1,1 0))",
                            expected("tcu+")("txc=")("tcc=")("txu="));

    // 21.01.2015
    test_geometry<ls, poly>("LINESTRING(1 3,3 1)",
                            "POLYGON((0 0,0 4,4 4,4 0,2 2,0 0))",
                            expected("mcu+")("mxc="));
    // extended
    test_geometry<ls, poly>("LINESTRING(1 7,4 4,7 1)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("tcu+")("mxc="));
    test_geometry<ls, poly>("LINESTRING(1 7,3 5,7 1)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("mcu+")("mxc="));
    test_geometry<ls, poly>("LINESTRING(1 7,5 3,7 1)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("mcu+")("mxc="));
    test_geometry<ls, poly>("LINESTRING(4 4,7 1)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("tcu+")("mxc="));
    test_geometry<ls, poly>("LINESTRING(5 3,7 1)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("mcu+")("mxc="));
    // reversed
    test_geometry<ls, poly>("LINESTRING(7 1,4 4,1 7)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("mcc+")("tiu="));
    test_geometry<ls, poly>("LINESTRING(7 1,3 5,1 7)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("mcc+")("miu="));
    test_geometry<ls, poly>("LINESTRING(7 1,5 3,1 7)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("mcc+")("ccc=")("miu="));
    test_geometry<ls, poly>("LINESTRING(7 1,4 4)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("mcc+")("txu="));
    test_geometry<ls, poly>("LINESTRING(7 1,5 3)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("mcc+")("mxu="));
    test_geometry<ls, poly>("LINESTRING(7 1,3 5)",
                            "POLYGON((0 0,0 8,8 8,8 0,4 4,0 0))",
                            expected("mcc+")("miu="));

    // 23.01.2015 - spikes
    test_geometry<ls, poly>("LINESTRING(3 10, 1 5, 1 10, 3 4, 7 8, 6 10, 10 2)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            expected("miu+")("miu+")("miu+")("mxu+"));
    // extended
    test_geometry<ls, poly>("LINESTRING(7 8, 6 10, 11 0)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0))",
                            expected("miu+")("iuu+"));

    // 25.01.2015
    test_geometry<ls, poly>("LINESTRING(2 3, 4 5, 0 6, 5 6)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(4 4,6 4,6 6,4 6,4 4))",
                            expected("miu+")("miu+")("mcu+")("mxc="));
    test_geometry<ls, poly>("LINESTRING(0 6, 5 6)",
                            "POLYGON((0 0,0 10,10 10,10 0,0 0),(4 4,6 4,6 6,4 6,4 4))",
                            expected("miu+")("mcu+")("mxc="));

}

int test_main(int, char* [])
{
    test_all<float>();
    test_all<double>();

#if ! defined(_MSC_VER)
    test_all<long double>();
#endif

    return 0;
}
