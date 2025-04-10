// Copyright (c) 2008  GeometryFactory Sarl (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Hyperbolic_triangulation_2/demo/Hyperbolic_triangulation_2/include/internal/Qt/TriangulationRemoveVertex.h $
// $Id: demo/Hyperbolic_triangulation_2/include/internal/Qt/TriangulationRemoveVertex.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
// Author(s)     : Andreas Fabri <Andreas.Fabri@geometryfactory.com>
//

#ifndef CGAL_QT_TRIANGULATION_REMOVE_VERTEX_H
#define CGAL_QT_TRIANGULATION_REMOVE_VERTEX_H

#include <CGAL/Qt/GraphicsViewInput.h>
#include <QGraphicsSceneMouseEvent>
#include <QEvent>
#include <list>
#include <CGAL/Qt/Converter.h>



namespace CGAL {
namespace Qt {

template <typename DT>
class TriangulationRemoveVertex : public GraphicsViewInput
{
public:
  typedef typename DT::Face_handle Face_handle;
  typedef typename DT::Vertex_handle Vertex_handle;
  typedef typename DT::Point Point;

  TriangulationRemoveVertex(DT* dt_, QObject* parent);

protected:
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  bool eventFilter(QObject *obj, QEvent *event);

  DT* dt;
};

template <typename T>
TriangulationRemoveVertex<T>::TriangulationRemoveVertex(T * dt_,
                                                          QObject* parent)
  :  GraphicsViewInput(parent), dt(dt_)
{}

template <typename T>
void
TriangulationRemoveVertex<T>::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  if((event->modifiers()  & ::Qt::ShiftModifier)
     && (! (event->modifiers() & ::Qt::ControlModifier))){
    if(dt->number_of_vertices() == 0){
      dt->clear();
    }else {
      Converter<typename T::Geom_traits> convert;
      typename T::Vertex_handle selected_vertex = dt->nearest_vertex(convert(event->scenePos()));
      dt->remove(selected_vertex);
    }
    Q_EMIT (modelChanged());
  }
}

template <typename T>
bool
TriangulationRemoveVertex<T>::eventFilter(QObject *obj, QEvent *event)
{
  if(event->type() == QEvent::GraphicsSceneMousePress) {
    QGraphicsSceneMouseEvent *mouseEvent = static_cast<QGraphicsSceneMouseEvent *>(event);
    mousePressEvent(mouseEvent);
    return false;
  } else{
    // standard event processing
    return QObject::eventFilter(obj, event);
  }
}


} // namespace Qt
} // namespace CGAL

#endif // CGAL_QT_TRIANGULATION_REMOVE_VERTEX_H
