/*
 * triangle_mesh.cpp
 *
 *  Created on: Apr 6, 2013
 *      Author: cds
 */

#include "triangle_mesh.h"
#include <stdexcept>
#include <iterator>
#include <algorithm>
#include <utility>
#include <boost/bind.hpp>
#include <boost/function.hpp>

using std::vector;
using std::ostream;

///////////////////////////
// mesh_edge

bool mesh_edge::operator==(const mesh_edge& e) const
{
	if (e.get_start_point() != this->get_start_point())
		return false;

	if (e.get_end_point() != this->get_end_point())
		return false;

	if (e.is_lamina() != this->is_lamina())
		return false;

	return true;
}

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

maths::vector3d mesh_edge::get_start_point() const
{
	return get_start_vertex()->get_point();
}

maths::vector3d mesh_edge::get_end_point() const
{
	return get_end_vertex()->get_point();
}

ostream& operator<<(ostream& os, const mesh_edge& edge)
{
	os << *(edge.get_vertex()) << " => " << *(edge.get_end_vertex());
	return os;
}

///////////////////////
// mesh_facet

maths::triangle3d mesh_facet::get_triangle() const
{
	vector<mesh_vertex_ptr> verts = get_verts();
	maths::triangle3d triangle(verts[0]->get_point(), verts[1]->get_point(), verts[2]->get_point());

	return triangle;
}

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

ostream& operator<<(ostream& os, const mesh_facet& facet)
{
	const std::vector<mesh_edge_ptr> edges = facet.get_edges();
	for (std::vector<mesh_edge_ptr>::const_iterator ei = edges.begin() ; ei != edges.end() ; ++ei)
	{
		os << (ei != edges.begin() ? " --> " : "") << *ei << std::endl;
	}

	// Also print normal and adjacent facets?

	return os;
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

	vector<mesh_facet_ptr> adj_facets = get_adjacent_facets();
	for (vector<mesh_facet_ptr>::iterator f = adj_facets.begin() ; f != adj_facets.end() ; ++f)
		vert_normal += (*f)->get_normal();

	vert_normal /= (double) adj_facets.size();
	vert_normal.unit();

	return vert_normal;
}

ostream& operator<<(ostream& os, const mesh_vertex& vertex)
{
	os << vertex.get_point();
	return os;
}

///////////////////////////////////////
/// triangle_mesh

triangle_mesh::triangle_mesh(const vector<maths::triangle3d>& triangles)
{
	build(triangles);
}

triangle_mesh::triangle_mesh(const triangle_mesh& mesh)
{
	_copy(mesh);
}

triangle_mesh& triangle_mesh::operator=(const triangle_mesh& mesh)
{
	reset();
	_copy(mesh);

	return *this;
}

bool triangle_mesh::operator==(const triangle_mesh& other) const
{
	vector<mesh_edge_ptr> this_edges = this->get_edges();
	vector<mesh_edge_ptr> other_edges = other.get_edges();

	// Use some quick and easy cases to early-out
	if (this_edges.size() != other_edges.size())
		return false;

	if (this->is_manifold() != other.is_manifold())
		return false;

	// Sort each edge based on its start and end points
	std::sort(this_edges.begin(), this_edges.end(), compare_edges());
	std::sort(other_edges.begin(), other_edges.end(), compare_edges());

	return std::equal(this_edges.begin(), this_edges.end(), other_edges.begin(), mesh_edge::are_equal);
}

void triangle_mesh::reset()
{
	_destroy();

	m_bbox = maths::bbox3d();

	m_edges.clear();
	m_verts.clear();
	m_facets.clear();
}

void triangle_mesh::_destroy()
{
	for (size_t i = 0 ; i < m_edges.size() ; i++)
		delete m_edges[i];
	for (size_t i = 0 ; i < m_verts.size() ; i++)
		delete m_verts[i];
	for (size_t i = 0 ; i < m_facets.size() ; i++)
		delete m_facets[i];
}

void triangle_mesh::_copy(const triangle_mesh& mesh)
{
	if (!is_empty())
		throw std::runtime_error("Cannot copy into a non-empty mesh!");

	std::vector<mesh_facet_ptr> mesh_facets = mesh.get_facets();
	std::vector<maths::triangle3d> triangles(mesh_facets.size());
	std::transform(mesh_facets.begin(), mesh_facets.end(), triangles.begin(), boost::bind(&mesh_facet::get_triangle, _1));

	build(triangles);
}

bool triangle_mesh::is_empty() const
{
	return m_edges.empty();
}

