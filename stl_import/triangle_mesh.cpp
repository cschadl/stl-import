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
#include <numeric>

using std::vector;
using std::ostream;
using std::setprecision;
using namespace std::placeholders;

///////////////////////////
// mesh_halfedge

bool mesh_halfedge::operator==(const mesh_halfedge& e) const
{
	if (e.get_start_point() != this->get_start_point())
		return false;

	if (e.get_end_point() != this->get_end_point())
		return false;

	if (e.is_lamina() != this->is_lamina())
		return false;

	return true;
}

vector<mesh_facet_ptr> mesh_halfedge::get_adjacent_facets() const
{
	vector<mesh_facet_ptr> facets;

	mesh_facet_ptr facet = get_facet();
	if (!facet)
	{
		facets.push_back(facet);

		mesh_halfedge_ptr sym_halfedge = get_sym_halfedge();
		if (sym_halfedge)
		{
			facets.push_back(sym_halfedge->get_facet());
		}
	}

	return facets;
}

vector<mesh_vertex_ptr> mesh_halfedge::get_verts() const
{
	vector<mesh_vertex_ptr> verts;
	if (!m_vert.expired())
	{
		verts.push_back(get_vertex());
		verts.push_back(get_end_vertex());
	}

	return verts;
}

mesh_vertex_ptr mesh_halfedge::get_end_vertex() const
{
	// We could use the symmetric halfedge to get the end vertex,
	// but, the halfedge could be lamina, and we use this when
	// we build the mesh in triangle_mesh::build()
	return get_next_halfedge()->get_vertex();
}

maths::vector3d mesh_halfedge::get_start_point() const
{
	return get_start_vertex()->get_point();
}

maths::vector3d mesh_halfedge::get_end_point() const
{
	return get_end_vertex()->get_point();
}

ostream& operator<<(ostream& os, const mesh_halfedge& halfedge)
{
	os << *(halfedge.get_vertex()) << " => " << *(halfedge.get_end_vertex());
	return os;
}

///////////////////////
// mesh_edge

mesh_edge::mesh_edge(const mesh_halfedge_ptr & he, const mesh_halfedge_ptr & he_sym)
: m_he(he)
, m_he_sym(he_sym)
{

}

maths::vector3d mesh_edge::p0() const { return get_halfedge()->get_start_point(); }

maths::vector3d mesh_edge::p1() const { return get_halfedge()->get_end_point(); }

///////////////////////
// mesh_facet

maths::triangle3d mesh_facet::get_triangle() const
{
	vector<mesh_vertex_ptr> verts = get_verts();
	maths::triangle3d triangle(verts[0]->get_point(), verts[1]->get_point(), verts[2]->get_point());

	return triangle;
}

vector<mesh_halfedge_ptr> mesh_facet::get_halfedges() const
{
	vector<mesh_halfedge_ptr> halfedges;

	mesh_halfedge_ptr start_halfedge = m_halfedge.lock();
	mesh_halfedge_ptr e = start_halfedge;
	do
	{
		halfedges.push_back(e);
		e = e->get_next_halfedge();
	}
	while (e != start_halfedge);

	return halfedges;
}

vector<mesh_vertex_ptr> mesh_facet::get_verts() const
{
	vector<mesh_vertex_ptr> verts;

	mesh_halfedge_ptr start_halfedge = m_halfedge.lock();
	mesh_halfedge_ptr e = start_halfedge;
	do
	{
		verts.push_back(e->get_vertex());
		e = e->get_next_halfedge();
	}
	while (e != start_halfedge);

	return verts;
}

vector<mesh_facet_ptr> mesh_facet::get_adjacent_facets() const
{
	vector<mesh_facet_ptr> facets;

	mesh_halfedge_ptr start_halfedge = m_halfedge.lock();
	mesh_halfedge_ptr e = start_halfedge;
	do
	{
		facets.push_back(e->get_sym_halfedge()->get_facet());
		// make sure facets.back() != this...
		e = e->get_next_halfedge();
	}
	while (e != start_halfedge);

	return facets;
}

