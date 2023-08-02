#include "build_mesh.h"
#include "mesh2.h"

#include <array>

#include <vectors.h>
#include <geom.h>

#include <tut.h>

using namespace maths;
using namespace cds::kdtree;
using namespace cds::cgl;

namespace tut
{

struct build_mesh_test_data { };

typedef test_group<build_mesh_test_data> build_mesh_test_data_t;
build_mesh_test_data_t build_mesh_tests("build_mesh");

template <> template <>
void build_mesh_test_data_t::object::test<1>()
{
    set_test_name("Unit cube");
    
    using facet_type = mesh2<vector3d>::FacetType;

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

    for (size_t i = 0 ; i < points.size() ; )
        facets.push_back(facet_type{i++, i++, i++});

    mesh2<vector3d> cube_mesh;
    ensure(
        cds::cgl::build_mesh(
            points.begin(), points.end(),
            facets.begin(), facets.end(),
            1e-6,
            cube_mesh));

    auto const mesh_verts = cube_mesh.get_vertices();
    ensure_equals(mesh_verts.size(), 8);

    std::vector<vector3d> expected_verts{
        vector3d(0, 0, 0),
        vector3d(0, 0, 1),
        vector3d(0, 1, 0),
        vector3d(0, 1, 1),
        vector3d(1, 0, 0),
        vector3d(1, 0, 1),
        vector3d(1, 1, 0),
        vector3d(1, 1, 1)
    };

    for (auto const& expected_v : expected_verts)
    {
        // Ensure that each expected vertex shows up in the mesh only once
        auto cube_v = std::find(mesh_verts.begin(), mesh_verts.end(), expected_v);

        ensure(cube_v != expected_verts.end());
        ensure(std::find(std::next(cube_v), mesh_verts.end(), expected_v) == mesh_verts.end());
    }

    auto mesh_facets = cube_mesh.get_facets();
    ensure_equals(mesh_facets.size(), 12);

    std::vector<vector3d> expected_normals{
        vector3d(1, 0, 0),
        vector3d(0, 1, 0),
        vector3d(0, 0, 1),
        vector3d(-1, 0, 0),
        vector3d(0, -1, 0),
        vector3d(0, 0, -1)
    };

    for (auto const& expected_n : expected_normals)
    {
        // Ensure that we have two of each facet with the expected normal,
        // and that these facets each share an edge (two vertices)
        auto facets_with_expected_normal = std::partition(
            mesh_facets.begin(), mesh_facets.end(),
            [&expected_n, &mesh_verts](facet_type const& f)
            {
                auto const& v0 = mesh_verts[f[0]];
                auto const& v1 = mesh_verts[f[1]];
                auto const& v2 = mesh_verts[f[2]];

                maths::triangle3d f_tri(v0, v1, v2);

                auto const& n = f_tri.normal();

                auto dot = n * expected_n;
                return maths::close(dot, 1.0, 1.0e-12);
            });

        ensure_equals(std::distance(mesh_facets.begin(), facets_with_expected_normal), 2);

        auto f1 = *(mesh_facets.begin());
        auto f2 = *(std::next(mesh_facets.begin()));

        std::sort(f1.begin(), f1.end());
        std::sort(f2.begin(), f2.end());

        std::vector<mesh2<vector3d>::IndexType> shared_indices;
        std::set_intersection(
            f1.begin(), f1.end(),
            f2.begin(), f2.end(),
            std::back_inserter(shared_indices));

        ensure_equals(shared_indices.size(), 2);
    }
}

}