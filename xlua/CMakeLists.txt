PROJECT(xlua)

SET(XLUA_SRC
	detail/common.h
	detail/export.h
	detail/extractor.h
	detail/global.h
	detail/serial_alloc.h
	detail/state.h
	detail/traits.h
	detail/type_desc.h
	detail/type_desc.cpp
	detail/global.cpp
	xlua_config.h
	xlua_def.h
	xlua_state.h
	xlua_export.h
	xlua.h
	xlua_state.cpp
	xlua.cpp
)

INCLUDE_DIRECTORIES("../3rd/lua-5.3.5/src/")
ADD_LIBRARY(xlua STATIC ${XLUA_SRC})
