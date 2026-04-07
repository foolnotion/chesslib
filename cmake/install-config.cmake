include(CMakeFindDependencyMacro)

find_dependency(magic_enum CONFIG)
find_dependency(small_vector CONFIG)
find_dependency(tl-expected CONFIG)
find_dependency(unordered_dense CONFIG)

include("${CMAKE_CURRENT_LIST_DIR}/chesslibTargets.cmake")
