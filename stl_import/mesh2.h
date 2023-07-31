#include <vector>
#include <array>

namespace cds
{

namespace cgl
{

template <typename PointType>
class mesh2
{
    using value_type = PointType::value_type;

    using IndexType = size_t;
    using FacetType = std::array<IndexType, 3>;  // triangle defined by vert indices

private:
    std::vector<PointType>  m_verts;
    std::vector<FacetType>  m_facets;

public:
    mesh2() = default;

    mesh2(std::vector<PointType> const& verts, std::vector<FacetType> const& facets)
        : m_verts(verts)
        , m_facets(facets)
    {

    }

    IndexType add_vertex(PointType const& p);   // returns index of vertex that was added

    std::vector<PointType> const& get_vertices() const { return m_verts; }
    std::vector<FacetType> const& get_facets() const { return m_facets; }

    // TODO - topology queries
};

}

}