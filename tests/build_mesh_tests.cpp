#include "build_mesh.h"

#include <mathstuff/vectors.h>

#include <tut.h>

namespace tut
{

struct build_mesh_test_data { };

typedef test_group<build_mesh_test_data> build_mesh_test_data_t;
build_mesh_test_data_t build_mesh_tests("build_mesh");

template <> template <>
void build_mesh_test_data_t::object::test<1>()
{
    set_test_name("Unit cube");
    
}

}