ostream& operator<<(ostream& os, const mesh_facet& facet)
{
	const std::vector<mesh_halfedge_ptr> halfedges = facet.get_halfedges();
	for (auto ei = halfedges.begin() ; ei != halfedges.end() ; ++ei)
	{
		os << (ei != halfedges.begin() ? " --> " : "") << *ei << std::endl;
	}

	// Also print normal and adjacent facets?

	return os;
}

//////////////////////////
// mesh_vertex

vector<mesh_halfedge_ptr> mesh_vertex::get_adjacent_halfedges() const
{
	vector<mesh_halfedge_ptr> halfedges;

	mesh_halfedge_ptr start_halfedge = m_halfedge.lock();
	mesh_halfedge_ptr e = start_halfedge;
	do
	{
		halfedges.push_back(e);
		e = e->get_prev_halfedge()->get_sym_halfedge();
		if (!e)
			break;	// TODO - case when halfedge is lamina
	}
	while (e != start_halfedge);

	return halfedges;
}

vector<mesh_facet_ptr> mesh_vertex::get_adjacent_facets() const
{
	vector<mesh_facet_ptr> facets;

	mesh_halfedge_ptr start_halfedge = m_halfedge.lock();
	mesh_halfedge_ptr e = start_halfedge;
	do
	{
		if (std::find(facets.begin(), facets.end(), e->get_facet()) != facets.end())
			break;

		facets.push_back(e->get_facet());
		e = e->get_prev_halfedge()->get_sym_halfedge();
		if (!e)
			break;
	}
	while(e != start_halfedge);

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

maths::vector3d& mesh_vertex::set_point(const maths::vector3d& point)
{
	m_point = point;
	return m_point;
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
	vector<mesh_halfedge_ptr> this_halfedges = this->get_halfedges();
	vector<mesh_halfedge_ptr> other_halfedges = other.get_halfedges();

	// Use some quick and easy cases to early-out
	if (this_halfedges.size() != other_halfedges.size())
		return false;

	if (this->is_manifold() != other.is_manifold())
		return false;

	// Sort each halfedge based on its start and end points
	std::sort(this_halfedges.begin(), this_halfedges.end(), compare_halfedges());
	std::sort(other_halfedges.begin(), other_halfedges.end(), compare_halfedges());

	return std::equal(this_halfedges.begin(), this_halfedges.end(), other_halfedges.begin(),
		[](mesh_halfedge_ptr e1, mesh_halfedge_ptr e2) { return *e1 == *e2; });
}

void triangle_mesh::reset()
{
	m_bbox = maths::bbox3d();

	m_halfedges.clear();
	m_verts.clear();
	m_facets.clear();
}

bool triangle_mesh::is_empty() const
{
	return m_halfedges.empty();
}

void triangle_mesh::add_triangle(const maths::triangle3d& t)
{
	vector<mesh_halfedge_ptr> triangle_halfedges;

	// First, connect the halfedges of the triangle
	for (int i = 0 ; i < 3 ; i++)
	{
		mesh_halfedge_ptr e(new mesh_halfedge);
		triangle_halfedges.push_back(e);
	}

	for (int i = 0 ; i < 3 ; i++)
	{
		triangle_halfedges[i]->set_next_halfedge(triangle_halfedges[(i + 1) % 3]);
		triangle_halfedges[i]->set_prev_halfedge(i == 0 ? triangle_halfedges[2] : triangle_halfedges[i - 1]);
	}

	// Set the vertices of this triangle
	for (int i = 0 ; i < 3 ; i++)
	{
		mesh_halfedge_ptr e = triangle_halfedges[i];
		const maths::vector3d& e_v = t[i];

		vertex_halfedge_map_t::iterator ve = m_vertex_halfedge_map.find(e_v);
		if (ve == m_vertex_halfedge_map.end())
		{
			mesh_vertex_ptr halfedge_start_vert(new mesh_vertex(e_v));
			e->set_vertex(halfedge_start_vert);
			halfedge_start_vert->set_halfedge(e);

			// Insert this halfedge / vertex pair into our map
			std::vector<mesh_halfedge_ptr> vertex_halfedges;
			vertex_halfedges.push_back(e);
			m_vertex_halfedge_map.insert(std::make_pair(e_v, vertex_halfedges));

			// Insert the vertex to the global list of vertices
			m_verts.push_back(halfedge_start_vert);
		}
		else
		{
			std::vector<mesh_halfedge_ptr>& vertex_halfedges = ve->second;
			if (vertex_halfedges.empty())
				throw std::runtime_error("Empty halfedge / vertices association");

			mesh_vertex_ptr v = (*vertex_halfedges.begin())->get_vertex();
			if (std::find_if(vertex_halfedges.begin() + 1, vertex_halfedges.end(),
				[&v](const mesh_halfedge_ptr & e) { return e->get_vertex() != v; }) != vertex_halfedges.end())
			{
				throw std::runtime_error("bad halfedge vertex");
			}

			if (!v)
				throw std::runtime_error("got NULL vertex!");

			e->set_vertex(v);

			auto p_sym_halfedge = std::find_if(vertex_halfedges.begin(), vertex_halfedges.end(),
				[&e_v, &t, i](mesh_halfedge_ptr e) {
					auto pe = e ? e->get_prev_halfedge() : nullptr;
					if (!pe)
						return false;

					auto pe_ev = pe->get_end_vertex();
					auto pe_v = pe->get_vertex();

					if (!pe_ev || !pe_v)
						return false;

					return pe_ev->get_point() == e_v && pe_v->get_point() == t[i + 1];
				}
			);

			if (p_sym_halfedge != vertex_halfedges.end())
			{
				mesh_halfedge_ptr e_sym = (*p_sym_halfedge)->get_prev_halfedge();

				if (e_sym->get_end_vertex()->get_point() != e_v)
					throw std::runtime_error("symmetric halfedge point mismatch");

				e->set_sym_halfedge(e_sym);
				e_sym->set_sym_halfedge(e);

				m_edges.emplace_back(std::make_shared<mesh_edge>(e, e_sym));
			}

			vertex_halfedges.push_back(e);
		}
	}

	// Set the facet of this triangle, and set the start halfedge of the facet
	mesh_facet_ptr f(new mesh_facet(t.normal()));
	for (auto & triangle_halfedge : triangle_halfedges)
		triangle_halfedge->set_facet(f);

	f->set_halfedge(triangle_halfedges.back());
	m_facets.push_back(f);

	// Add the triangle halfedges to the list of halfedges
	std::copy(triangle_halfedges.begin(), triangle_halfedges.end(), std::back_inserter(m_halfedges));
}

void triangle_mesh::build(const vector<maths::triangle3d>& triangles)
{
	if (!is_empty())
		reset();

	m_vertex_halfedge_map.clear();
	std::for_each(triangles.begin(), triangles.end(), std::bind(&triangle_mesh::add_triangle, this, _1));
	m_vertex_halfedge_map.clear();	// don't need this no mo
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
	return std::find_if(m_halfedges.begin(), m_halfedges.end(), std::mem_fn(&mesh_halfedge::is_lamina)) == m_halfedges.end();
}

vector<mesh_halfedge_ptr> triangle_mesh::get_lamina_halfedges() const
{
	vector<mesh_halfedge_ptr> lamina_halfedges;
	std::copy_if(m_halfedges.begin(), m_halfedges.end(), std::back_inserter(lamina_halfedges), std::mem_fn(&mesh_halfedge::is_lamina));

	return lamina_halfedges;
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

void triangle_mesh::center()
{
	const auto& vertices = get_vertices();
	auto centroid = maths::centroid(vertices.begin(), vertices.end(), std::mem_fn(&mesh_vertex::get_point));

	for (const auto& vert : vertices)
	{
		vert->set_point(vert->get_point() - centroid);
	}
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
