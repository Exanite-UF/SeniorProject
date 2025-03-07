//  Copyright (C) 2011 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/lockfree/spsc_queue.hpp>

#define BOOST_TEST_MAIN
#ifdef BOOST_LOCKFREE_INCLUDE_TESTS
#    include <boost/test/included/unit_test.hpp>
#else
#    include <boost/test/unit_test.hpp>
#endif

#include "test_common.hpp"

using namespace boost;
using namespace boost::lockfree;
using namespace std;

BOOST_AUTO_TEST_CASE( simple_spsc_queue_test )
{
    spsc_queue< int, capacity< 64 > > f;

    BOOST_TEST_REQUIRE( f.empty() );
    f.push( 1 );
    f.push( 2 );

    int i1( 0 ), i2( 0 );

    BOOST_TEST_REQUIRE( f.pop( i1 ) );
    BOOST_TEST_REQUIRE( i1 == 1 );

    BOOST_TEST_REQUIRE( f.pop( i2 ) );
    BOOST_TEST_REQUIRE( i2 == 2 );
    BOOST_TEST_REQUIRE( f.empty() );
}

BOOST_AUTO_TEST_CASE( simple_spsc_queue_test_compile_time_size )
{
    spsc_queue< int > f( 64 );

    BOOST_TEST_REQUIRE( f.empty() );
    f.push( 1 );
    f.push( 2 );

    int i1( 0 ), i2( 0 );

    BOOST_TEST_REQUIRE( f.pop( i1 ) );
    BOOST_TEST_REQUIRE( i1 == 1 );

    BOOST_TEST_REQUIRE( f.pop( i2 ) );
    BOOST_TEST_REQUIRE( i2 == 2 );
    BOOST_TEST_REQUIRE( f.empty() );
}

BOOST_AUTO_TEST_CASE( ranged_push_test )
{
    spsc_queue< int > stk( 64 );

    int data[ 2 ] = { 1, 2 };

    BOOST_TEST_REQUIRE( stk.push( data, data + 2 ) == data + 2 );

    int out;
    BOOST_TEST_REQUIRE( stk.pop( out ) );
    BOOST_TEST_REQUIRE( out == 1 );
    BOOST_TEST_REQUIRE( stk.pop( out ) );
    BOOST_TEST_REQUIRE( out == 2 );
    BOOST_TEST_REQUIRE( !stk.pop( out ) );
}

BOOST_AUTO_TEST_CASE( spsc_queue_consume_one_test )
{
    spsc_queue< int > f( 64 );

    BOOST_WARN( f.is_lock_free() );
    BOOST_TEST_REQUIRE( f.empty() );

    f.push( 1 );
    f.push( 2 );

    bool success1 = f.consume_one( []( int i ) {
        BOOST_TEST_REQUIRE( i == 1 );
    } );

    bool success2 = f.consume_one( []( int i ) {
        BOOST_TEST_REQUIRE( i == 2 );
    } );

    BOOST_TEST_REQUIRE( success1 );
    BOOST_TEST_REQUIRE( success2 );

    BOOST_TEST_REQUIRE( f.empty() );
}

BOOST_AUTO_TEST_CASE( spsc_queue_consume_all_test )
{
    spsc_queue< int > f( 64 );

    BOOST_WARN( f.is_lock_free() );
    BOOST_TEST_REQUIRE( f.empty() );

    f.push( 1 );
    f.push( 2 );

    size_t consumed = f.consume_all( []( int i ) {} );

    BOOST_TEST_REQUIRE( consumed == 2u );

    BOOST_TEST_REQUIRE( f.empty() );
}

enum
{
    pointer_and_size,
    reference_to_array,
    iterator_pair,
    span_,
    output_iterator_
};

BOOST_AUTO_TEST_CASE( spsc_queue_capacity_test )
{
    spsc_queue< int, capacity< 2 > > f;

    BOOST_TEST_REQUIRE( f.push( 1 ) );
    BOOST_TEST_REQUIRE( f.push( 2 ) );
    BOOST_TEST_REQUIRE( !f.push( 3 ) );

    spsc_queue< int > g( 2 );

    BOOST_TEST_REQUIRE( g.push( 1 ) );
    BOOST_TEST_REQUIRE( g.push( 2 ) );
    BOOST_TEST_REQUIRE( !g.push( 3 ) );
}

template < typename QueueType >
void spsc_queue_avail_test_run( QueueType& q )
{
    BOOST_TEST_REQUIRE( q.write_available() == 16 );
    BOOST_TEST_REQUIRE( q.read_available() == 0 );

    for ( size_t i = 0; i != 8; ++i ) {
        BOOST_TEST_REQUIRE( q.write_available() == 16 - i );
        BOOST_TEST_REQUIRE( q.read_available() == i );

        q.push( 1 );
    }

    // empty queue
    int dummy;
    while ( q.pop( dummy ) ) {}

    for ( size_t i = 0; i != 16; ++i ) {
        BOOST_TEST_REQUIRE( q.write_available() == 16 - i );
        BOOST_TEST_REQUIRE( q.read_available() == i );

        q.push( 1 );
    }
}

