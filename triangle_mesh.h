/*
 * triangle_mesh.h
 *
 *  Created on: Apr 6, 2013
 *      Author: cds
 */

#ifndef TRIANGLE_MESH_H_
#define TRIANGLE_MESH_H_

#include <vector>
#include <map>
#include <memory>

#include "geom.h"

class mesh_vertex;
class mesh_halfedge;
class mesh_edge;
class mesh_facet;

typedef std::weak_ptr<mesh_vertex> 		mesh_vertex_weak;
typedef std::weak_ptr<mesh_halfedge>	mesh_halfedge_weak;
typedef std::weak_ptr<mesh_edge>		mesh_edge_weak;
typedef std::weak_ptr<mesh_facet>		mesh_facet_weak;

typedef std::shared_ptr<mesh_vertex>	mesh_vertex_ptr;
typedef std::shared_ptr<mesh_halfedge>	mesh_halfedge_ptr;
typedef std::shared_ptr<mesh_edge>		mesh_edge_ptr;
typedef std::shared_ptr<mesh_facet>		mesh_facet_ptr;

/** An halfedge in the mesh.
 *  Most of the mesh topology is encoded in this data structure.
 */
class mesh_halfedge
{
private:
	mesh_vertex_weak	m_vert;
	mesh_facet_weak		m_facet;
	mesh_halfedge_weak	m_next_halfedge;	// CCW order
	mesh_halfedge_weak	m_prev_halfedge;	// CCW order
	mesh_halfedge_weak	m_symmetric_halfedge;

public:
	mesh_halfedge()
	{

	}

	bool operator==(const mesh_halfedge& e) const;

	void set_vertex(const mesh_vertex_ptr& v) { m_vert = v; }
	mesh_vertex_ptr get_vertex() const { return m_vert.lock(); }

	void set_facet(const mesh_facet_ptr& f) { m_facet = f; }
	mesh_facet_ptr get_facet() const { return m_facet.lock(); }

	void set_prev_halfedge(const mesh_halfedge_ptr& e) { m_prev_halfedge = e; }
	mesh_halfedge_ptr get_prev_halfedge() const { return m_prev_halfedge.lock(); }

	void set_next_halfedge(const mesh_halfedge_ptr& e) { m_next_halfedge = e; }
	mesh_halfedge_ptr get_next_halfedge() const { return m_next_halfedge.lock(); }

	void set_sym_halfedge(const mesh_halfedge_ptr& e) { m_symmetric_halfedge = e; }
	mesh_halfedge_ptr get_sym_halfedge() const { return m_symmetric_halfedge.lock(); }

	void set(const mesh_vertex_ptr& v,
			 const mesh_facet_ptr& f,
			 const mesh_halfedge_ptr& e_prev,
			 const mesh_halfedge_ptr& e_next,
			 const mesh_halfedge_ptr& e_sym)
	{
		m_vert = v;
		m_facet = f;
		m_prev_halfedge = e_prev;
		m_next_halfedge = e_next;
		m_symmetric_halfedge = e_sym;
	}

	std::vector<mesh_facet_ptr>	get_adjacent_facets() const;
	std::vector<mesh_vertex_ptr> get_verts() const;

	mesh_vertex_ptr get_start_vertex() const { return get_vertex(); }
	mesh_vertex_ptr get_end_vertex() const;
	maths::vector3d get_start_point() const;
	maths::vector3d get_end_point() const;

	bool is_lamina() const { return m_symmetric_halfedge.expired(); }

	friend std::ostream& operator<<(std::ostream& os, const mesh_halfedge& halfedge);

	// I guess there should be a facet_iterator...
};

/** A mesh edge is just a mesh halfedge and its symmetric halfedge, in no particular order. */
class mesh_edge
{
private:
	mesh_halfedge_weak	m_he;
	mesh_halfedge_weak	m_he_sym;

public:
	mesh_edge(const mesh_halfedge_ptr & he, const mesh_halfedge_ptr & sym);

	mesh_halfedge_ptr get_halfedge() const { return m_he.lock(); }
	mesh_halfedge_ptr get_sym_halfedge() const { return m_he_sym.lock(); }

	maths::vector3d p0() const;
	maths::vector3d p1() const;
};

