include(CMakeFindDependencyMacro)

# PRIVATE link deps of a static archive must be re-found so that consumers
# can resolve $<LINK_ONLY:...> entries in INTERFACE_LINK_LIBRARIES.
# The previous if(NOT chesslib_SHARED_LIBS) guard was fragile — that variable
# is undefined at consumer config time unless explicitly generated.
find_dependency(fmt CONFIG)
find_dependency(libassert CONFIG)
find_dependency(mdspan CONFIG)
find_dependency(magic_enum CONFIG)
find_dependency(small_vector CONFIG)
find_dependency(tl-expected CONFIG)
find_dependency(unordered_dense CONFIG)

include("${CMAKE_CURRENT_LIST_DIR}/chesslibTargets.cmake")
