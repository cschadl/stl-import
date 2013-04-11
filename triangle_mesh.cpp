/*
 * triangle_mesh.cpp
 *
 *  Created on: Apr 6, 2013
 *      Author: cds
 */

#include "triangle_mesh.h"
#include <stdexcept>
#include <boost/bind.hpp>

using std::vector;

///////////////////////////
// mesh_edge

vector<mesh_facet_ptr> mesh_edge::get_adjacent_facets() const
{
	vector<mesh_facet_ptr> facets;
	if (m_facet)
	{
		facets.push_back(m_facet);
		if (m_symmetric_edge)
		{
			facets.push_back(m_symmetric_edge->get_facet());
		}
	}

	return facets;
}

vector<mesh_vertex_ptr> mesh_edge::get_verts() const
{
	vector<mesh_vertex_ptr> verts;
	if (m_vert)
	{
		verts.push_back(m_vert);
		verts.push_back(get_end_vertex());
	}

	return verts;
}

mesh_vertex_ptr mesh_edge::get_end_vertex() const
{
	// We could use the symmetric edge to get the end vertex,
	// but, the edge could be lamina, and we use this when
	// we build the mesh in triangle_mesh::build()
	return m_next_edge->get_vertex();
}

///////////////////////
// mesh_facet

vector<mesh_edge_ptr> mesh_facet::get_edges() const
{
	vector<mesh_edge_ptr> edges;

	const mesh_edge_ptr& start_edge = m_edge;
	mesh_edge_ptr e = start_edge;
	do
	{
		edges.push_back(e);
		e = e->get_next_edge();
	}
	while (e != start_edge);

	return edges;
}

vector<mesh_vertex_ptr> mesh_facet::get_verts() const
{
	vector<mesh_vertex_ptr> verts;

	const mesh_edge_ptr& start_edge = m_edge;
	mesh_edge_ptr e = start_edge;
	do
	{
		verts.push_back(e->get_vertex());
		e = e->get_next_edge();
	}
	while (e != start_edge);

	return verts;
}

vector<mesh_facet_ptr> mesh_facet::get_adjacent_facets() const
{
	vector<mesh_facet_ptr> facets;

	const mesh_edge_ptr& start_edge = m_edge;
	mesh_edge_ptr e = start_edge;
	do
	{
		facets.push_back(e->get_sym_edge()->get_facet());
		// make sure facets.back() != this...
		e = e->get_next_edge();
	}
	while (e != start_edge);

	return facets;
}

//////////////////////////
// mesh_vertex

vector<mesh_edge_ptr> mesh_vertex::get_adjacent_edges() const
{
	vector<mesh_edge_ptr> edges;

	const mesh_edge_ptr& start_edge = m_edge;
	mesh_edge_ptr e = start_edge;
	do
	{
		edges.push_back(e);
		e = e->get_prev_edge()->get_sym_edge();
		if (!e)
			break;	// TODO - case when edge is lamina
	}
	while (e != start_edge);

	return edges;
}

vector<mesh_facet_ptr> mesh_vertex::get_adjacent_facets() const
{
	vector<mesh_facet_ptr> facets;

	const mesh_edge_ptr& start_edge = m_edge;
	mesh_edge_ptr e = start_edge;
	do
	{
		facets.push_back(e->get_facet());
		e = e->get_prev_edge()->get_sym_edge();
		if (!e)
			break;
	}
	while(e != start_edge);

	return facets;
}

maths::vector3d mesh_vertex::get_normal() const
{
	maths::vector3d vert_normal;

	vector<mesh_facet_ptr> facets = get_adjacent_facets();
	for (vector<mesh_facet_ptr>::iterator f = facets.begin() ; f != facets.end() ; ++f)
		vert_normal += (*f)->get_normal();

	vert_normal.unit();

	return vert_normal;
}

///////////////////////////////////////
/// triangle_mesh

triangle_mesh::triangle_mesh(const vector<maths::triangle3d>& triangles)
{
	build(triangles);
}

void triangle_mesh::reset()
{
	m_edges.clear();
	m_verts.clear();
	m_facets.clear();
}

bool triangle_mesh::is_empty() const
{
	return m_edges.empty();
}

void triangle_mesh::_add_triangle(const maths::triangle3d& t)
{
	vector<mesh_edge_ptr> triangle_edges;
	for (int i = 0 ; i < 3 ; i++)
	{
		mesh_edge_ptr e(new mesh_edge);

		vertex_edge_map_t::iterator ve = m_vertex_edge_map.find(t[i]);
		if (ve == m_vertex_edge_map.end())
		{
			mesh_vertex_ptr edge_start_vert(new mesh_vertex(t[i]));
			e->set_vertex(edge_start_vert);

			// Insert this edge / vertex pair into our map
			std::vector<mesh_edge_ptr> vertex_edges;
			vertex_edges.push_back(e);
			m_vertex_edge_map.insert(std::make_pair(t[i], vertex_edges));

			// Insert the vertex to the global list of vertices
			m_verts.push_back(edge_start_vert);
		}
		else
		{
			std::vector<mesh_edge_ptr>& vertex_edges = ve->second;
			if (vertex_edges.empty())
				throw std::runtime_error("Empty edge / vertices association");

			std::vector<mesh_edge_ptr>::iterator p_sym_edge = std::find_if(vertex_edges.begin(), vertex_edges.end(),
				boost::bind(&mesh_vertex::get_point, boost::bind(&mesh_edge::get_end_vertex, _1)) == t[i]);
			if (p_sym_edge != vertex_edges.end())
			{
				mesh_edge_ptr e_sym = *p_sym_edge;

				e->set_vertex(e_sym->get_next_edge()->get_vertex());

				e->set_sym_edge(e_sym);
				e_sym->set_sym_edge(e);
			}

			vertex_edges.push_back(e);
		}

		// Set prev edge, next edge
		if (i > 0)
		{
			e->set_prev_edge(triangle_edges[i - 1]);
			triangle_edges[i - 1]->set_next_edge(e);
		}

		if (i == 2)
			(*triangle_edges.begin())->set_prev_edge(e);

		triangle_edges.push_back(e);
	}

	mesh_facet_ptr f(new mesh_facet(t.normal()));
	m_facets.push_back(f);

	std::for_each(triangle_edges.begin(), triangle_edges.end(), boost::bind(&mesh_edge::set_facet, _1, f));
	std::copy(triangle_edges.begin(), triangle_edges.end(), std::back_inserter(m_edges));
}

void triangle_mesh::build(const vector<maths::triangle3d>& triangles)
{
	m_vertex_edge_map.clear();
	std::for_each(triangles.begin(), triangles.end(), boost::bind(&triangle_mesh::_add_triangle, this, _1));
	m_vertex_edge_map.clear();	// don't need this no mo
}

bool triangle_mesh::is_manifold() const
{
	return std::find_if(m_edges.begin(), m_edges.end(), boost::bind(&mesh_edge::is_lamina, _1)) == m_edges.end();
}

vector<mesh_edge_ptr> triangle_mesh::get_lamina_edges() const
{
	vector<mesh_edge_ptr> lamina_edges;
	std::remove_copy_if(m_edges.begin(), m_edges.end(),
		std::back_inserter(lamina_edges), !boost::bind(&mesh_edge::is_lamina, _1));

	return lamina_edges;
}
