PROJECT(test_xlua)

SET(TEST_SRC
	example.h
	example.cpp
	lua_export.cpp
	lua_export.h
	test.cpp
	test.h
	main.cpp
)

aux_source_directory("../xlua/" TEST_SRC)

INCLUDE_DIRECTORIES("../3rd/lua-5.3.5/src/")
INCLUDE_DIRECTORIES("../xlua/")
INCLUDE_DIRECTORIES("gtest/")
LINK_DIRECTORIES("../3rd/gtest/lib/")
LINK_LIBRARIES(gtest.lib)

ADD_EXECUTABLE(test_xlua ${TEST_SRC})
#ADD_DEPENDENCIES(test lua xlua)
#TARGET_LINK_LIBRARIES(test_xlua lua xlua)
TARGET_LINK_LIBRARIES(test_xlua lua)
