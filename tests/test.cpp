/*
 * test.cpp
 *
 *  Created on: Mar 13, 2013
 *      Author: cds
 */

#include "stl_import.h"	// deprecated
#include "stl_importer.h"
#include "triangle_mesh.h"
#include "vectors.h"
#include "misc.h"
#include "geom.h"

#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/param.h>
#include <math.h>

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include <tut.h>

using std::istringstream;
using std::ostringstream;
using std::string;
using std::vector;

using maths::vector3d;
using maths::triangle3d;
using maths::bbox3d;

namespace tut
{
	struct test_stl_import_data
	{
		const std::string _test_data_path;

		test_stl_import_data()
		{
			// Get path of TUT test data.
			// We'll assume that it's one directory up in test_data/
			// Note - this isn't portable at all
			char tmp[32];
			char path_buf[MAXPATHLEN];

			::sprintf(tmp, "/proc/%d/exe", ::getpid());
			const int bytes = std::min(::readlink(tmp, path_buf, MAXPATHLEN), (ssize_t)(MAXPATHLEN - 1));
			if (bytes < 0)
				throw std::runtime_error("Couldn't readlink() path");	// shouldn't happen...

			const std::string cur_path = ::dirname(path_buf);

			char cur_path_buf[MAXPATHLEN];	// because dirname() doesn't take a const char*...
			std::size_t cur_path_len = cur_path.copy(cur_path_buf, cur_path.length());
			cur_path_buf[cur_path_len] = '\0';

			const std::string base_path = ::dirname(cur_path_buf);

			const_cast<std::string&>(_test_data_path) = base_path + "/test_data";
		}

		const std::string& test_data_path() const { return _test_data_path; }

		static string get_simple_stl_str()
		{
			// STL that defines a single facet
			ostringstream stl_ss;
			stl_ss << "solid test" << std::endl;
			stl_ss << "facet normal 0.0 0.0 1.0" << std::endl;
			stl_ss << "outer loop" << std::endl;
			stl_ss << "vertex 0.0 0.0 1.0" << std::endl;
			stl_ss << "vertex 1.0 1.0 1.0" << std::endl;
			stl_ss << "vertex 0.0 1.0 1.0" << std::endl;
			stl_ss << "endloop" << std::endl;
			stl_ss << "endfacet" << std::endl;
			stl_ss << "endsolid" << std::endl;

			return stl_ss.str();
		}

		static string get_tetrahedron_stl_str()
		{
			ostringstream stl_ss;
			stl_ss << "solid test_tetrahedron" << std::endl;
			stl_ss << "	facet normal 0.0 -0.89442719 0.447213596" << std::endl;
			stl_ss << "		outer loop" << std::endl;
			stl_ss << "			vertex -0.5 -0.4330127 0.0" << std::endl;
			stl_ss << "			vertex  0.5 -0.4330127 0.0" << std::endl;	// Note the extra spaces
			stl_ss << "			vertex  0 0 0.86602540" << std::endl;
			stl_ss << "		endloop" << std::endl;
			stl_ss << "	endfacet" << std::endl;
			stl_ss << "	facet normal -0.84016805 0.48507125 0.24253563" << std::endl;
			stl_ss << "		outer loop" << std::endl;
			stl_ss << "			vertex -0.5 -0.4330127 0.0" << std::endl;
			stl_ss << "			vertex  0 0 0.86602540" << std::endl;
			stl_ss << "			vertex  0 0.4330127 0.0" << std::endl;
			stl_ss << "		endloop" << std::endl;
			stl_ss << "	endfacet" << std::endl;
			stl_ss << "	facet normal 0.84016805 0.48507125 0.24253563" << std::endl;
			stl_ss << "		outer loop" << std::endl;
			stl_ss << "			vertex 0.0 0.4330127 0.0" << std::endl;
			stl_ss << "			vertex 0 0 0.86602540" << std::endl;
			stl_ss << "			vertex 0.5 -0.4330127 0.0" << std::endl;
			stl_ss << "		endloop" << std::endl;
			stl_ss << "	endfacet" << std::endl;
			stl_ss << "	facet normal 0 0 -1" << std::endl;
			stl_ss << "		outer loop" << std::endl;
			stl_ss << "			vertex  0.5 -0.4330127 0.0" << std::endl;
			stl_ss << "			vertex -0.5 -0.4330127 0.0" << std::endl;
			stl_ss << "			vertex  0    0.4330127 0" << std::endl;
			stl_ss << "		endloop" << std::endl;
			stl_ss << "	endfacet" << std::endl;
			stl_ss << "endsolid" << std::endl;

			return stl_ss.str();
		}
	};

