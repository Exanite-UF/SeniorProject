// Copyright (c) 2017  GeometryFactory (France).  All rights reserved.
//
// This file is part of CGAL (www.cgal.org)
//
// $URL: https://github.com/CGAL/cgal/blob/v6.0.1/Surface_mesh/include/CGAL/boost/graph/properties_Surface_mesh_features.h $
// $Id: include/CGAL/boost/graph/properties_Surface_mesh_features.h 50cfbde3b84 $
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Commercial
//
//
// Author(s)     : Andreas Fabri

#ifndef CGAL_PROPERTIES_SURFACE_MESH_FEATURES_H
#define CGAL_PROPERTIES_SURFACE_MESH_FEATURES_H

#include <CGAL/license/Surface_mesh.h>


#ifndef DOXYGEN_RUNNING

#include <CGAL/Surface_mesh.h>
#include <set>

namespace boost {

template <typename P, typename I>
struct property_map<CGAL::Surface_mesh<P>, CGAL::face_patch_id_t<I> >
{

  typedef typename boost::graph_traits<CGAL::Surface_mesh<P> >::face_descriptor face_descriptor;

  typedef typename CGAL::Surface_mesh<P>::template Property_map<face_descriptor, I> type;
  typedef type const_type;
};


template <typename P>
struct property_map<CGAL::Surface_mesh<P>, CGAL::face_patch_id_t<void> >
{

  typedef typename boost::graph_traits<CGAL::Surface_mesh<P> >::face_descriptor face_descriptor;

  typedef CGAL::Constant_property_map<typename boost::graph_traits<CGAL::Surface_mesh<P> >::face_descriptor,std::pair<int,int> > type;
  typedef type const_type;
};


template<typename P>
struct property_map<CGAL::Surface_mesh<P>, CGAL::edge_is_feature_t>
{
  typedef typename boost::graph_traits<CGAL::Surface_mesh<P> >::edge_descriptor edge_descriptor;

  typedef typename CGAL::Surface_mesh<P>::template Property_map<edge_descriptor, bool> type;
  typedef type const_type;
};


template <typename P>
struct property_map<CGAL::Surface_mesh<P>, CGAL::vertex_feature_degree_t>
{

  typedef typename boost::graph_traits<CGAL::Surface_mesh<P> >::vertex_descriptor vertex_descriptor;

  typedef typename CGAL::Surface_mesh<P>::template Property_map<vertex_descriptor, int> type;
  typedef type const_type;
};


template <typename P, typename I>
struct property_map<CGAL::Surface_mesh<P>, CGAL::vertex_incident_patches_t<I> >
{

  typedef typename boost::graph_traits<CGAL::Surface_mesh<P> >::vertex_descriptor vertex_descriptor;

  typedef typename CGAL::Surface_mesh<P>::template Property_map<vertex_descriptor, std::set<I> > type;
  typedef type const_type;
};


} // namespace boost

namespace CGAL {

template <typename P, typename I>
typename boost::lazy_disable_if<
   std::is_const<P>,
   Get_pmap_of_surface_mesh<P, CGAL::face_patch_id_t<I> >
 >::type
inline get(CGAL::face_patch_id_t<I>, Surface_mesh<P> & smesh)
{
 typedef typename boost::graph_traits<Surface_mesh<P> >::face_descriptor face_descriptor;
  return smesh. template add_property_map<face_descriptor,I>("f:patch_id", 1).first;
}


template <typename P, typename I>
typename boost::lazy_disable_if<
   std::is_const<P>,
   Get_pmap_of_surface_mesh<P, CGAL::face_patch_id_t<I> >
 >::type
inline get(CGAL::face_patch_id_t<I>, const Surface_mesh<P> & smesh)
{
 typedef typename boost::graph_traits<Surface_mesh<P> >::face_descriptor face_descriptor;
  return smesh. template property_map<face_descriptor,I>("f:patch_id").value();
}


#define CGAL_PROPERTY_SURFACE_MESH_RETURN_TYPE(Tag) \
  typename boost::lazy_disable_if<                      \
     std::is_const<P>,                                \
     Get_pmap_of_surface_mesh<P, Tag >                  \
   >::type

template <typename P>
CGAL_PROPERTY_SURFACE_MESH_RETURN_TYPE(CGAL::face_patch_id_t<void>)
inline get(CGAL::face_patch_id_t<void>, const Surface_mesh<P> &)
{
  typedef CGAL::Constant_property_map<typename boost::graph_traits<Surface_mesh<P> >::face_descriptor,std::pair<int,int> > Pmap;

  return Pmap(std::make_pair(0,1));
}


template <typename P>
CGAL_PROPERTY_SURFACE_MESH_RETURN_TYPE(CGAL::edge_is_feature_t)
inline get(CGAL::edge_is_feature_t, Surface_mesh<P>& smesh)
{
  typedef typename boost::graph_traits<Surface_mesh<P> >::edge_descriptor edge_descriptor;
  return smesh. template add_property_map<edge_descriptor,bool>("e:is_feature", false).first;
}


template <typename P>
CGAL_PROPERTY_SURFACE_MESH_RETURN_TYPE(CGAL::edge_is_feature_t)
inline get(CGAL::edge_is_feature_t, const Surface_mesh<P>& smesh)
{
  typedef typename boost::graph_traits<Surface_mesh<P> >::edge_descriptor edge_descriptor;
  return smesh. template property_map<edge_descriptor,bool>("e:is_feature").value();
}


template <typename P>
CGAL_PROPERTY_SURFACE_MESH_RETURN_TYPE(CGAL::vertex_feature_degree_t)
inline get(CGAL::vertex_feature_degree_t, Surface_mesh<P> & smesh)
{
  typedef typename boost::graph_traits<Surface_mesh<P> >::vertex_descriptor vertex_descriptor;
  return smesh. template add_property_map<vertex_descriptor,int>("v:nfe").first;
}

template <typename P>
CGAL_PROPERTY_SURFACE_MESH_RETURN_TYPE(CGAL::vertex_feature_degree_t)
inline get(CGAL::vertex_feature_degree_t, const Surface_mesh<P> & smesh)
{
  typedef typename boost::graph_traits<Surface_mesh<P> >::vertex_descriptor vertex_descriptor;
  return smesh. template property_map<vertex_descriptor,int>("v:nfe").value();
}

template <typename P, typename I>
typename boost::lazy_disable_if<
   std::is_const<P>,
   Get_pmap_of_surface_mesh<P, CGAL::vertex_incident_patches_t<I> >
 >::type
  inline get(CGAL::vertex_incident_patches_t<I>, Surface_mesh<P> & smesh)
{
  typedef typename boost::graph_traits<Surface_mesh<P> >::vertex_descriptor vertex_descriptor;
  return smesh. template add_property_map<vertex_descriptor,std::set<I> >("v:ip").first;
}

template <typename P, typename I>
typename boost::lazy_disable_if<
   std::is_const<P>,
   Get_pmap_of_surface_mesh<P, CGAL::vertex_incident_patches_t<I> >
 >::type
  inline get(CGAL::vertex_incident_patches_t<I>, const Surface_mesh<P> & smesh)
{
  typedef typename boost::graph_traits<Surface_mesh<P> >::vertex_descriptor vertex_descriptor;
  return smesh. template property_map<vertex_descriptor,std::set<I> >("v:ip").value();
}

} // namespace CGAL

#undef CGAL_PROPERTY_SURFACE_MESH_RETURN_TYPE

#endif // DOXYGEN_RUNNING

#endif //CGAL_PROPERTIES_SURFACE_MESH_FEATURES_H
