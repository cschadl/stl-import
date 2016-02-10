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
#include <tuple>
#include <iomanip>
#include <functional>

using std::vector;
using std::ostream;
using std::setprecision;
using namespace std::placeholders;

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

	mesh_facet_ptr facet = get_facet();
	if (!facet)
	{
		facets.push_back(facet);

		mesh_edge_ptr sym_edge = get_sym_edge();
		if (sym_edge)
		{
			facets.push_back(sym_edge->get_facet());
		}
	}

	return facets;
}

vector<mesh_vertex_ptr> mesh_edge::get_verts() const
{
	vector<mesh_vertex_ptr> verts;
	if (!m_vert.expired())
	{
		verts.push_back(get_vertex());
		verts.push_back(get_end_vertex());
	}

	return verts;
}

mesh_vertex_ptr mesh_edge::get_end_vertex() const
{
	// We could use the symmetric edge to get the end vertex,
	// but, the edge could be lamina, and we use this when
	// we build the mesh in triangle_mesh::build()
	return get_next_edge()->get_vertex();
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

	mesh_edge_ptr start_edge = m_edge.lock();
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

	mesh_edge_ptr start_edge = m_edge.lock();
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

	mesh_edge_ptr start_edge = m_edge.lock();
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
	for (auto ei = edges.begin() ; ei != edges.end() ; ++ei)
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

	mesh_edge_ptr start_edge = m_edge.lock();
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

	mesh_edge_ptr start_edge = m_edge.lock();
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
	for (const auto& f : adj_facets)
		vert_normal += f->get_normal();

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

	return std::equal(this_edges.begin(), this_edges.end(), other_edges.begin(),
		[](mesh_edge_ptr e1, mesh_edge_ptr e2) { return *e1 == *e2; });
}

void triangle_mesh::reset()
{
	m_bbox = maths::bbox3d();

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
			if (std::find_if(vertex_edges.begin() + 1, vertex_edges.end(),
				[&v](const mesh_edge_ptr & e) { return e->get_vertex() != v; }) != vertex_edges.end())
			{
				throw std::runtime_error("bad edge vertex");
			}

			if (!v)
				throw std::runtime_error("got NULL vertex!");

			e->set_vertex(v);

			std::vector<mesh_edge_ptr>::iterator p_sym_edge = std::find_if(vertex_edges.begin(), vertex_edges.end(),
				[&e_v, &t, i](mesh_edge_ptr e) {
					return 	e->get_prev_edge()->get_end_vertex()->get_point() == e_v &&
							e->get_prev_edge()->get_vertex()->get_point() == t[i + 1];
				}
			);

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
	for (auto & triangle_edge : triangle_edges)
		triangle_edge->set_facet(f);

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
	std::for_each(triangles.begin(), triangles.end(), std::bind(&triangle_mesh::_add_triangle, this, _1));
	m_vertex_edge_map.clear();	// don't need this no mo

	// DEBUG

}

const maths::bbox3d& triangle_mesh::bbox() const
{
	if (m_bbox.is_empty())
	{
		maths::bbox3d mesh_bbox;

		std::vector<maths::vector3d> vertex_points(m_verts.size());
		std::transform(m_verts.begin(), m_verts.end(), vertex_points.begin(), std::mem_fn(&mesh_vertex::get_point));
		mesh_bbox.add_points(vertex_points.begin(), vertex_points.end());

		m_bbox = mesh_bbox;
	}
	return m_bbox;
}

bool triangle_mesh::is_manifold() const
{
	return std::find_if(m_edges.begin(), m_edges.end(), std::mem_fn(&mesh_edge::is_lamina)) == m_edges.end();
}

vector<mesh_edge_ptr> triangle_mesh::get_lamina_edges() const
{
	vector<mesh_edge_ptr> lamina_edges;
	std::copy_if(m_edges.begin(), m_edges.end(), std::back_inserter(lamina_edges), std::mem_fn(&mesh_edge::is_lamina));

	return lamina_edges;
}

triangle_mesh::vbo_data_t triangle_mesh::get_vbo_data() const
{
	vbo_data_t vbo_data;

	// First, build the index list
	// Maybe there is a more efficient way to do this?
	typedef std::map<maths::vector3d, unsigned int, compare_points> vertex_index_map_t;
	vertex_index_map_t vertex_index_map;

	const std::vector<mesh_vertex_ptr>& mesh_verts = get_vertices();
	const std::vector<mesh_facet_ptr>& mesh_facets = get_facets();
	for (mesh_facet_ptr facet : mesh_facets)
	{
		std::vector<mesh_vertex_ptr> facet_verts = facet->get_verts();

		for (size_t i = 0 ; i < 3 ; i++)
		{
			mesh_vertex_ptr vi = facet_verts[i];
			vertex_index_map_t::iterator vert_index = vertex_index_map.find(vi->get_point());
			if (vert_index == vertex_index_map.end())
			{
				const maths::vector3d& vi_pt = vi->get_point();
				auto matching_vert = std::find_if(mesh_verts.begin(), mesh_verts.end(),
					[&vi_pt](const mesh_vertex_ptr & vp) { return vp->get_point() == vi_pt; });

				if (matching_vert == mesh_verts.end())
					throw std::runtime_error("Couldn't find matching vert!");

				// Constant-time complexity for random access iterator
				const unsigned int idx = (unsigned int) std::distance(mesh_verts.begin(), matching_vert);
				std::tie(vert_index, std::ignore) = vertex_index_map.insert(std::make_pair(vi_pt, idx));
			}

			vbo_data.indices.push_back(vert_index->second);
		}
	}

	// Next, add the normals and vertices
	for (mesh_vertex_ptr mesh_vert : mesh_verts)
	{
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

double triangle_mesh::volume() const
{
	// Maybe cache this
	// Compute signed volume of each triangle in the mesh
	double volume = std::accumulate(m_facets.begin(), m_facets.end(), 0.0,
		[](double vol, const mesh_facet_ptr & f)
		{
			maths::triangle3d t = f->get_triangle();
			vol += t.signed_volume();

			return vol;
		}
	);

	return std::abs(volume);
}

double triangle_mesh::area() const
{
	// This should probably be cached, too.
	double area = std::accumulate(m_facets.begin(), m_facets.end(), 0.0,
		[](double a, const mesh_facet_ptr & f)
		{
			maths::triangle3d t = f->get_triangle();
			return a += t.area();
		}
	);

	return area;
}

ostream& operator<<(ostream& os, const triangle_mesh& mesh)
{
	const vector<mesh_facet_ptr>& facets = mesh.get_facets();

	os << "solid" << std::endl;	// TODO - add mesh name

	for (mesh_facet_ptr f : facets)
	{
		const maths::triangle3d t = f->get_triangle();
		const maths::vector3d n = f->get_normal();

		os << "facet normal ";
		os << setprecision(6) << n.x() << " " << n.y() << " " << n.z() << std::endl;

		os << "\t" << "outer loop" << std::endl;
		for (int i = 0 ; i < 3 ; i++)
		{
			const maths::vector3d& p = t[i];
			os << "\t\t" << "vertex " << "\t" << setprecision(6) << p.x() << " " << p.y() << " " << p.z() << std::endl;
		}
		os << "\t" << "endloop" << std::endl;
		os << "endfacet" << std::endl;
	}

	os << "endsolid";

	return os;
}
