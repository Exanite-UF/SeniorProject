// Copyright (c) 2011-2018  INRIA Sophia Antipolis, INRIA Nancy (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Hyperbolic_triangulation_2/demo/Hyperbolic_triangulation_2/include/internal/Qt/HyperbolicPainterOstream.h $
// $Id: demo/Hyperbolic_triangulation_2/include/internal/Qt/HyperbolicPainterOstream.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s)     : Mikhail Bogdanov

#ifndef CGAL_HYPERBOLIC_PAINTER_OSTREAM_H
#define CGAL_HYPERBOLIC_PAINTER_OSTREAM_H

#include <CGAL/Qt/PainterOstream.h>
#include <CGAL/Hyperbolic_Delaunay_triangulation_traits_2.h>

namespace CGAL{

namespace Qt {

template <typename K>
class PainterOstream< Hyperbolic_Delaunay_triangulation_traits_2<K> >
  : public PainterOstream<K>
{
  typedef PainterOstream<K>                                         Base;
  typedef PainterOstream<Hyperbolic_Delaunay_triangulation_traits_2<K> >   Self;
  typedef Hyperbolic_Delaunay_triangulation_traits_2<K>             Gt;

  typedef typename Gt::Hyperbolic_segment_2                         Hyperbolic_segment_2;
  typedef typename Gt::Circular_arc_2                               Circular_arc_2;
  typedef typename Gt::Euclidean_segment_2                          Euclidean_segment_2;
  typedef typename Gt::Point_2                                      Point_2;

private:
  PainterOstream& operator<<(const Circular_arc_2& arc)
  {
    const typename K::Circle_2 & circ  = arc.supporting_circle();
    const typename K::Point_2 & center = circ.center();
    const typename K::Point_2 & source = arc.source();
    const typename K::Point_2 & target = arc.target();

    double asource = std::atan2( -to_double(source.y() - center.y()),
                                 to_double(source.x() - center.x()));
    double atarget = std::atan2( -to_double(target.y() - center.y()),
                                 to_double(target.x() - center.x()));

    std::swap(asource, atarget);
    double aspan = atarget - asource;

    if(aspan < 0.)
      aspan += 2 * CGAL_PI;

    const double coeff = 180*16/CGAL_PI;
    qp->drawArc(convert(circ.bbox()), int(asource * coeff), int(aspan * coeff));

    return *this;
  }


  PainterOstream& operator<<(const Euclidean_segment_2& seg)
  {
    const Point_2 & source = seg.source();
    const Point_2 & target = seg.target();

    QPointF src(to_double(source.x()), to_double(source.y()));
    QPointF tgt(to_double(target.x()), to_double(target.y()));

    qp->drawLine(src, tgt);

    return *this;
  }

public:
  PainterOstream(QPainter* p, QRectF rect = QRectF())
    : Base(p, rect), qp(p), convert(rect)
  {}

  using Base::operator <<;

  PainterOstream& operator<<(const Hyperbolic_segment_2& s)
  {
    if(const Euclidean_segment_2* seg = std::get_if<Euclidean_segment_2>(&s)) {
      CGAL::Qt::PainterOstream<K>::operator << (*seg);
      return *this;
    }

    const Circular_arc_2& arc = std::get<Circular_arc_2>(s);

    if(arc.squared_radius() > 100) {
      Euclidean_segment_2 seg(arc.source(), arc.target());
      CGAL::Qt::PainterOstream<K>::operator << (seg);
      return *this;
    }

    operator<<(arc);
    return *this;
  }

private:
  // ToDo: These objects must be deleted
  // Copies of these objects are in the base class.
  // We need access to the copies in the base class.
  QPainter* qp;
  Converter<K> convert;
};

} //namespace Qt

} //namespace CGAL

#endif // CGAL_HYPERBOLIC_PAINTER_OSTREAM_H

