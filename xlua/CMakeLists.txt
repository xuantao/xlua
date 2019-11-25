PROJECT(xlua)

SET(XLUA_SRC
	scripts.hpp
	support.hpp
	core.h
	core.cpp
	xlua_config.h
	xlua_def.h
	xlua_state.h
	xlua_export.h
)

INCLUDE_DIRECTORIES("../3rd/lua-5.3.5/src/")
ADD_LIBRARY(xlua STATIC ${XLUA_SRC})
