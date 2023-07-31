#include "build_mesh.h"
#include "mesh2.h"

#include <array>

#include <vectors.h>
#include <geom.h>

#include <tut.h>

using namespace maths;
using namespace cds::kdtree;

namespace tut
{

struct build_mesh_test_data { };

typedef test_group<build_mesh_test_data> build_mesh_test_data_t;
build_mesh_test_data_t build_mesh_tests("build_mesh");

template <> template <>
void build_mesh_test_data_t::object::test<1>()
{
    set_test_name("Unit cube");
    
    using facet_type = std::array<size_t, 3>;

    std::vector<maths::vector3d> points;
    std::vector<facet_type> facets;
    
    points.push_back(vector3d(0., 1., 1.));
    points.push_back(vector3d(0., 1., 0.));
    points.push_back(vector3d(0., 0., 1.));

    points.push_back(vector3d(0., 0., 1.));
    points.push_back(vector3d(0., 1., 0.));
    points.push_back(vector3d(0., 0., 0.));

    points.push_back(vector3d(1., 1., 1.));
    points.push_back(vector3d(0., 1., 1.));
    points.push_back(vector3d(1., 0., 1.));

    points.push_back(vector3d(1., 0., 1.));
    points.push_back(vector3d(0., 1., 1.));
    points.push_back(vector3d(0., 0., 1.));

    points.push_back(vector3d(1., 1., 0.));
    points.push_back(vector3d(1., 1., 1.));
    points.push_back(vector3d(1., 0., 0.));

    points.push_back(vector3d(1., 0., 0.));
    points.push_back(vector3d(1., 1., 1.));
    points.push_back(vector3d(1., 0., 1.));

    points.push_back(vector3d(0., 1., 0.));
    points.push_back(vector3d(1., 1., 0.));
    points.push_back(vector3d(0., 0., 0.));

    points.push_back(vector3d(0., 0., 0.));
    points.push_back(vector3d(1., 1., 0.));
    points.push_back(vector3d(1., 0., 0.));

    points.push_back(vector3d(1., 1., 1.));
    points.push_back(vector3d(1., 1., 0.));
    points.push_back(vector3d(0., 1., 1.));

    points.push_back(vector3d(0., 1., 1.));
    points.push_back(vector3d(1., 1., 0.));
    points.push_back(vector3d(0., 1., 0.));

    points.push_back(vector3d(1., 0., 0.));
    points.push_back(vector3d(1., 0., 1.));
    points.push_back(vector3d(0., 0., 0.));

    points.push_back(vector3d(0., 0., 0.));
    points.push_back(vector3d(1., 0., 1.));
    points.push_back(vector3d(0., 0., 1.));

    for (size_t i = 0 ; i < points.size() / 3 ; )
        facets.push_back(facet_type{i++, i++, i++});

    cds::cgl::mesh2<vector3d> cube_mesh;
    ensure(
        cds::cgl::build_mesh(
            points.begin(), points.end(),
            facets.begin(), facets.end(),
            1e-6,
            cube_mesh));
}

}