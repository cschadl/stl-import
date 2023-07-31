#include "mesh2.h"
#include <array>
#include <stack>
#include <vector>
#include <kdtree/kdtree.hpp>

namespace cds
{

namespace cgl
{

namespace detail_
{
    constexpr size_t invalidIdx = std::numeric_limits<size_t>::max();

    // A point and its associated triangle
    template <typename PointType>
    struct pointAndTriangle
    {
        PointType p;
        typename mesh2<PointType>::FacetType tri;

        using value_type = typename kdtree::point_traits<PointType>::value_type;

        value_type operator[](size_t idx) const { return p[idx]; }

        static constexpr size_t Dim = 3;

        pointAndTriangle(value_type v)
            : p(v)
            , tri{invalidIdx, invalidIdx, invalidIdx}
        {

        }

        pointAndTriangle(PointType const& p_)
            : p(p_)
            , tri{invalidIdx, invalidIdx, invalidIdx}
        {

        }

        pointAndTriangle(PointType const& p_, typename mesh2<PointType>::FacetType const& facet)
            : p(p_)
            , tri(facet)
        {

        }

        pointAndTriangle()
            : p(value_type(0))
            , tri{invalidIdx, invalidIdx, invalidIdx}
        {

        }
    };

    template <typename PointType>
    struct facetMap
    {
        using FacetType = typename mesh2<PointType>::FacetType;

        FacetType origFacetIndex;
        FacetType newFacetIndex;
    };
}

template <typename PointIteratorType, typename TriIteratorType, typename PointType>
bool build_mesh(
    PointIteratorType pts_begin, PointIteratorType pts_end,
    TriIteratorType tris_begin, TriIteratorType tris_end,
    typename kdtree::point_traits<PointType>::value_type tolerance,
    mesh2<PointType>& outMesh)
{
    using FacetType = typename mesh2<PointType>::FacetType;

    constexpr size_t invalidIdx = detail_::invalidIdx;

    static_assert(std::is_same_v<std::decay_t<decltype(*pts_begin)>, PointType>);
    static_assert(std::is_same_v<std::decay_t<decltype(*tris_begin)>, FacetType>);
    static_assert(kdtree::point_traits<PointType>::dim() == 3);
    
    std::vector<detail_::pointAndTriangle<PointType>> pts_tris;
    for (auto tri_it = tris_begin ; tri_it != tris_end ; ++tri_it)
    {
        auto const& t = *tri_it;
        for (int j = 0 ; j < 3; j++)
        {
            auto const pt_idx = t[j];
            auto const& pt = *(pts_begin + pt_idx);

            pts_tris.emplace_back(
                detail_::pointAndTriangle<PointType>{ pt, t }
            );
        }
    }

    kdtree::kd_tree<detail_::pointAndTriangle<PointType>> lookup_tree;
    lookup_tree.build(pts_tris.begin(), pts_tris.end());

    std::stack<std::pair<FacetType, FacetType>> facet_stack;

    facet_stack.emplace(
        std::make_pair(
            *tris_begin++,
            FacetType{ invalidIdx, invalidIdx, invalidIdx })
    );

    auto tri_it = tris_begin;

    std::vector<PointType> mesh_points;
    std::vector<FacetType> mesh_facets;

    while (!facet_stack.empty())
    {
        FacetType new_facet;
        FacetType old_facet;

        std::tie(old_facet, new_facet) = facet_stack.top();
        facet_stack.pop();

        // Make a new triangle from this triangle
        for (int i = 0 ; i < 3 ; i++)
        {
            if (new_facet[i] != invalidIdx)
                continue;   // This vert is an existing vert one or more facets

            auto const p = *(pts_begin + new_facet[i]);

            // Add this new point to the list of mesh vertices
            mesh_points.push_back(p);
            size_t pt_idx = mesh_points.size() - 1;

            auto const& near_pts_tris = lookup_tree.radius_search(p, tolerance);
            for (const auto& np : near_pts_tris)
            {
                auto const& p = np.p;
                auto const& t = np.tri;

                FacetType np_new_tri{
                    pt_idx,
                    invalidIdx,
                    invalidIdx
                };

                // TODO - check for degenerate triangle
                facet_stack.emplace(std::make_pair(old_facet, np_new_tri));
            }
        }

        mesh_facets.push_back(new_facet);
    }

    outMesh = mesh2<PointType>(mesh_points, mesh_facets);

    return true;
}

}

}