	typedef test_group<test_stl_import_data> stl_test_group_t;
	stl_test_group_t stl_test_group("deprecated tests");

	template<> template<>
	void stl_test_group_t::object::test<1>()
	{
		set_test_name("Read single facet");

		string stl_str = get_simple_stl_str();
		std::istringstream stl_is(stl_str);

		stl_import importer(stl_is);
		ensure(importer.get_name() == "test");

		const vector<triangle3d>& facets = importer.get_facets();
		ensure(facets.size() == 1);

		const triangle3d& facet = *facets.begin();
		const double tol = std::numeric_limits<double>::epsilon();
		ensure(maths::close(facet.normal().distance_sq(vector3d(0.0, 0.0, 1.0)), 0.0, tol * tol));
	}

	template<> template<>
	void stl_test_group_t::object::test<2>()
	{
		set_test_name("Read tetrahedron");

		string stl_str = get_tetrahedron_stl_str();
		std::istringstream tet_is(stl_str);

		stl_import importer(tet_is);
		ensure(importer.get_name() == "test_tetrahedron");

		const vector<triangle3d>& facets = importer.get_facets();
		ensure(facets.size() == 4);

		vector<triangle3d>::const_iterator f = facets.begin();

		const double tol = 1.0e-8;	// only 8 digits in our vectors

		const vector3d n1(0.0, -0.89442719, 0.447213596);
		const vector3d n2(-0.84016805, 0.48507125, 0.24253563);
		const vector3d n3(0.84016805, 0.48507125, 0.24253563);
		const vector3d n4(0, 0, -1);

		ensure(maths::close(n1.distance_sq(f->normal()), 0.0, tol * tol));	++f;
		ensure(maths::close(n2.distance_sq(f->normal()), 0.0, tol * tol));	++f;
		ensure(maths::close(n3.distance_sq(f->normal()), 0.0, tol * tol));	++f;
		ensure(maths::close(n4.distance_sq(f->normal()), 0.0, tol * tol));	++f;
		ensure(f == facets.end());
	}

	template<> template<>
	void stl_test_group_t::object::test<3>()
	{
		set_test_name("Triangle mesh - tetrahedron");

		const double tol = 1.0e-8;

		string stl_str = get_tetrahedron_stl_str();
		std::istringstream tet_is(stl_str);

		stl_import importer(tet_is);

		triangle_mesh mesh;
		mesh.build(importer.get_facets());

		ensure(mesh.get_halfedges().size() == 12);
		ensure(mesh.get_vertices().size() == 4);
		ensure(mesh.get_facets().size() == 4);
		ensure(mesh.is_manifold());
		ensure(mesh.get_lamina_halfedges().empty());

		// Facets should be in the order that they were read...
		const vector<mesh_facet_ptr>& facets = mesh.get_facets();
		ensure(facets[0]->get_normal().is_close(vector3d(0, -0.89442719, 0.447213596), tol));
		ensure(facets[1]->get_normal().is_close(vector3d(-0.84016805, 0.48507125, 0.24253563), tol));
		ensure(facets[2]->get_normal().is_close(vector3d(0.84016805, 0.48507125, 0.24253563), tol));
		ensure(facets[3]->get_normal().is_close(vector3d(0, 0, -1), tol));

		vector<mesh_facet_ptr>::const_iterator pf = facets.begin();
		for ( ; pf != facets.end() ; ++pf)
		{
			mesh_facet_ptr f = *pf;
			vector<mesh_facet_ptr> adj_facets_expected = mesh.get_facets();
			adj_facets_expected.erase(std::find(adj_facets_expected.begin(), adj_facets_expected.end(), f));
			vector<mesh_facet_ptr> adj_facets = f->get_adjacent_facets();

			// I think the call to erase() might mess up the order...
			std::sort(adj_facets_expected.begin(), adj_facets_expected.end());
			std::sort(adj_facets.begin(), adj_facets.end());

			// debugging
//			std::copy(adj_facets_expected.begin(), adj_facets_expected.end(), std::ostream_iterator<mesh_facet_ptr>(std::cout, ", "));
//			std::cout << std::endl;
//			std::copy(adj_facets.begin(), adj_facets.end(), std::ostream_iterator<mesh_facet_ptr>(std::cout, ", "));
//			std::cout << std::endl;

			ensure(adj_facets == adj_facets_expected);
		}
	}

