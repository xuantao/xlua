PROJECT(test)

SET(TEST_SRC
	lua_export.cpp
	lua_export.h
	main.cpp
	shape.h
	test.h
	test_export.cpp
	test_global.cpp
	test_push_load.cpp
)

INCLUDE_DIRECTORIES("../3rd/lua-5.3.5/src/")
INCLUDE_DIRECTORIES("../xlua/")

ADD_EXECUTABLE(test ${TEST_SRC})
#ADD_DEPENDENCIES(test lua xlua)
TARGET_LINK_LIBRARIES(test lua xlua)
