#include "example.h"

int LifeTime::s_counter = 0;

/* static member var */
bool TestMember::s_boolean_val = false;
int TestMember::s_int_val = 1;
long TestMember::s_long_val = 1;
char TestMember::s_name_val[64];
ExportObj TestMember::s_export_val;
NoneExportObj TestMember::s_none_export_va;
std::vector<int> TestMember::s_vector_val;
std::map<std::string, TestMember*> TestMember::s_map_val;
int TestMember::s_lua_name__ = 0;
int TestMember::s_read_only = 1;