	template<> template<>
	void stl_test_group_t::object::test<4>()
	{
		set_test_name("Triangle mesh - sphere");

		std::ifstream sphere_infile;
		sphere_infile.open((test_data_path() + "/sphere.stl").c_str(), std::ifstream::in);

		ensure(sphere_infile.is_open());
		ensure(sphere_infile.good());

		stl_import importer(sphere_infile);

		triangle_mesh mesh;
		mesh.build(importer.get_facets());

		// Euler's formula |V| + |F| - |E|/2 = 2 should hold
		// for sphere (or, maybe all convex shapes?)
		ensure(mesh.get_halfedges().size() == 864);
		ensure(mesh.get_vertices().size() == 146);
		ensure(mesh.get_facets().size() == 288);

		ensure(mesh.is_manifold());
	}

	template <> template<>
	void stl_test_group_t::object::test<5>()
	{
		set_test_name("Triangle mesh - AR-15 lower receiver");

		std::ifstream ar15_infile;	// note that this is a CRLF txt file
		const std::string file_path = test_data_path() + "/lowerreceiver.stl";
		ar15_infile.open(file_path.c_str());

		ensure(ar15_infile.is_open());
		ensure(ar15_infile.good());

		stl_import importer(ar15_infile);

		triangle_mesh mesh;
		mesh.build(importer.get_facets());

		ensure(mesh.get_facets().size() == 36288);
		ensure(mesh.is_manifold());

		const maths::bbox3d mesh_bbox = mesh.bbox();
		ensure(!mesh_bbox.is_empty());
	}

	template <> template <>
	void stl_test_group_t::object::test<6>()
	{
		set_test_name("Triangle mesh - null facet normal");

		std::ifstream dna_infile;
		const std::string file_path = test_data_path() + "/DNA_L.stl";
		dna_infile.open(file_path.c_str());

		ensure(dna_infile.is_open());
		ensure(dna_infile.good());

		stl_import importer(dna_infile);

		triangle_mesh mesh;
		mesh.build(importer.get_facets());

		ensure(mesh.is_manifold());
	}

	template <> template <>
	void stl_test_group_t::object::test<7>()
	{
		set_test_name("STL import - long solid name");

		std::ifstream humanoid_infile;
		const std::string file_path = test_data_path() + "/humanoid.stl";
		humanoid_infile.open(file_path.c_str());

		ensure(humanoid_infile.is_open());
		ensure(humanoid_infile.good());

		stl_import importer(humanoid_infile);
		ensure(!importer.get_facets().empty());
	}

	template <> template <>
	void stl_test_group_t::object::test<8>()
	{
		set_test_name("Triangle mesh - non-manifold");

		std::ifstream bottle_infile;
		const std::string file_path = test_data_path() + "/bottle.stl";
		bottle_infile.open(file_path.c_str(), std::fstream::binary);

		ensure(bottle_infile.is_open());
		ensure(bottle_infile.good());

		stl_import importer(bottle_infile);
		triangle_mesh mesh;
		mesh.build(importer.get_facets());

		ensure(!mesh.is_empty());
		ensure(!mesh.is_manifold());

		// Even though the mesh is not manifold, we should be able to iterate
		// over all vertices in the mesh and retrieve a non-null vertex normal.
		const std::vector<mesh_vertex_ptr> vertices = mesh.get_vertices();
		for (std::vector<mesh_vertex_ptr>::const_iterator vi = vertices.begin() ; vi != vertices.end() ; ++vi)
		{
			mesh_vertex_ptr v = *vi;
			ensure(!!v);
			ensure(!v->get_normal().is_null());
		}
	}

	template <> template <>
	void stl_test_group_t::object::test<9>()
	{
		set_test_name("Triangle mesh - VBO");

		triangle_mesh mesh;
		{
			std::ifstream sphere_infile;
			sphere_infile.open((test_data_path() + "/sphere.stl").c_str(), std::ifstream::in);
			stl_import importer(sphere_infile);

			mesh.build(importer.get_facets());
		}
		ensure(!mesh.is_empty());
		ensure(mesh.is_manifold());

		std::vector<maths::triangle3d> vbo_triangles;
		triangle_mesh::vbo_data_t vbo_data = mesh.get_vbo_data();

		size_t i = 0;
		const std::vector<unsigned int>& vis = vbo_data.indices;
		while (i < vbo_data.indices.size())
		{
			double * vbo_0 = vbo_data.verts[vis[i]];
			double * vbo_1 = vbo_data.verts[vis[i + 1]];
			double * vbo_2 = vbo_data.verts[vis[i + 2]];
			maths::vector3d v1(vbo_0[0], vbo_0[1], vbo_0[2]);
			maths::vector3d v2(vbo_1[0], vbo_1[1], vbo_1[2]);
			maths::vector3d v3(vbo_2[0], vbo_2[1], vbo_2[2]);

			vbo_triangles.push_back(maths::triangle3d(v1, v2, v3));

			i += 3;
		}
		std::random_shuffle(vbo_triangles.begin(), vbo_triangles.end());

		triangle_mesh sphere_rebuilt;
		sphere_rebuilt.build(vbo_triangles);

		ensure(mesh == sphere_rebuilt);
	}

