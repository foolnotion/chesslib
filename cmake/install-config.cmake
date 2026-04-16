include(CMakeFindDependencyMacro)

# needed for static builds — PRIVATE deps are $<LINK_ONLY:...> for static archives
if(NOT chesslib_SHARED_LIBS)  # or check via a generated variable
    find_dependency(fmt CONFIG)
    find_dependency(libassert CONFIG)
    find_dependency(mdspan CONFIG)
endif()

find_dependency(magic_enum CONFIG)
find_dependency(small_vector CONFIG)
find_dependency(tl-expected CONFIG)
find_dependency(unordered_dense CONFIG)

include("${CMAKE_CURRENT_LIST_DIR}/chesslibTargets.cmake")