BOOST_AUTO_TEST_CASE( spsc_queue_avail_test )
{
    spsc_queue< int, capacity< 16 > > f;
    spsc_queue_avail_test_run( f );

    spsc_queue< int > g( 16 );
    spsc_queue_avail_test_run( g );
}


template < int EnqueueMode >
void spsc_queue_buffer_push_return_value( void )
{
    const size_t                       xqueue_size = 64;
    const size_t                       buffer_size = 100;
    spsc_queue< int, capacity< 100 > > rb;

    int data[ xqueue_size ];
    for ( size_t i = 0; i != xqueue_size; ++i )
        data[ i ] = (int)i * 2;

    switch ( EnqueueMode ) {
    case pointer_and_size:   BOOST_TEST_REQUIRE( rb.push( data, xqueue_size ) == xqueue_size ); break;
    case reference_to_array: BOOST_TEST_REQUIRE( rb.push( data ) == xqueue_size ); break;
    case iterator_pair:      BOOST_TEST_REQUIRE( rb.push( data, data + xqueue_size ) == data + xqueue_size ); break;
    case span_:              BOOST_TEST_REQUIRE( rb.push( boost::span< const int >( data, xqueue_size ) ) == xqueue_size ); break;
    default:                 assert( false );
    }

    switch ( EnqueueMode ) {
    case pointer_and_size:   BOOST_TEST_REQUIRE( rb.push( data, xqueue_size ) == buffer_size - xqueue_size ); break;
    case reference_to_array: BOOST_TEST_REQUIRE( rb.push( data ) == buffer_size - xqueue_size ); break;
    case span_:
        BOOST_TEST_REQUIRE( rb.push( boost::span< const int >( data, xqueue_size ) ) == buffer_size - xqueue_size );
        break;
    case iterator_pair:
        BOOST_TEST_REQUIRE( rb.push( data, data + xqueue_size ) == data + buffer_size - xqueue_size );
        break;

    default: assert( false );
    }
}

BOOST_AUTO_TEST_CASE( spsc_queue_buffer_push_return_value_test )
{
    spsc_queue_buffer_push_return_value< pointer_and_size >();
    spsc_queue_buffer_push_return_value< reference_to_array >();
    spsc_queue_buffer_push_return_value< iterator_pair >();
    spsc_queue_buffer_push_return_value< span_ >();
}

template < int EnqueueMode, int ElementCount, int BufferSize, int NumberOfIterations >
void spsc_queue_buffer_push( void )
{
    const size_t                              xqueue_size = ElementCount;
    spsc_queue< int, capacity< BufferSize > > rb;

    int data[ xqueue_size ];
    for ( size_t i = 0; i != xqueue_size; ++i )
        data[ i ] = (int)i * 2;

    std::vector< int > vdata( data, data + xqueue_size );

    for ( int i = 0; i != NumberOfIterations; ++i ) {
        BOOST_TEST_REQUIRE( rb.empty() );
        switch ( EnqueueMode ) {
        case pointer_and_size:   BOOST_TEST_REQUIRE( rb.push( data, xqueue_size ) == xqueue_size ); break;
        case reference_to_array: BOOST_TEST_REQUIRE( rb.push( data ) == xqueue_size ); break;
        case iterator_pair:      BOOST_TEST_REQUIRE( rb.push( data, data + xqueue_size ) == data + xqueue_size ); break;
        case span_:
            BOOST_TEST_REQUIRE( rb.push( boost::span< const int >( data, xqueue_size ) ) == xqueue_size );
            break;

        default: assert( false );
        }

        int out[ xqueue_size ];
        BOOST_TEST_REQUIRE( rb.pop( out, xqueue_size ) == xqueue_size );
        for ( size_t i = 0; i != xqueue_size; ++i )
            BOOST_TEST_REQUIRE( data[ i ] == out[ i ] );
    }
}

BOOST_AUTO_TEST_CASE( spsc_queue_buffer_push_test )
{
    spsc_queue_buffer_push< pointer_and_size, 7, 16, 64 >();
    spsc_queue_buffer_push< reference_to_array, 7, 16, 64 >();
    spsc_queue_buffer_push< iterator_pair, 7, 16, 64 >();
    spsc_queue_buffer_push< span_, 7, 16, 64 >();
}