	template <> template <>
	void stl_test_group_t::object::test<10>()
	{
		set_test_name("Copy constructor and assignment operator");

		triangle_mesh mesh1;
		{
			triangle_mesh mesh;

			std::ifstream dna_infile;
			dna_infile.open((test_data_path() + "/DNA_L.stl").c_str(), std::ifstream::in);
			stl_import importer(dna_infile);

			std::vector<maths::triangle3d> mesh1_facets = importer.get_facets();
			std::random_shuffle(mesh1_facets.begin(), mesh1_facets.end());

			mesh.build(mesh1_facets);

			mesh1 = mesh;
		}

		ensure(!mesh1.is_empty());

		triangle_mesh mesh2 = mesh1;
		ensure(mesh1 == mesh2);

		triangle_mesh mesh3;
		{
			// Just load some junk into mesh3.  Doesn't matter what.
			string stl_str = get_tetrahedron_stl_str();
			std::istringstream tet_is(stl_str);

			stl_import importer(tet_is);
			mesh3.build(importer.get_facets());

			ensure(mesh3 != mesh1);
		}

		mesh3 = mesh1;

		ensure(mesh1 == mesh3);
		ensure(mesh3 == mesh2);
	}

	template<> template<>
	void stl_test_group_t::object::test<11>()
	{
		set_test_name("Output");

		triangle_mesh mesh1;
		{
			// Use DNA_L.stl again, since we know that the copy constructor
			// works, assuming that the previous test passed.
			std::ifstream dna_infile;
			dna_infile.open((test_data_path() + "/DNA_L.stl").c_str(), std::ifstream::in);
			stl_import importer(dna_infile);

			mesh1.build(importer.get_facets());
		}
		ensure(!mesh1.is_empty());
		ensure(mesh1.is_manifold());

		std::ostringstream os;
		os << mesh1;

		std::istringstream is(os.str());

		triangle_mesh mesh2;
		{
			stl_import importer(is);
			mesh2.build(importer.get_facets());
		}

		// operator== doesn't quite work as intended, so we'll just have to make do for now
		ensure(!mesh2.is_empty());
		ensure(mesh2.is_manifold());

		ensure(mesh1.get_halfedges().size() == mesh2.get_halfedges().size());
		ensure(mesh1.get_facets().size() == mesh2.get_facets().size());
		ensure(mesh1.get_vertices().size() == mesh2.get_vertices().size());
	}

	template<> template<>
	void stl_test_group_t::object::test<12>()
	{
		set_test_name("Sphere area and volume");

		std::ifstream sphere_infile;
		sphere_infile.open((test_data_path() + "/unit_sphere-ascii.stl").c_str(), std::ifstream::in);
		stl_import importer(sphere_infile);

		triangle_mesh sphere_mesh;
		sphere_mesh.build(importer.get_facets());

		ensure(!sphere_mesh.is_empty());
		ensure(sphere_mesh.is_manifold());

		const double sphere_volume = sphere_mesh.volume();
		const double sphere_area = sphere_mesh.area();

		// This is just a tessellation of the sphere, so we'll have to use a fairly loose tolerance
		ensure_distance(sphere_area, 4 * M_PI, 0.03);
		ensure_distance(sphere_volume, 4.0 / 3.0 * M_PI, 0.03);
	}

	template<> template<>
	void stl_test_group_t::object::test<13>()
	{
		set_test_name("mesh_triangle_inserter");

		stl_util::stl_importer importer(test_data_path() + "/buddha.stl");

		triangle_mesh mesh;
		importer.import(mesh_triangle_inserter(mesh));

		ensure("no facets read", importer.num_facets_read() == mesh.get_facets().size());
		ensure("mesh not solid", mesh.is_manifold());
	}
};