void triangle_mesh::_add_triangle(const maths::triangle3d& t)
{
	vector<mesh_edge_ptr> triangle_edges;

	// First, connect the edges of the triangle
	for (int i = 0 ; i < 3 ; i++)
	{
		mesh_edge_ptr e(new mesh_edge);
		triangle_edges.push_back(e);
	}

	for (int i = 0 ; i < 3 ; i++)
	{
		triangle_edges[i]->set_next_edge(triangle_edges[(i + 1) % 3]);
		triangle_edges[i]->set_prev_edge(i == 0 ? triangle_edges[2] : triangle_edges[i - 1]);
	}

	// Set the vertices of this triangle
	for (int i = 0 ; i < 3 ; i++)
	{
		mesh_edge_ptr e = triangle_edges[i];
		const maths::vector3d& e_v = t[i];

		vertex_edge_map_t::iterator ve = m_vertex_edge_map.find(e_v);
		if (ve == m_vertex_edge_map.end())
		{
			mesh_vertex_ptr edge_start_vert(new mesh_vertex(e_v));
			e->set_vertex(edge_start_vert);
			edge_start_vert->set_edge(e);

			// Insert this edge / vertex pair into our map
			std::vector<mesh_edge_ptr> vertex_edges;
			vertex_edges.push_back(e);
			m_vertex_edge_map.insert(std::make_pair(e_v, vertex_edges));

			// Insert the vertex to the global list of vertices
			m_verts.push_back(edge_start_vert);
		}
		else
		{
			std::vector<mesh_edge_ptr>& vertex_edges = ve->second;
			if (vertex_edges.empty())
				throw std::runtime_error("Empty edge / vertices association");

			mesh_vertex_ptr v = (*vertex_edges.begin())->get_vertex();
			if (std::find_if(vertex_edges.begin() + 1, vertex_edges.end(), boost::bind(&mesh_edge::get_vertex, _1) != v) != vertex_edges.end())
				throw std::runtime_error("bad edge vertex");

			if (!v)
				throw std::runtime_error("got NULL vertex!");

			e->set_vertex(v);

			std::vector<mesh_edge_ptr>::iterator p_sym_edge = std::find_if(vertex_edges.begin(), vertex_edges.end(),
				(boost::bind(&mesh_vertex::get_point, boost::bind(&mesh_edge::get_end_vertex, boost::bind(&mesh_edge::get_prev_edge, _1))) == e_v) &&
				(boost::bind(&mesh_vertex::get_point, boost::bind(&mesh_edge::get_vertex, boost::bind(&mesh_edge::get_prev_edge, _1))) == t[i + 1]));

			if (p_sym_edge != vertex_edges.end())
			{
				mesh_edge_ptr e_sym = (*p_sym_edge)->get_prev_edge();

				if (e_sym->get_end_vertex()->get_point() != e_v)
					throw std::runtime_error("symmetric edge point mismatch");

				e->set_sym_edge(e_sym);
				e_sym->set_sym_edge(e);
			}

			vertex_edges.push_back(e);
		}
	}

	// Set the facet of this triangle, and set the start edge of the facet
	mesh_facet_ptr f(new mesh_facet(t.normal()));
	std::for_each(triangle_edges.begin(), triangle_edges.end(), boost::bind(&mesh_edge::set_facet, _1, f));
	f->set_edge(triangle_edges.back());
	m_facets.push_back(f);

	// Add the triangle edges to the list of edges
	std::copy(triangle_edges.begin(), triangle_edges.end(), std::back_inserter(m_edges));
}

void triangle_mesh::build(const vector<maths::triangle3d>& triangles)
{
	if (!is_empty())
		reset();

	m_vertex_edge_map.clear();
	std::for_each(triangles.begin(), triangles.end(), boost::bind(&triangle_mesh::_add_triangle, this, _1));
	m_vertex_edge_map.clear();	// don't need this no mo

	// DEBUG

}

const maths::bbox3d& triangle_mesh::bbox() const
{
	if (m_bbox.is_empty())
	{
		maths::bbox3d mesh_bbox;

		std::vector<maths::vector3d> vertex_points(m_verts.size());
		std::transform(m_verts.begin(), m_verts.end(), vertex_points.begin(), boost::bind(&mesh_vertex::get_point, _1));
		mesh_bbox.add_points(vertex_points.begin(), vertex_points.end());

		m_bbox = mesh_bbox;
	}
	return m_bbox;
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

triangle_mesh::vbo_data_t triangle_mesh::get_vbo_data() const
{
	vbo_data_t vbo_data;

	// First, build the index list
	// Maybe there is a more efficient way to do this?
	typedef std::map<maths::vector3d, unsigned int, compare_points> vertex_index_map_t;
	vertex_index_map_t vertex_index_map;

	std::vector<mesh_vertex_ptr> mesh_verts = get_vertices();
	std::vector<mesh_facet_ptr> mesh_facets = get_facets();
	for (std::vector<mesh_facet_ptr>::iterator it = mesh_facets.begin() ; it != mesh_facets.end() ; ++it)
	{
		mesh_facet_ptr facet = *it;
		std::vector<mesh_vertex_ptr> facet_verts = facet->get_verts();

		for (size_t i = 0 ; i < 3 ; i++)
		{
			mesh_vertex_ptr vi = facet_verts[i];
			vertex_index_map_t::iterator vert_index = vertex_index_map.find(vi->get_point());
			if (vert_index == vertex_index_map.end())
			{
				const maths::vector3d& vi_pt = vi->get_point();
				std::vector<mesh_vertex_ptr>::iterator matching_vert =
					std::find_if(mesh_verts.begin(), mesh_verts.end(), boost::bind(&mesh_vertex::get_point, _1) == vi_pt);

				if (matching_vert == mesh_verts.end())
					throw std::runtime_error("Couldn't find matching vert!");

				// Constant-time complexity for random access iterator
				const unsigned int idx = (unsigned int) std::distance(mesh_verts.begin(), matching_vert);
				std::pair<vertex_index_map_t::iterator, bool> res = vertex_index_map.insert(std::make_pair(vi_pt, idx));

				vert_index = res.first;
			}

			vbo_data.indices.push_back(vert_index->second);
		}
	}

	// Next, add the normals and vertices
	for (std::vector<mesh_vertex_ptr>::iterator vi = mesh_verts.begin() ; vi != mesh_verts.end() ; ++vi)
	{
		mesh_vertex_ptr mesh_vert = *vi;

		double* vert = new double[3];
		double* normal = new double[3];
		vert[0] = mesh_vert->get_point().x();
		vert[1] = mesh_vert->get_point().y();
		vert[2] = mesh_vert->get_point().z();

		normal[0] = mesh_vert->get_normal().x();
		normal[1] = mesh_vert->get_normal().y();
		normal[2] = mesh_vert->get_normal().z();

		vbo_data.verts.push_back(vert);
		vbo_data.normals.push_back(normal);
	}

	return vbo_data;
}
