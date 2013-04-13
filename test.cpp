/*
 * test.cpp
 *
 *  Created on: Mar 13, 2013
 *      Author: cds
 */

#include "stl_import.h"
#include "triangle_mesh.h"
#include "vectors.h"
#include "misc.h"
#include "geom.h"

#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <iterator>
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
	stl_test_group_t stl_test_group("STL importer tests");

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

		ensure(mesh.get_edges().size() == 12);
		ensure(mesh.get_vertices().size() == 4);
		ensure(mesh.get_facets().size() == 4);
		ensure(mesh.is_manifold());

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

			std::copy(adj_facets_expected.begin(), adj_facets_expected.end(), std::ostream_iterator<mesh_facet_ptr>(std::cout, ", "));
			std::cout << std::endl;
			std::copy(adj_facets.begin(), adj_facets.end(), std::ostream_iterator<mesh_facet_ptr>(std::cout, ", "));
			std::cout << std::endl;

			ensure(adj_facets == adj_facets_expected);
		}
	}

	template<> template<>
	void stl_test_group_t::object::test<4>()
	{
		set_test_name("Triangle mesh - sphere");

		std::ifstream sphere_infile;
		sphere_infile.open("/home/cds/Desktop/sphere.stl", std::ifstream::in);

		ensure(sphere_infile.is_open());
		ensure(sphere_infile.good());

		stl_import importer(sphere_infile);

		triangle_mesh mesh;
		mesh.build(importer.get_facets());

		// Euler's formula |V| + |F| - |E|/2 = 2 should hold
		// for sphere (or, maybe all convex shapes?)
		ensure(mesh.get_edges().size() == 864);
		ensure(mesh.get_vertices().size() == 146);
		ensure(mesh.get_facets().size() == 288);

		ensure(mesh.is_manifold());
	}

	template <> template<>
	void stl_test_group_t::object::test<5>()
	{
		set_test_name("Triangle mesh - AR-15 lower receiver");

		std::ifstream ar15_infile;	// note that this is a CRLF txt file
		ar15_infile.open("/home/cds/Downloads/ar15_magazine_and_reciever/lowerreceiver.stl");

		ensure(ar15_infile.is_open());
		ensure(ar15_infile.good());

		stl_import importer(ar15_infile);

		triangle_mesh mesh;
		mesh.build(importer.get_facets());

		ensure(mesh.get_facets().size() == 36288);
		ensure(mesh.is_manifold());
	}
};