class mesh_facet
{
private:
	mesh_halfedge_weak	m_halfedge;	// Any (?) halfedge on this facet
	maths::vector3d	m_normal;

public:
	mesh_facet(const maths::vector3d& normal)
	: m_normal(normal) { }

	void set_halfedge(const mesh_halfedge_ptr& halfedge) { m_halfedge = halfedge; }

	const maths::vector3d& get_normal() const { return m_normal; }
	void set_normal(const maths::vector3d& normal) { m_normal = normal; }

	maths::triangle3d get_triangle() const;

	std::vector<mesh_halfedge_ptr>	get_halfedges() const;
	std::vector<mesh_vertex_ptr>	get_verts() const;
	std::vector<mesh_facet_ptr>		get_adjacent_facets() const;

	friend std::ostream& operator<<(std::ostream& os, const mesh_facet& facet);
};

class mesh_vertex : public std::enable_shared_from_this<mesh_vertex>
{
private:
	mesh_halfedge_weak	m_halfedge;
	maths::vector3d	m_point;

public:
	mesh_vertex(const maths::vector3d& point)
	: m_point(point) { }

	void set_halfedge(const mesh_halfedge_ptr& halfedge)  { m_halfedge = halfedge; }

	std::vector<mesh_halfedge_ptr>	get_adjacent_halfedges() const;
	std::vector<mesh_facet_ptr>	get_adjacent_facets() const;

	// maybe we should store the normal, but that's a little awkward,
	// since we can only compute it when we have all of the neighboring
	// facets to this vertex populated in the mesh...
	maths::vector3d get_normal() const;
	const maths::vector3d& get_point() const { return m_point; }
	maths::vector3d& set_point(const maths::vector3d& p);

	// halfedge_iterator, vertex_iterator

	friend std::ostream& operator<<(std::ostream& os, const mesh_vertex& vertex);
};

/// The main triangle mesh class
class triangle_mesh
{
private:
	std::vector<mesh_halfedge_ptr>	m_halfedges;
	std::vector<mesh_edge_ptr>		m_edges;	// only "complete" (e.g. non-lamina) edges
	std::vector<mesh_facet_ptr>		m_facets;
	std::vector<mesh_vertex_ptr>	m_verts;

	mutable maths::bbox3d			m_bbox;	// cached for speed

	std::string						m_name;

	// I'm a bit iffy on defining operator< for
	// maths::n_vector, so for now, we'll compare
	// points using this functor
	struct compare_points
	{
		bool operator()(const maths::vector3d& p1, const maths::vector3d& p2) const
		{
			if (p1.x() != p2.x())
				return p1.x() < p2.x();
			else if (p1.y() != p2.y())
				return p1.y() < p2.y();

			return p1.z() < p2.z();
		}
	};

	/** Like compare_points, but compares the start and end vertices of two mesh_halfedge pointers.
	 * Basically, sorts line segments. */
	struct compare_halfedges
	{
		bool operator()(const mesh_halfedge_ptr e1, const mesh_halfedge_ptr e2) const
		{
			const maths::vector3d e1_start = e1->get_start_point();
			const maths::vector3d e2_start = e2->get_start_point();
			const maths::vector3d e1_end = e1->get_end_point();
			const maths::vector3d e2_end = e2->get_end_point();

			if (e1_start.x() != e2_start.x())
				return e1_start.x() < e2_start.x();
			else if (e1_start.y() != e2_start.y())
				return e1_start.y() < e2_start.y();
			else if (e1_start.z() != e2_start.z())
				return e1_start.z() < e2_start.z();
			else if (e1_end.x() != e2_end.x())
				return e1_end.x() < e2_end.x();
			else if (e1_end.y() != e2_end.y())
				return e1_end.y() < e2_end.y();

			return e1_end.z() < e2_end.z();
		}
	};

	// Used when building the mesh from a set of triangles
	// Associates a point to the set of all halfedges that have this vector as their starting point
	typedef std::map<maths::vector3d, std::vector<mesh_halfedge_ptr>, compare_points> vertex_halfedge_map_t;
	vertex_halfedge_map_t m_vertex_halfedge_map;

public:
	/** Create an empty triangle mesh */
	triangle_mesh() { }

	/** Create a mesh from a bunch of triangles */
	triangle_mesh(const std::vector<maths::triangle3d>& triangles);