template < int EnqueueMode, int ElementCount, int BufferSize, int NumberOfIterations >
void spsc_queue_buffer_pop( void )
{
    const size_t                              xqueue_size = ElementCount;
    spsc_queue< int, capacity< BufferSize > > rb;

    int data[ xqueue_size ];
    for ( size_t i = 0; i != xqueue_size; ++i )
        data[ i ] = (int)i * 2;

    std::vector< int > vdata( data, data + xqueue_size );

    for ( int i = 0; i != NumberOfIterations; ++i ) {
        BOOST_TEST_REQUIRE( rb.empty() );
        BOOST_TEST_REQUIRE( rb.push( data ) == xqueue_size );

        int           out[ xqueue_size ];
        vector< int > vout;

        switch ( EnqueueMode ) {
        case pointer_and_size:   BOOST_TEST_REQUIRE( rb.pop( out, xqueue_size ) == xqueue_size ); break;
        case reference_to_array: BOOST_TEST_REQUIRE( rb.pop( out ) == xqueue_size ); break;
        case output_iterator_:   BOOST_TEST_REQUIRE( rb.pop( std::back_inserter( vout ) ) == xqueue_size ); break;
        default:                 assert( false );
        }

        if ( EnqueueMode == output_iterator_ ) {
            BOOST_TEST_REQUIRE( vout.size() == xqueue_size );
            for ( size_t i = 0; i != xqueue_size; ++i )
                BOOST_TEST_REQUIRE( data[ i ] == vout[ i ] );
        } else {
            for ( size_t i = 0; i != xqueue_size; ++i )
                BOOST_TEST_REQUIRE( data[ i ] == out[ i ] );
        }
    }
}

BOOST_AUTO_TEST_CASE( spsc_queue_buffer_pop_test )
{
    spsc_queue_buffer_pop< pointer_and_size, 7, 16, 64 >();
    spsc_queue_buffer_pop< reference_to_array, 7, 16, 64 >();
    spsc_queue_buffer_pop< output_iterator_, 7, 16, 64 >();
}

// Test front() and pop()
template < typename Queue >
void spsc_queue_front_pop( Queue& queue )
{
    queue.push( 1 );
    queue.push( 2 );
    queue.push( 3 );

    // front as ref and const ref
    int&       rfront  = queue.front();
    const int& crfront = queue.front();

    BOOST_TEST_REQUIRE( 1 == rfront );
    BOOST_TEST_REQUIRE( 1 == crfront );

    int front = 0;

    // access element pushed first
    front = queue.front();
    BOOST_TEST_REQUIRE( 1 == front );

    // front is still the same
    front = queue.front();
    BOOST_TEST_REQUIRE( 1 == front );

    queue.pop();

    front = queue.front();
    BOOST_TEST_REQUIRE( 2 == front );

    queue.pop();                // pop 2

    bool pop_ret = queue.pop(); // pop 3
    BOOST_TEST_REQUIRE( pop_ret );

    pop_ret = queue.pop(); // pop on empty queue
    BOOST_TEST_REQUIRE( !pop_ret );
}

BOOST_AUTO_TEST_CASE( spsc_queue_buffer_front_and_pop_runtime_sized_test )
{
    spsc_queue< int, capacity< 64 > > queue;
    spsc_queue_front_pop( queue );
}

BOOST_AUTO_TEST_CASE( spsc_queue_buffer_front_and_pop_compiletime_sized_test )
{
    spsc_queue< int > queue( 64 );
    spsc_queue_front_pop( queue );
}

BOOST_AUTO_TEST_CASE( spsc_queue_reset_test )
{
    spsc_queue< int, capacity< 64 > > f;

    BOOST_TEST_REQUIRE( f.empty() );
    f.push( 1 );
    f.push( 2 );

    f.reset();

    BOOST_TEST_REQUIRE( f.empty() );
}

BOOST_AUTO_TEST_CASE( move_semantics )
{
    boost::lockfree::spsc_queue< std::unique_ptr< int >, boost::lockfree::capacity< 128 > > stk;

    stk.push( std::make_unique< int >( 0 ) );
    stk.push( std::make_unique< int >( 1 ) );

    auto two = std::make_unique< int >( 2 );
    stk.push( std::move( two ) );

    std::unique_ptr< int > out;
    BOOST_TEST_REQUIRE( stk.pop( out ) );
    BOOST_TEST_REQUIRE( *out == 0 );

    stk.consume_one( []( std::unique_ptr< int > one ) {
        BOOST_TEST_REQUIRE( *one == 1 );
    } );

    stk.consume_all( []( std::unique_ptr< int > ) {} );
}

#if !defined( BOOST_NO_CXX17_HDR_OPTIONAL )

BOOST_AUTO_TEST_CASE( queue_uses_optional )
{
    boost::lockfree::spsc_queue< int > stk( 5 );

    bool pop_to_nullopt = stk.pop( boost::lockfree::uses_optional ) == std::nullopt;
    BOOST_TEST_REQUIRE( pop_to_nullopt );

    stk.push( 53 );
    bool pop_to_optional = stk.pop( boost::lockfree::uses_optional ) == 53;
    BOOST_TEST_REQUIRE( pop_to_optional );
}

#endif
