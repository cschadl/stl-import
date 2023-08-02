#include "mesh2.h"
#include <algorithm>
#include <array>
#include <stack>
#include <vector>
#include <unordered_set>
#include <kdtree/kdtree.hpp>

namespace cds
{

namespace cgl
{

namespace detail_
{
    constexpr size_t invalidIdx = std::numeric_limits<size_t>::max();

    // A point and its associated index in a list of vertices
    template <typename PointType>
    struct pointAndIndex
    {
        PointType p;
        size_t index;

        using value_type = typename kdtree::point_traits<PointType>::value_type;

        value_type operator[](size_t idx) const { return p[idx]; }

        static constexpr size_t Dim = 3;

        pointAndIndex(value_type v)
            : p(v)
            , index(invalidIdx)
        {

        }

        pointAndIndex(PointType const& p_)
            : p(p_)
            , index(invalidIdx)
        {

        }

        pointAndIndex(PointType const& p_, size_t index)
            : p(p_)
            , index(index)
        {

        }

        pointAndIndex()
            : p(value_type(0))
            , index(invalidIdx)
        {

        }
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

    static_assert(std::is_same_v<std::decay_t<decltype(*pts_begin)>, PointType>);
    static_assert(std::is_same_v<std::decay_t<decltype(*tris_begin)>, FacetType>);
    static_assert(kdtree::point_traits<PointType>::dim() == 3);

    constexpr size_t invalidIdx = detail_::invalidIdx;
    
    // "Collapse" the points into a unique set of vertices (to within tol)

    std::vector<detail_::pointAndIndex<PointType>> point_indices;
    for (auto tri_it = tris_begin ; tri_it != tris_end ; ++tri_it)
    {
        auto const& tri = *tri_it;
        for (int i = 0 ; i < 3 ; i++)
        {
            // TODO - handle degenerate vertices
            auto const& pt = *(pts_begin + tri[i]);

            point_indices.emplace_back(pt, tri[i]);
        }
    }

    kdtree::kd_tree<detail_::pointAndIndex<PointType>> pt_orig_idx_tree;
    pt_orig_idx_tree.build(point_indices.begin(), point_indices.end());

    std::unordered_set<size_t> processed_indices;
    std::vector<PointType> unique_vertices;

    for (auto pt_it = pts_begin ; pt_it != pts_end ; ++pt_it)
    {
        auto const& pt_orig_indices = pt_orig_idx_tree.radius_search(*pt_it, tolerance);
        if (pt_orig_indices.empty())
            return false;   // Shouldn't happen

        // The search returns all indices associated with this point in the original
        // list of triangles. Add them to the processed_indices map (so we don't process them again)
        if (std::none_of(pt_orig_indices.begin(), pt_orig_indices.end(),
            [&processed_indices](const auto& pt_idx)
            {
                return processed_indices.find(pt_idx.index) != processed_indices.end();
            }))
        {
            unique_vertices.push_back(pt_orig_indices.front().p);
            
            std::transform(pt_orig_indices.begin(), pt_orig_indices.end(),
                std::inserter(processed_indices, processed_indices.end()),
                [](const auto& pt_idx) { return pt_idx.index; });
        }
    }

    // OK, now make a new lookup tree out of our unique vertices

    std::vector<detail_::pointAndIndex<PointType>> unique_vert_indices;
    for (size_t i = 0 ; i < unique_vertices.size() ; i++)
    {
        unique_vert_indices.emplace_back(unique_vertices[i], i);
    }

    kdtree::kd_tree<detail_::pointAndIndex<PointType>> vertex_lookup_tree;
    vertex_lookup_tree.build(unique_vert_indices.begin(), unique_vert_indices.end());

    // Finally, build new connected facets from our unique verts

    std::vector<FacetType> mesh_facets;
    for (auto tri_it = tris_begin ; tri_it != tris_end ; ++tri_it)
    {
        auto const& orig_tri = *tri_it;

        FacetType new_facet{ invalidIdx, invalidIdx, invalidIdx };

        for (int i = 0 ; i < 3 ; i++)
        {
            auto const& p = *(pts_begin + orig_tri[i]);
            
            auto const vert_and_index = vertex_lookup_tree.k_nn(p, 1);
            if (vert_and_index.empty())
                return false;   // shouldn't happen

            new_facet[i] = vert_and_index.front().index;
            
            if (new_facet[i] == invalidIdx)
                return false;   // Also shouldn't happen
        }

        mesh_facets.push_back(new_facet);
    }

    outMesh = mesh2<PointType>(unique_vertices, mesh_facets);

    return true;
}

}

}