	/** @name Copying
	 *  Copy Constructors / assignment operators
	 *  @{ */
	triangle_mesh(const triangle_mesh& mesh) = default;
	triangle_mesh& operator=(const triangle_mesh& mesh) = default;
	/** @} */

	/** Destructor */
	~triangle_mesh() { }

	/** Test two meshes for equality.
	 *  This will return true if each halfedge in this mesh exactly matches each halfedge in the other mesh. */
	bool operator==(const triangle_mesh& other) const;
	bool operator!=(const triangle_mesh& other) const { return !(*this == other); }

	/** Resets the mesh */
	void reset();

	/** Is the mesh empty? */
	bool is_empty() const;

	/** Builds a new mesh from the given set of triangles */
	void build(const std::vector<maths::triangle3d>& triangles);

	/** Adds unique vertices, halfedges and facets to m_halfedges, m_facets, and m_verts
	 *  from the given triangle. */
	void	add_triangle(const maths::triangle3d& t);

	const std::vector<mesh_halfedge_ptr>& get_halfedges() const { return m_halfedges; }
	const std::vector<mesh_edge_ptr>& get_edges() const { return m_edges; }
	const std::vector<mesh_facet_ptr>& get_facets() const { return m_facets; }
	const std::vector<mesh_vertex_ptr>& get_vertices() const { return m_verts; }

	const maths::bbox3d& bbox() const;

	typedef std::vector<mesh_halfedge_ptr>::const_iterator halfedge_iterator;
	typedef std::vector<mesh_facet_ptr>::const_iterator facet_iterator;
	typedef std::vector<mesh_vertex_ptr>::const_iterator vertex_iterator;

	const halfedge_iterator halfedges_begin() const { return m_halfedges.begin(); }
	const halfedge_iterator halfedges_end() const { return m_halfedges.end(); }
	const vertex_iterator vertices_begin() const { return m_verts.begin(); }
	const vertex_iterator vertices_end() const { return m_verts.end(); }
	const facet_iterator facets_begin() const { return m_facets.begin(); }
	const facet_iterator facets_end() const { return m_facets.end(); }

	// Stuff for passing mesh to OpenGL VBOs
	struct vbo_data_t
	{
		std::vector<double*> 		verts;		/**< 3 doubles per vertex */
		std::vector<double*> 		normals;	/**< 3 doubles per normal */
		std::vector<unsigned int>	indices;	/**< flat array of vert/normal indices */

		~vbo_data_t()
		{
			for (size_t i = 0 ; i < verts.size() ; i++)
			{
				delete[] verts[i];
				delete[] normals[i];
			}
		}
	};

	vbo_data_t get_vbo_data() const;

	/** Returns true if there are no lamina halfedges in the tessellation */
	bool is_manifold() const;

	std::vector<mesh_halfedge_ptr> get_lamina_halfedges() const;

	// Properties
	double volume() const;	// unit-free
	double area() const;

	std::string& name() { return m_name; }
	const std::string& name() const { return m_name; }

	// Centers the mesh
	void center();

	friend std::ostream& operator<<(std::ostream& os, const triangle_mesh& mesh);
};

/** Output mesh as an ASCII STL to the given stream.
 *  @remark	This should probably eventually be replaced with a
 *  		mesh serializer class, if only to control binary or ASCII output.
 */
std::ostream& operator<<(std::ostream& os, const triangle_mesh& mesh);

/** Output iterator for inserting triangles into a mesh.
 */

class mesh_triangle_inserter : public std::iterator<std::output_iterator_tag, void, void, void, void>
{
private:
	triangle_mesh*	m_triangle_mesh;	// not owned

public:
	mesh_triangle_inserter() = delete;
	explicit mesh_triangle_inserter(triangle_mesh& mesh) : m_triangle_mesh(&mesh) { }

	mesh_triangle_inserter& operator*() { return *this; }
	mesh_triangle_inserter& operator++() { return *this; }
	mesh_triangle_inserter& operator++(int) { return *this; }

	mesh_triangle_inserter& operator=(const maths::triangle3d& t) { m_triangle_mesh->add_triangle(t); return *this; }
};

#endif /* TRIANGLE_MESH_H_ */
