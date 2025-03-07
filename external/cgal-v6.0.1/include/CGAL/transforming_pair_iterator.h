// Copyright (c) 2014
// INRIA Saclay-Ile de France (France)
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/STL_Extension/include/CGAL/transforming_pair_iterator.h $
// $Id: include/CGAL/transforming_pair_iterator.h 50cfbde3b84 $
// SPDX-License-Identifier: LGPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s)     : Marc Glisse

#ifndef CGAL_TRANSFORMING_PAIR_ITERATOR_H
#define CGAL_TRANSFORMING_PAIR_ITERATOR_H
// Should be a combination of transform_iterator and zip_iterator,
// but boost's iterator_category games are a pain.

#include <CGAL/transforming_iterator.h>
#include <CGAL/assertions.h>

namespace CGAL {
namespace internal {
template <class Cat1, class Cat2, bool=std::is_convertible<Cat1,Cat2>::value>
struct Min_category {
        static_assert(std::is_convertible<Cat2,Cat1>::value);
        typedef Cat1 type;
};

template <class Cat1, class Cat2>
struct Min_category<Cat1,Cat2,true> {
        typedef Cat2 type;
};


template <typename Derived, typename F, typename It1, typename It2, typename Ref, typename Val>
class transforming_pair_iterator_helper
{
        typedef typename Min_category<
                typename std::iterator_traits<It1>::iterator_category,
                typename std::iterator_traits<It1>::iterator_category>
                        ::type iterator_category;

        typedef typename Default::Get<Ref,
                decltype(std::declval<F>()(std::declval<typename std::iterator_traits<It1>::reference>(),std::declval<typename std::iterator_traits<It2>::reference>()))
                        >::type reference;

        typedef typename Default::Get<Val,std::remove_cv_t<std::remove_reference_t<reference>>>::type value_type;

        public:
        typedef boost::iterator_facade<
                Derived,
                value_type,
                iterator_category,
                reference
        // expect ptrdiff_t is good enough for difference
                        > type;
};
}

template <typename F, typename It1, typename It2, typename Ref=Default, typename Val=Default>
class transforming_pair_iterator
: public internal::transforming_pair_iterator_helper<transforming_pair_iterator<F,It1,It2,Ref,Val>,F,It1,It2,Ref,Val>::type,
private internal::Functor_as_base<F>
{
        It1 iter1; It2 iter2;
        friend class boost::iterator_core_access;
        typedef typename internal::transforming_pair_iterator_helper<transforming_pair_iterator,F,It1,It2,Ref,Val>::type Base;
        typedef internal::Functor_as_base<F> Functor_base;
        typename Base::reference dereference()const{
                return functor()(*iter1,*iter2);
        }
        bool equal(transforming_pair_iterator const&i)const{
                bool b=(iter1==i.iter1);
                CGAL_assertion(b==(iter2==i.iter2));
                //FIXME: or do we want only one driving iterator
                return b;
        }
        void increment(){ ++iter1; ++iter2; }
        void decrement(){ --iter1; --iter2; }
        void advance(std::ptrdiff_t n){
                std::advance(iter1,n);
                std::advance(iter2,n);
        }
        std::ptrdiff_t distance_to(transforming_pair_iterator const&i)const{
                std::ptrdiff_t dist=std::distance(iter1,i.iter1);
                CGAL_assertion(dist==std::distance(iter2,i.iter2));
                return dist;
        }
        public:
        using Functor_base::functor;
        transforming_pair_iterator(){}
        explicit transforming_pair_iterator(It1 i1,It2 i2,F const& f=F())
                :Functor_base(f),iter1(i1),iter2(i2){}
        template<class F2,class J1,class J2,class R2,class V2>
        transforming_pair_iterator(
                transforming_pair_iterator<F2,J1,J2,R2,V2> const&i,
                std::enable_if_t<std::is_convertible<J1, It1>::value>* = 0,
                std::enable_if_t<std::is_convertible<J2, It2>::value>* = 0,
                std::enable_if_t<std::is_convertible<F2, F>::value>* = 0)
                : Functor_base(i.functor()),iter1(i.iter1),iter2(i.iter2) {}

};

template <typename F, typename It1, typename It2> inline
transforming_pair_iterator<F,It1,It2> make_transforming_pair_iterator(It1 i1, It2 i2, F const&f=F()) {
        return transforming_pair_iterator<F,It1,It2>(i1,i2,f);
}

}

#endif // CGAL_TRANSFORMING_PAIR_ITERATOR_H
