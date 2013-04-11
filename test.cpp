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

		string stl_str = get_tetrahedron_stl_str();
		std::istringstream tet_is(stl_str);

		stl_import importer(tet_is);

		triangle_mesh mesh;
		mesh.build(importer.get_facets());

		ensure(mesh.get_edges().size() == 12);
		ensure(mesh.get_vertices().size() == 4);
		ensure(mesh.get_facets().size() == 4);
		ensure(mesh.is_manifold());
	}
};
