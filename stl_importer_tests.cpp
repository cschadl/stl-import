#include "stl_importer.h"
#include "triangle_mesh.h"

#include <tut.h>

#include <stdio.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/param.h>
#include <math.h>

using namespace std;

namespace tut
{

struct stl_importer_test_data
{
	std::string _test_data_path;

	stl_importer_test_data()
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

typedef test_group<stl_importer_test_data> stl_importer_test_t;
stl_importer_test_t stl_importer_tests("stl_importer tests");

template <> template <>
void stl_importer_test_t::object::test<1>()
{
	set_test_name("Read facets");

	auto ss = make_shared<istringstream>(get_tetrahedron_stl_str());
	stl_util::stl_importer importer(ss);
	ensure_equals(importer.num_facets_expected(), 4);

	std::vector<maths::triangle3d> stl_triangles(importer.num_facets_expected());

	importer.import(stl_triangles.begin());

	ensure(stl_triangles.size() == 4);
	ensure(importer.num_facets_read() == 4);
}

template <> template <>
void stl_importer_test_t::object::test<2>()
{
	set_test_name("Long solid name");

	stl_util::stl_importer importer(test_data_path() + "/humanoid.stl");

	std::vector<maths::triangle3d> stl_triangles;
	importer.import(back_inserter(stl_triangles));

	ensure(importer.name() == "MYSOLID created by IVCON, original data in file.obj");
	ensure(stl_triangles.size() == 96);
}

template <> template <>
void stl_importer_test_t::object::test<3>()
{
	set_test_name("Binary STL");

	stl_util::stl_importer importer(test_data_path() + "/unit_cube.stl");

	ensure(importer.num_facets_expected() == 12);

	std::vector<maths::triangle3d> stl_triangles;
	importer.import(back_inserter(stl_triangles));

	ensure(importer.name() == "Output by MakerBot Kit for MODO's modo");
	ensure(stl_triangles.size() == 12);
}

template <> template <>
void stl_importer_test_t::object::test<4>()
{
	set_test_name("Tab in solid element");

	stl_util::stl_importer importer(test_data_path() + "/sphere.stl");

	auto num_facets_expected = importer.num_facets_expected();
	ensure(num_facets_expected > 0);

	std::vector<maths::triangle3d> stl_triangles(num_facets_expected);
	importer.import(stl_triangles.begin());

	ensure(stl_triangles.size() == num_facets_expected);
}

};
