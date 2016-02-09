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
class mesh_edge;
class mesh_facet;

typedef std::weak_ptr<mesh_vertex> 	mesh_vertex_weak;
typedef std::weak_ptr<mesh_edge>	mesh_edge_weak;
typedef std::weak_ptr<mesh_facet>	mesh_facet_weak;

typedef std::shared_ptr<mesh_vertex>	mesh_vertex_ptr;
typedef std::shared_ptr<mesh_edge>		mesh_edge_ptr;
typedef std::shared_ptr<mesh_facet>		mesh_facet_ptr;

/** An edge in the mesh.
 *  Most of the mesh topology is encoded in this data structure.
 */
class mesh_edge
{
private:
	mesh_vertex_weak	m_vert;
	mesh_facet_weak		m_facet;
	mesh_edge_weak		m_next_edge;	// CCW order
	mesh_edge_weak		m_prev_edge;	// CCW order
	mesh_edge_weak		m_symmetric_edge;

public:
	mesh_edge()
	{

	}

	bool operator==(const mesh_edge& e) const;

	void set_vertex(const mesh_vertex_ptr& v) { m_vert = v; }
	mesh_vertex_ptr get_vertex() const { return m_vert.lock(); }

	void set_facet(const mesh_facet_ptr& f) { m_facet = f; }
	mesh_facet_ptr get_facet() const { return m_facet.lock(); }

	void set_prev_edge(const mesh_edge_ptr& e) { m_prev_edge = e; }
	mesh_edge_ptr get_prev_edge() const { return m_prev_edge.lock(); }

	void set_next_edge(const mesh_edge_ptr& e) { m_next_edge = e; }
	mesh_edge_ptr get_next_edge() const { return m_next_edge.lock(); }

	void set_sym_edge(const mesh_edge_ptr& e) { m_symmetric_edge = e; }
	mesh_edge_ptr get_sym_edge() const { return m_symmetric_edge.lock(); }

	void set(const mesh_vertex_ptr& v,
			 const mesh_facet_ptr& f,
			 const mesh_edge_ptr& e_prev,
			 const mesh_edge_ptr& e_next,
			 const mesh_edge_ptr& e_sym)
	{
		m_vert = v;
		m_facet = f;
		m_prev_edge = e_prev;
		m_next_edge = e_next;
		m_symmetric_edge = e_sym;
	}

	std::vector<mesh_facet_ptr>	get_adjacent_facets() const;
	std::vector<mesh_vertex_ptr> get_verts() const;

	mesh_vertex_ptr get_start_vertex() const { return get_vertex(); }
	mesh_vertex_ptr get_end_vertex() const;
	maths::vector3d get_start_point() const;
	maths::vector3d get_end_point() const;

	bool is_lamina() const { return m_symmetric_edge.expired(); }

	friend std::ostream& operator<<(std::ostream& os, const mesh_edge& edge);

	// I guess there should be a facet_iterator...
};

class mesh_facet
{
private:
	mesh_edge_weak	m_edge;	// Any (?) edge on this facet
	maths::vector3d	m_normal;

public:
	mesh_facet(const maths::vector3d& normal)
	: m_normal(normal) { }

	void set_edge(const mesh_edge_ptr& edge) { m_edge = edge; }

	const maths::vector3d& get_normal() const { return m_normal; }
	maths::triangle3d get_triangle() const;

	std::vector<mesh_edge_ptr>		get_edges() const;
	std::vector<mesh_vertex_ptr>	get_verts() const;
	std::vector<mesh_facet_ptr>		get_adjacent_facets() const;

	friend std::ostream& operator<<(std::ostream& os, const mesh_facet& facet);
};

class mesh_vertex
{
private:
	mesh_edge_weak	m_edge;
	maths::vector3d	m_point;

public:
	mesh_vertex(const maths::vector3d& point)
	: m_point(point) { }

	void set_edge(const mesh_edge_ptr& edge)  { m_edge = edge; }

	std::vector<mesh_edge_ptr>	get_adjacent_edges() const;
	std::vector<mesh_facet_ptr>	get_adjacent_facets() const;

	// maybe we should store the normal, but that's a little awkward,
	// since we can only compute it when we have all of the neighboring
	// facets to this vertex populated in the mesh...
	maths::vector3d get_normal() const;
	const maths::vector3d& get_point() const { return m_point; }

	// edge_iterator, vertex_iterator

	friend std::ostream& operator<<(std::ostream& os, const mesh_vertex& vertex);
};

/// The main triangle mesh class
class triangle_mesh
{
private:
	std::vector<mesh_edge_ptr>		m_edges;
	std::vector<mesh_facet_ptr>		m_facets;
	std::vector<mesh_vertex_ptr>	m_verts;

	mutable maths::bbox3d			m_bbox;	// cached for speed

	/** Adds unique vertices, edges and facets to m_edges, m_facets, and m_verts
	 *  from the given triangle. */
	void	_add_triangle(const maths::triangle3d& t);

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

	/** Like compare_points, but compares the start and end vertices of two mesh_edge pointers.
	 * Basically, sorts line segments. */
	struct compare_edges
	{
		bool operator()(const mesh_edge_ptr e1, const mesh_edge_ptr e2) const
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
	// Associates a point to the set of all edges that have this vector as their starting point
	typedef std::map<maths::vector3d, std::vector<mesh_edge_ptr>, compare_points> vertex_edge_map_t;
	vertex_edge_map_t m_vertex_edge_map;

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
	 *  This will return true if each edge in this mesh exactly matches each edge in the other mesh. */
	bool operator==(const triangle_mesh& other) const;
	bool operator!=(const triangle_mesh& other) const { return !(*this == other); }

	/** Resets the mesh */
	void reset();

	/** Is the mesh empty? */
	bool is_empty() const;

	/** Builds a new mesh from the given set of triangles */
	void build(const std::vector<maths::triangle3d>& triangles);

	const std::vector<mesh_edge_ptr>& get_edges() const { return m_edges; }
	const std::vector<mesh_facet_ptr>& get_facets() const { return m_facets; }
	const std::vector<mesh_vertex_ptr>& get_vertices() const { return m_verts; }

	const maths::bbox3d& bbox() const;

	typedef std::vector<mesh_edge_ptr>::const_iterator edge_iterator;
	typedef std::vector<mesh_facet_ptr>::const_iterator facet_iterator;
	typedef std::vector<mesh_vertex_ptr>::const_iterator vertex_iterator;

	const edge_iterator edges_begin() const { return m_edges.begin(); }
	const edge_iterator edges_end() const { return m_edges.end(); }
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

	/** Returns true if there are no lamina edges in the tessellation */
	bool is_manifold() const;

	std::vector<mesh_edge_ptr> get_lamina_edges() const;

	friend std::ostream& operator<<(std::ostream& os, const triangle_mesh& mesh);
};

/** Output mesh as an ASCII STL to the given stream.
 *  @remark	This should probably eventually be replaced with a
 *  		mesh serializer class, if only to control binary or ASCII output.
 */
std::ostream& operator<<(std::ostream& os, const triangle_mesh& mesh);

#endif /* TRIANGLE_MESH_H_ */
