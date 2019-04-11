#include "xlua_state.h"
#include "detail/global.h"

XLUA_NAMESPACE_BEGIN

#define _XLUA_GET_STATE(l)  xlua::detail::GlobalVar::GetInstance()->GetState(l)

namespace detail {
    static void TraceCallStack(lua_State* L, xLuaLogBuffer& lb) {
        int level = 1;
        lua_Debug dbg;

        lb.Log("stack taceback:");
        while (lua_getstack(L, level, &dbg)) {
            lua_getinfo(L, "nSl", &dbg);
            ++level;

            if (dbg.name)
                lb.Log("    %s:%d, in faild '%s'", dbg.source, dbg.currentline, dbg.name);
            else
                lb.Log("    %s:%d, in main chunk", dbg.source, dbg.currentline);
        }

        if (level == 1)
            lb.Log("    in C?");
    };

    static void LogWithStack(lua_State* l, const char* fmt, ...) {
        LogBufCache<> lb;
        {
            char tmp[XLUA_MAX_BUFFER_CACHE];
            va_list args;
            va_start(args, fmt);
            int n = vsnprintf(tmp, XLUA_MAX_BUFFER_CACHE, fmt, args);
            va_end(args);
            lb.Log(tmp);
        }

        TraceCallStack(l, lb);
        LogError(lb.Finalize());
    }

    static bool HasGlobal(const TypeInfo* info) {
        while (info) {
            if (info->global_vars[0].name || info->global_funcs[0].name)
                return true;
            info = info->super;
        }
        return false;
    }

    static bool IsObjValid(lua_State* l) {
        int l_ty = lua_type(l, 1);
        if (l_ty == LUA_TLIGHTUSERDATA) {
#if XLUA_USE_LIGHT_USER_DATA
            auto ud = MakeLightPtr(lua_touserdata(l, 1));
            if (!ud.IsLightData())
                return false;

            if (ud.IsInternalType()) {
                auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud.index_);
                return (ary_obj != nullptr && ary_obj->serial_num_ == ud.serial_);
            } else {
                const auto* src_info = GlobalVar::GetInstance()->GetExternalTypeInfo(ud.type_);
                if (src_info == nullptr)
                    return false;

                if (src_info->is_weak_obj)
                    return (ud.serial_ == xLuaGetWeakObjSerialNum(ud.index_));
                return true;
            }
#endif // XLUA_USE_LIGHT_USER_DATA
        } else if (l_ty == LUA_TUSERDATA) {
            auto* ud = static_cast<FullUserData*>(lua_touserdata(l, 1));
            if (ud == nullptr)
                return false;

            if (ud->type_ == UserDataCategory::kObjPtr) {
                if (ud->info_->is_weak_obj) {
                    return (ud->serial_ == xLuaGetWeakObjSerialNum(ud->index_));
                } else {
                    auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud->index_);
                    return (ary_obj == nullptr && ary_obj->serial_num_ == ud->serial_);
                }
            }
            return true;
        }
        return false;
    }

    struct MetaFuncs {
        static int LuaIndex(lua_State* l) {
            lua_getmetatable(l, 1);     // 1: ud, 2: key, 3: metatable
            lua_pushvalue(l, 2);        // 4: copy key
            int ty = lua_rawget(l, -2); // 4: table member
            if (ty == LUA_TFUNCTION)
                return 1;

            UserDataInfo ud_info{ false, nullptr, nullptr, nullptr };
            if (!GetUserDataInfo(l, 1, &ud_info)) {
                LogWithStack(l, "meta \"__index\" object is nullptr");
                return 0;
            }

            if (ty != LUA_TLIGHTUSERDATA) {
                const char* name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] is not exist", ud_info.info->type_name, name ? name : "?");
                return 0;
            }

            auto* mem = (MemberVar*)lua_touserdata(l, -1);
            assert(mem->name);
            if (mem->getter == nullptr) {
                const char* name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] can not been read", ud_info.info->type_name, name ? name : "?");
                return 0;
            }

            xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
            mem->getter(xl, ud_info.obj, ud_info.info);
            return 1;
        }

        static int LuaNewIndex(lua_State* l) {
            UserDataInfo ud_info{ false, nullptr, nullptr, nullptr };
            if (!GetUserDataInfo(l, 1, &ud_info)) {
                LogWithStack(l, "meta \"__newindex\" object is nullptr");
                return 0;
            }

            lua_getmetatable(l, 1);     // 1: ud, 2: key, 3: value, 4: metatable
            lua_pushvalue(l, 2);        // 5: copy_key
            int ty = lua_rawget(l, -2); // -1: key, -2: metatable
            if (ty != LUA_TLIGHTUSERDATA) {
                const char* name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] is not exist", ud_info.info->type_name, name ? name : "?");
                return 0;
            }

            auto* mem = (MemberVar*)lua_touserdata(l, -1);
            assert(mem->name);
            if (mem->setter == nullptr) {
                const char* name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] is read only", ud_info.info->type_name, name ? name : "?");
                return 0;
            }

            xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
            lua_pop(l, 2);  // pop: -1: type_member, -2: metatable
            mem->setter(xl, ud_info.obj, ud_info.info);
            return 0;
        }

        static int LuaGc(lua_State* l) {
            xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
            xl->Gc(static_cast<FullUserData*>(lua_touserdata(l, 1)));
            return 0;
        }

        static int LuaToString(lua_State* l) {
            return 0;
        }

        static int LuaLen(lua_State* l) {
            return 0;
        }

        static int LuaPairs(lua_State* l) {
            return 0;
        }

#if XLUA_USE_LIGHT_USER_DATA
        // stack info, 1: ud, 2: key
        static int LuaLightPtrIndex(lua_State* l) {
            UserDataInfo ud_info{ false, nullptr, nullptr, nullptr };
            if (!GetUserDataInfo(l, 1, &ud_info)) {
                LogWithStack(l, "meta \"__index\" object is nullptr");
                return 0;
            }

            xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
            lua_rawgeti(l, LUA_REGISTRYINDEX, xl->meta_table_ref_); // metatable list
            lua_rawgeti(l, -1, ud_info.info->index);               // active metatable
            lua_pushvalue(l, 2);            // copy key
            int ty = lua_rawget(l, -2);     // 1: ud, 2: key, 3: meta_list, 4: metatable, 5: key
            if (ty == LUA_TFUNCTION)
                return 1;   // function

            if (ty != LUA_TLIGHTUSERDATA) {
                const char* name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] is not exist", ud_info.info->type_name, name ? name : "?");
                return 0;
            }

            auto* mem = (MemberVar*)lua_touserdata(l, -1);
            assert(mem->name);
            if (mem->getter == nullptr) {
                const char* name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] can not been read", ud_info.info->type_name, name ? name : "?");
                return 0;
            }

            mem->getter(xl, ud_info.obj, ud_info.info);
            return 1;
        }

        // lua stack, [1]:ud, [2]:key, [3]:value
        static int LuaLightPtrNewIndex(lua_State* l) {
            UserDataInfo ud_info{ false, nullptr, nullptr, nullptr };
            if (!GetUserDataInfo(l, 1, &ud_info)) {
                LogWithStack(l, "meta \"__index\" object is nullptr");
                return 0;
            }

            xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
            lua_rawgeti(l, LUA_REGISTRYINDEX, xl->meta_table_ref_);
            lua_rawgeti(l, -1, ud_info.info->index);
            lua_pushvalue(l, 2);    // copy key to top
            if (lua_rawget(l, -2) != LUA_TLIGHTUSERDATA) {
                const char* name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] is not exist", ud_info.info->type_name, name ? name : "?");
                return 0;
            }

            auto* mem = (MemberVar*)lua_touserdata(l, -1);
            assert(mem->name);
            if (mem->setter == nullptr) {
                const char* name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] is read only", ud_info.info->type_name, name ? name : "?");
                return 0;
            }

            // stack: [1]:ud, [2]:key, [3]:value, [4]:meta_list, [5]:metatable, [6]: member
            lua_pop(l, 3);

            mem->setter(xl, ud_info.obj, ud_info.info);
            return 0;
        }
#endif

        static int LuaGlobalIndex(lua_State* l) {
            lua_getmetatable(l, 1);
            lua_pushvalue(l, 2);
            int ty = lua_rawget(l, -2); // 4: table member
            if (ty != LUA_TLIGHTUSERDATA) {
                lua_pushstring(l, "__name");
                lua_rawget(l, 3);

                const char* table_name = lua_tostring(l, -1);
                const char* member_name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] is not exist", table_name ? table_name : "?", member_name ? member_name : "?");
                return 0;
            }

            auto* mem = (MemberVar*)lua_touserdata(l, -1);
            assert(mem->name);
            if (mem->getter == nullptr) {
                lua_pushstring(l, "__name");
                lua_rawget(l, 3);

                const char* table_name = lua_tostring(l, -1);
                const char* member_name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] can not been read", table_name ? table_name : "?", member_name ? member_name : "?");
                return 0;
            }

            xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
            mem->getter(xl, nullptr, nullptr);
            return 1;
        }

        static int LuaGlobalNewIndex(lua_State* l) {
            lua_getmetatable(l, 1);
            lua_pushvalue(l, 2);
            int ty = lua_rawget(l, -2); // 5: table member
            if (ty != LUA_TLIGHTUSERDATA) {
                lua_pushstring(l, "__name");
                lua_rawget(l, 4);

                const char* table_name = lua_tostring(l, -1);
                const char* member_name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] is not exist", table_name ? table_name : "?", member_name ? member_name : "?");
                return 0;
            }

            auto* mem = (MemberVar*)lua_touserdata(l, -1);
            assert(mem->name);
            if (mem->setter == nullptr) {
                lua_pushstring(l, "__name");
                lua_rawget(l, 4);

                const char* table_name = lua_tostring(l, -1);
                const char* member_name = lua_tostring(l, 2);
                LogWithStack(l, "type:[%s] member:[%s] is ready only", table_name ? table_name : "?", member_name ? member_name : "?");
                return 0;
            }

            xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
            mem->setter(xl, nullptr, nullptr);
            return 0;
        }

        static int LuaConstIndex(lua_State* l) {
            lua_pushstring(l, "__name");
            if (lua_rawget(l, 1) != LUA_TSTRING || lua_isstring(l, 2) == 0)
                return 0;

            LogWithStack(l, "const Type:[%s] member:[%s] does not exist", lua_tostring(l, -1), lua_tostring(l, 2));
            return 0;
        }

        static int LuaConstNewIndex(lua_State* l) {
            lua_pushstring(l, "__name");
            if (lua_rawget(l, 1) != LUA_TSTRING || lua_isstring(l, 2) == 0)
                return 0;

            LogWithStack(l, "const Type:[%s] can not add new member:[%s]", lua_tostring(l, -1), lua_tostring(l, 2));
            return 0;
        }

        static int LuaCast(lua_State* l) {
            const char* dst_name = lua_tostring(l, 2);
            const TypeInfo* dst_info = GlobalVar::GetInstance()->GetTypeInfo(dst_name);
            if (dst_info == nullptr) {
                lua_pushnil(l);
                LogWithStack(l, "can not find dest type info[%s]", dst_name ? dst_name : "");
                return 1;
            }

            int l_ty = lua_type(l, 1);
            if (l_ty == LUA_TLIGHTUSERDATA) {
#if XLUA_USE_LIGHT_USER_DATA
                auto ud = MakeLightPtr(lua_touserdata(l, 1));
                LogBufCache<> lb;
                if (ud.Ptr() == nullptr) {
                    lua_pushnil(l);
                    LogWithStack(l, "unknown obj");
                    return 1;
                }

                if (!dst_info->caster->ToDerived(&ud, lb)) {
                    LogWithStack(l, lb.Finalize());
                    lua_pushnil(l);
                    return 1;
                }

                lua_pushlightuserdata(l, ud.Ptr());
                return 1;
#else
                lua_pushnil(l);
                LogWithStack(l, "unknown obj");
                return 1;
#endif // XLUA_USE_LIGHT_USER_DATA
            } else if (l_ty == LUA_TUSERDATA) {
                auto* ud = static_cast<FullUserData*>(lua_touserdata(l, 1));
                LogBufCache<> lb;
                if (ud == nullptr || ud->info_ == nullptr) {
                    lua_pushnil(l);
                    LogWithStack(l, "unknown obj");
                    return 1;
                }

                if (IsBaseOf(dst_info, ud->info_)) {
                    lua_pushvalue(l, 1);
                    return 1;   // not nessasery
                }

                if (!dst_info->caster->ToDerived(ud, lb)) {
                    LogWithStack(l, lb.Finalize());
                    lua_pushnil(l);
                    return 1;
                }

                xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
                lua_rawgeti(l, LUA_REGISTRYINDEX, xl->meta_table_ref_);
                lua_rawgeti(l, -1, dst_info->index);
                lua_remove(l, -2);
                lua_setmetatable(l, 1);
                lua_pushvalue(l, 1);        // copy object to stack top
                return 1;
            } else {
                lua_pushnil(l);
                LogWithStack(l, "can not cast unknown value");
                return 1;
            }
        }

        static int LuaIsValid(lua_State* l) {
            bool isValid = IsObjValid(l);
            lua_pushboolean(l, isValid);
            return 1;
        }

        static int LuaType(lua_State* l) {
            int l_ty = lua_type(l, 1);
            if (l_ty == LUA_TLIGHTUSERDATA) {
#if XLUA_USE_LIGHT_USER_DATA
                auto ud = MakeLightPtr(lua_touserdata(l, 1));
                if (!ud.IsLightData()) {
                    lua_pushstring(l, "Unknown");
                    return 1;
                }
                if (ud.IsInternalType()) {
                    auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud.index_);
                    if (ary_obj == nullptr || ary_obj->serial_num_ != ud.serial_)
                        lua_pushstring(l, "nil");
                    else
                        lua_pushstring(l, ary_obj->info_->type_name);
                    return 1;
                } else {
                    const auto* info = GlobalVar::GetInstance()->GetExternalTypeInfo(ud.type_);
                    if (info == nullptr) {
                        lua_pushstring(l, "Unknown");
                        return 1;
                    }

                    if (info->is_weak_obj) {
                        if (ud.serial_ != xLuaGetWeakObjSerialNum(ud.index_)) {
                            lua_pushstring(l, "nil");
                            return 1;
                        }
                    }
                    lua_pushstring(l, info->type_name);
                    return 1;
                }
#else // XLUA_USE_LIGHT_USER_DATA
                lua_pushstring(l, "Unknown");
                return 1;
#endif // XLUA_USE_LIGHT_USER_DATA
            } else if (l_ty == LUA_TUSERDATA) {
                auto* ud = static_cast<FullUserData*>(lua_touserdata(l, 1));
                if (ud == nullptr) {
                    lua_pushstring(l, "Unknown");
                    return 1;
                }

                if (ud->type_ == UserDataCategory::kObjPtr) {
                    if (ud->info_->is_weak_obj) {
                        if (ud->serial_ != xLuaGetWeakObjSerialNum(ud->index_)) {
                            lua_pushstring(l, "nil");
                            return 1;
                        }
                    } else {
                        auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud->index_);
                        if (ary_obj == nullptr || ary_obj->serial_num_ != ud->serial_) {
                            lua_pushstring(l, "nil");
                            return 1;
                        }
                    }
                }

                lua_pushstring(l, ud->info_->type_name);
                return 1;
            } else {
                lua_pushstring(l, lua_typename(l, 1));
                return 1;
            }
        }

        static int LuaGetTypeMeta(lua_State* l) {
            const auto* info = GlobalVar::GetInstance()->GetTypeInfo(lua_tostring(l, 1));
            if (info == nullptr || info->index == 0)
                return 0;

            xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
            lua_rawgeti(l, LUA_REGISTRYINDEX, xl->meta_table_ref_);
            lua_rawgeti(l, -1, info->index);
            lua_remove(l, -2);
            return 1;
        }

        static int LuaGetMetaList(lua_State* l) {
            xLuaState* xl = static_cast<xLuaState*>(lua_touserdata(l, lua_upvalueindex(1)));
            lua_rawgeti(l, LUA_REGISTRYINDEX, xl->meta_table_ref_);
            return 1;
        }

        static int LuaLonlyGc(lua_State* l) {
            auto* ud = static_cast<UserDataBase*>(lua_touserdata(l, 1));
            ud->~UserDataBase();    // 析构对象
            return 0;
        }
    };
}

void xLuaObjBase::AddRef() {
    if (ary_index_ != -1)
        lua_->AddObjRef(ary_index_);
}

void xLuaObjBase::UnRef() {
    if (ary_index_ != -1) {
        lua_->UnRefObj(ary_index_);
        ary_index_ = -1;
    }
}

xLuaGuard::xLuaGuard(xLuaState* l) : l_(l) {
    top_ = l->GetTop();
}

xLuaGuard::xLuaGuard(xLuaState* l, int off) : l_(l) {
    top_ = l_->GetTop() + off;
}

xLuaGuard::~xLuaGuard() {
    if (l_)
        l_->SetTop(top_);
}

xLuaState::xLuaState(lua_State* l, bool attach)
    : state_(l)
    , attach_(attach) {
}

xLuaState::~xLuaState() {

}

void xLuaState::Release() {
    auto global = detail::GlobalVar::GetInstance();
    assert(global != nullptr);

    if (!attach_)
        lua_close(state_);
    else
        lua_gc(state_, LUA_GCCOLLECT, 0);
    global->Destory(this);
}

void xLuaState::GetCallStack(xLuaLogBuffer& lb) const {
    detail::TraceCallStack(state_, lb);
}

const char* xLuaState::GetTypeName(int index) {
    *type_name_buf_ = 0;
    int l_ty = GetType(index);
    const char* ret = type_name_buf_;

    if (l_ty == LUA_TLIGHTUSERDATA) {
#if XLUA_USE_LIGHT_USER_DATA
        detail::LightUserData ud = detail::MakeLightPtr(lua_touserdata(state_, index));
        if (!ud.IsLightData()) {
            snprintf(type_name_buf_, XLUA_MAX_TYPE_NAME_LENGTH, "light user data");
        } else if (ud.IsInternalType()) {
            detail::ArrayObj* obj = detail::GlobalVar::GetInstance()->GetArrayObj(ud.index_);
            if (obj == nullptr)
                snprintf(type_name_buf_, XLUA_MAX_TYPE_NAME_LENGTH, "nullptr");
            else
                snprintf(type_name_buf_, XLUA_MAX_TYPE_NAME_LENGTH, "%s*", obj->info_->type_name);
        } else {
            const auto* info = detail::GlobalVar::GetInstance()->GetExternalTypeInfo(ud.type_);
            if (info == nullptr)
                snprintf(type_name_buf_, XLUA_MAX_TYPE_NAME_LENGTH, "nullptr");
            else
                snprintf(type_name_buf_, XLUA_MAX_TYPE_NAME_LENGTH, "%s*", info->type_name);
        }
#endif // XLUA_USE_LIGHT_USER_DATA
    } else if (l_ty == LUA_TUSERDATA) {
        detail::FullUserData* ud = static_cast<detail::FullUserData*>(lua_touserdata(state_, index));
        switch (ud->type_) {
        case detail::UserDataCategory::kValue:
            ret = ud->info_->type_name;
            break;
        case detail::UserDataCategory::kRawPtr:
        case detail::UserDataCategory::kObjPtr:
            snprintf(type_name_buf_, XLUA_MAX_TYPE_NAME_LENGTH, "%s*", ud->info_->type_name);
            break;
        case detail::UserDataCategory::kSharedPtr:
            snprintf(type_name_buf_, XLUA_MAX_TYPE_NAME_LENGTH, "std::shared_ptr<%s>", ud->info_->type_name);
            break;
        default:
            assert(false);
            break;
        }
    } else {
        ret = lua_typename(state_, index);
    }
    return ret;
}

bool xLuaState::DoString(const char* buff, const char* chunk) {
    xLuaGuard guard(this);
    if (luaL_loadbuffer(state_, buff, strlen(buff), chunk) != LUA_OK) {
        const char* err = lua_tostring(state_, -1);
        detail::LogError("DoString failed chunk:%s err:%s", chunk ? chunk : "", err ? err : "");
        return false;
    }

    if (lua_pcall(state_, 0, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(state_, -1);
        detail::LogError("DoString failed chunk:%s err:%s", chunk ? chunk : "", err ? err : "");
        return false;
    }
    return true;
}

int xLuaState::LoadGlobal(const char* path) {
    if (path == nullptr || *path == 0) {
        lua_pushnil(state_);
        return LUA_TNIL;
    }

    lua_pushglobaltable(state_);
    while (const char* sub = ::strchr(path, '.')) {
        lua_pushlstring(state_, path, sub - path);
        if (lua_gettable(state_, -2) != LUA_TTABLE) {
            lua_pop(state_, 2);
            lua_pushnil(state_);
            return LUA_TNIL;
        }

        lua_remove(state_, -2);
        path = sub + 1;
    }

    int ty = lua_getfield(state_, -1, path);
    lua_remove(state_, -2);
    return ty;
}

bool xLuaState::DoSetGlobal(const char* path, bool create, bool rawset) {
    int top = GetTop();
    if (top == 0)
        return false;   // none value

    if (path == nullptr || *path == 0) {
        lua_pop(state_, 1); // pop value
        return false;   // wrong name
    }

    lua_pushglobaltable(state_);
    while (const char* sub = ::strchr(path, '.')) {
        lua_pushlstring(state_, path, sub - path);
        int l_ty = lua_gettable(state_, -2);
        if (l_ty == LUA_TNIL && create) {
            lua_pop(state_, 1);                         // pop nil
            lua_newtable(state_);                       // new table
            lua_pushlstring(state_, path, sub - path);  // key
            lua_pushvalue(state_, -2);                  // copy table
            if (rawset)
                lua_rawset(state_, -4);
            else
                lua_settable(state_, -4);               // set field
        } else if (l_ty != LUA_TTABLE) {
            lua_pop(state_, 3); // [1]:value, [2]:table, [3]:unknown value
            return false;
        }

        lua_remove(state_, -2);
        path = sub + 1;
    }

    lua_pushstring(state_, path);
    lua_rotate(state_, top, -1);
    if (rawset)
        lua_rawset(state_, -3);
    else
        lua_settable(state_, -3);   // set field

    lua_pop(state_, 1); // [1]: table
    return true;
}

int xLuaState::LoadTableField(int index, int field) {
    int ty = GetType(index);
    if (ty != LUA_TTABLE && ty != LUA_TLIGHTUSERDATA && ty != LUA_TUSERDATA) {
        lua_pushnil(state_);
        detail::LogError("attempt to get table field:[%d], target is not table", field);
        return LUA_TNIL;
    }
    return lua_geti(state_, index, field);
}

int xLuaState::LoadTableField(int index, const char* field) {
    int ty = GetType(index);
    if (ty != LUA_TTABLE && ty != LUA_TLIGHTUSERDATA && ty != LUA_TUSERDATA) {
        lua_pushnil(state_);
        detail::LogError("attempt to get table field:[%s], target is not table", field);
        return LUA_TNIL;
    }
    return lua_getfield(state_, index, field);
}

bool xLuaState::SetTableField(int index, int field) {
    int ty = GetType(index);
    if (ty != LUA_TTABLE && ty != LUA_TLIGHTUSERDATA && ty != LUA_TUSERDATA) {
        lua_pop(state_, 1);
        detail::LogError("attempt to set table field:[%d], target is not table", field);
        return false;
    }
    lua_seti(state_, index, field);
    return true;
}

bool xLuaState::SetTableField(int index, const char* field) {
    int ty = GetType(index);
    if (ty != LUA_TTABLE && ty != LUA_TLIGHTUSERDATA && ty != LUA_TUSERDATA) {
        lua_pop(state_, 1);
        detail::LogError("attempt to set table field:[%s], target is not table", field);
        return false;
    }
    lua_setfield(state_, index, field);
    return true;
}

void xLuaState::PushPtrUserData(const detail::FullUserData& ud) {
    if (ud.type_ == detail::UserDataCategory::kRawPtr) {
        void* root = detail::GetRootPtr(ud.obj_, ud.info_);
        auto it = raw_ptrs_.find(root);
        if (it != raw_ptrs_.cend()) {
            UpdateCahce(it->second, ud.obj_, ud.info_);
            PushUd(it->second);
        } else {
            UdCache udc = RefCache(NewUserData<detail::FullUserData>(ud.info_, ud));
            raw_ptrs_.insert(std::make_pair(root, udc));
        }
    } else if (ud.type_ == detail::UserDataCategory::kObjPtr) {
        if (ud.info_->is_weak_obj) {
            if ((int)weak_obj_ptrs_.size() <= ud.index_) {
                weak_obj_ptrs_.resize((1 + ud.index_ / XLUA_CONTAINER_INCREMENTAL) * XLUA_CONTAINER_INCREMENTAL, UdCache{0, nullptr});
            }

            auto& udc = lua_obj_ptrs_[ud.index_];
            if (udc.user_data_ != nullptr && udc.user_data_->serial_ == ud.serial_) {
                UpdateCahce(udc, ud.obj_, ud.info_);
                PushUd(udc);
            } else {
                udc = RefCache(NewUserData<detail::FullUserData>(ud.info_, ud));
            }
        } else {
            if ((int)lua_obj_ptrs_.size() <= ud.index_) {
                lua_obj_ptrs_.resize((1 + ud.index_ / XLUA_CONTAINER_INCREMENTAL) * XLUA_CONTAINER_INCREMENTAL, UdCache{0, nullptr});
            }

            auto& udc = lua_obj_ptrs_[ud.index_];
            if (udc.user_data_ != nullptr && udc.user_data_->serial_ == ud.serial_) {
                UpdateCahce(udc, ud.obj_, ud.info_);
                PushUd(udc);
            } else {
                udc = RefCache(NewUserData<detail::FullUserData>(ud.info_, ud));
            }
        }
    } else {
        assert(false);
    }
}

void xLuaState::Gc(detail::FullUserData* ud) {
    switch (ud->type_) {
    case detail::UserDataCategory::kValue:
        break;
    case detail::UserDataCategory::kRawPtr:
        {
            auto it = raw_ptrs_.find(detail::GetRootPtr(ud->obj_, ud->info_));
            UnRefCachce(it->second);
            raw_ptrs_.erase(it);
        }
        break;
    case detail::UserDataCategory::kSharedPtr:
        {
            auto it = shared_ptrs_.find(detail::GetRootPtr(ud->obj_, ud->info_));
            UnRefCachce(it->second);
            shared_ptrs_.erase(it);
        }
        break;
    case detail::UserDataCategory::kObjPtr:
        if (ud->info_->is_weak_obj) {
            auto& udc = weak_obj_ptrs_[ud->index_];
            if (ud == udc.user_data_)
                UnRefCachce(udc);
        } else {
            auto& udc = lua_obj_ptrs_[ud->index_];
            if (ud == udc.user_data_)
                UnRefCachce(udc);
        }
        break;
    default:
        assert(false);
        break;
    }

    ud->~FullUserData();
}

bool xLuaState::InitEnv(const char* export_module,
    const std::vector<const detail::ConstInfo*>& consts,
    const std::vector<detail::TypeInfo*>& types,
    const std::vector<const char*>& scripts) {
    // user data ref table (weak table)
    lua_createtable(state_, XLUA_CONTAINER_INCREMENTAL, 0);
    lua_createtable(state_, 0, 1);
    lua_pushfstring(state_, "v");
    lua_setfield(state_, 2, "__mode");
    lua_setmetatable(state_, -2);
    user_data_table_ref_ = luaL_ref(state_, LUA_REGISTRYINDEX);
    assert(user_data_table_ref_);
    assert(GetTop() == 0);

    // table of type metatable
    lua_createtable(state_, (int)types.size(), 0);
    meta_table_ref_ = luaL_ref(state_, LUA_REGISTRYINDEX);
    assert(meta_table_ref_);

    // user data cache table
    lua_newtable(state_);
    lua_obj_table_ref_ = luaL_ref(state_, LUA_REGISTRYINDEX);
    assert(lua_obj_table_ref_);

    // xlua table
    lua_newtable(state_);
    PushClosure(&detail::MetaFuncs::LuaCast);
    lua_setfield(state_, -2, "Cast");
    PushClosure(&detail::MetaFuncs::LuaIsValid);
    lua_setfield(state_, -2, "IsValid");
    PushClosure(&detail::MetaFuncs::LuaType);
    lua_setfield(state_, -2, "Type");
    PushClosure(&detail::MetaFuncs::LuaGetTypeMeta);
    lua_setfield(state_, -2, "GetTypeMeta");
    PushClosure(&detail::MetaFuncs::LuaGetMetaList);
    lua_setfield(state_, -2, "GetMetaList");
    lua_setglobal(state_, "xlua");

#if XLUA_USE_LIGHT_USER_DATA
    // light user data metatable
    lua_pushlightuserdata(state_, nullptr);  //修改lightuserdata的metatable
    lua_createtable(state_, 0, 3);

    lua_pushstring(state_, "light_user_data_metatable");
    lua_setfield(state_, -2, "__name");

    PushClosure(&detail::MetaFuncs::LuaLightPtrIndex);
    lua_setfield(state_, -2, "__index");

    PushClosure(&detail::MetaFuncs::LuaLightPtrNewIndex);
    lua_setfield(state_, -2, "__newindex");

    lua_setmetatable(state_, -2);   // set light user data metatable
    lua_pop(state_, 1);             // light user data
    assert(GetTop() == 0);
#endif // XLUA_USE_LIGHT_USER_DATA

    lua_createtable(state_, 0, 2);
    lua_pushstring(state_, "lonly_user_data");
    lua_setfield(state_, -2, "__name");
    lua_pushcfunction(state_, &detail::MetaFuncs::LuaLonlyGc);
    lua_setfield(state_, -2, "__gc");
    lonly_ud_meta_ref_ = luaL_ref(state_, LUA_REGISTRYINDEX);

    InitConsts(export_module, consts);

    for (const detail::TypeInfo* info : types) {
        if (info->category != detail::TypeCategory::kGlobal)
            CreateTypeMeta(info);
        if (detail::HasGlobal(info))
            CreateTypeGlobal(export_module, info);
    }

    for (const char* script : scripts) {
        DoString(script, "InitEnv");
    }

    return true;
}

void xLuaState::InitConsts(const char* export_module, const std::vector<const detail::ConstInfo*>& consts) {
    lua_createtable(state_, 0, 2);  // const metatable
    lua_pushcfunction(state_, &detail::MetaFuncs::LuaConstIndex);
    lua_setfield(state_, -2, "__index");
    lua_pushcfunction(state_, &detail::MetaFuncs::LuaConstNewIndex);
    lua_setfield(state_, -2, "__newindex");
    lua_pushstring(state_, "const_meta_table");
    lua_setfield(state_, -2, "__name");

    for (const detail::ConstInfo* info : consts) {
        char type_name[XLUA_MAX_TYPE_NAME_LENGTH];
        if (*info->name == 0 || ::strcmp(info->name, "_G") == 0) {
            snprintf(type_name, XLUA_MAX_TYPE_NAME_LENGTH, export_module);
        } else if (export_module == nullptr || *export_module == 0) {
            snprintf(type_name, XLUA_MAX_TYPE_NAME_LENGTH, info->name);
        } else {
            snprintf(type_name, XLUA_MAX_TYPE_NAME_LENGTH, "%s.%s", export_module, info->name);
        }

        if (*type_name == 0) {
            // global table
            lua_pushglobaltable(state_);
        } else {
            int num = 0;
            while (info->values[num].name)
                ++num;

            lua_createtable(state_, 0, num + 1);
            lua_pushvalue(state_, -1);  // copy ref
            bool ok = DoSetGlobal(type_name, true, true);
            assert(ok);
        }

        for (int i = 0; info->values[i].name; ++i) {
            const auto& val = info->values[i];
            switch (val.category) {
            case detail::ConstCategory::kInteger:
                lua_pushnumber(state_, val.int_val);
                break;
            case detail::ConstCategory::kFloat:
                lua_pushnumber(state_, val.float_val);
                break;
            case detail::ConstCategory::kString:
                lua_pushstring(state_, val.string_val);
                break;
            default:
                assert(false);
                break;
            }

            lua_setfield(state_, -2, val.name);
        }

        if (*type_name != 0) {
            lua_pushstring(state_, info->name);
            lua_setfield(state_, -2, "__name");

            lua_pushvalue(state_, -2);      // copy meta table to top
            lua_setmetatable(state_, -2);   // set metatable
        }

        lua_pop(state_, 1); // pop table
    }

    lua_pop(state_, 1); // pop const meta table
    assert(GetTop() == 0);
}

void xLuaState::SetTypeMember(const detail::TypeInfo* info) {
    const auto* super = info->super;
    while (super) {
        SetTypeMember(super);
        super = super->super;
    }

    for (const auto* mem = info->funcs; mem->name; ++mem) {
        PushClosure(mem->func);
        lua_setfield(state_, -2, mem->name);
    }

    for (const auto* mem = info->vars; mem->name; ++mem) {
        lua_pushlightuserdata(state_, const_cast<detail::MemberVar*>(mem));
        lua_setfield(state_, -2, mem->name);
    }
}

void xLuaState::SetGlobalMember(const detail::TypeInfo* info, bool func, bool var) {
    const auto* super = info->super;
    while (super) {
        SetGlobalMember(super, func, var);
        super = super->super;
    }

    if (func) {
        for (const auto* mem = info->global_funcs; mem->name; ++mem) {
            PushClosure(mem->func);
            lua_setfield(state_, -2, mem->name);
        }
    }

    if (var) {
        for (const auto* mem = info->global_vars; mem->name; ++mem) {
            lua_pushlightuserdata(state_, const_cast<detail::MemberVar*>(mem));
            lua_setfield(state_, -2, mem->name);
        }
    }
}

void xLuaState::CreateTypeMeta(const detail::TypeInfo* info) {
    lua_rawgeti(state_, LUA_REGISTRYINDEX, meta_table_ref_);
    lua_newtable(state_);
    lua_pushvalue(state_, -1);
    lua_rawseti(state_, -3, info->index);
    lua_remove(state_, -2);

    SetTypeMember(info);

    lua_pushstring(state_, info->type_name);
    lua_setfield(state_, -2, "__name");

    PushClosure(&detail::MetaFuncs::LuaIndex);
    lua_setfield(state_, -2, "__index");

    PushClosure(&detail::MetaFuncs::LuaNewIndex);
    lua_setfield(state_, -2, "__newindex");

    PushClosure(&detail::MetaFuncs::LuaGc);
    lua_setfield(state_, -2, "__gc");

    lua_pop(state_, 1);
    assert(GetTop() == 0);
}

void xLuaState::CreateTypeGlobal(const char* export_module, const detail::TypeInfo* info) {
    char type_name[XLUA_MAX_TYPE_NAME_LENGTH];
    if (*info->type_name == 0 || ::strcmp(info->type_name, "_G") == 0) {
        snprintf(type_name, XLUA_MAX_TYPE_NAME_LENGTH, export_module);
    } else if (export_module == nullptr || *export_module == 0) {
        snprintf(type_name, XLUA_MAX_TYPE_NAME_LENGTH, info->type_name);
    } else {
        snprintf(type_name, XLUA_MAX_TYPE_NAME_LENGTH, "%s.%s", export_module, info->type_name);
    }

    if (*type_name == 0) {
        lua_pushglobaltable(state_);
        SetGlobalMember(info, true, false);

        int var_num = 0;
        auto node = info;
        while (node) {
            for (int i = 0; node->global_vars[i].name; ++i)
                ++var_num;
            node = node->super;
        }

        if (var_num)
            detail::LogError("global table can not add member var");
        assert(var_num == 0 && "global table can not add member var");
    } else {
        lua_newtable(state_);
        lua_pushvalue(state_, -1);  // copy ref
        bool ok = DoSetGlobal(type_name, true, true);
        assert(ok);

        lua_pushstring(state_, info->type_name);
        lua_setfield(state_, -2, "__name");

        SetGlobalMember(info, true, false);

        // meta table
        lua_createtable(state_, 0, 0);
        SetGlobalMember(info, false, true);

        lua_pushstring(state_, info->type_name);
        lua_setfield(state_, -2, "__name");

        PushClosure(&detail::MetaFuncs::LuaGlobalIndex);
        lua_setfield(state_, -2, "__index");

        PushClosure(&detail::MetaFuncs::LuaGlobalNewIndex);
        lua_setfield(state_, -2, "__newindex");

        lua_setmetatable(state_, -2);   // set metatable
    }

    lua_pop(state_, 1);
    assert(GetTop() == 0);
}

void xLuaState::PushClosure(lua_CFunction func) {
    lua_pushlightuserdata(state_, this);    // upvalue(1):xLuaSate*
    lua_pushcclosure(state_, func, 1);
}

int xLuaState::RefLuaObj(int index) {
    if (next_free_lua_obj_ == -1) {
        size_t o_s = lua_objs_.size();
        size_t n_s = o_s + XLUA_CONTAINER_INCREMENTAL;
        lua_objs_.resize(n_s, LuaObjRef{-1, 0});

        for (size_t i = o_s; i < n_s; ++i)
            lua_objs_[i].next_free_ = (int)(i + 1);
        next_free_lua_obj_ = (int)o_s;
        lua_objs_.back().next_free_ = -1;
    }

    int ary_index = next_free_lua_obj_;
    auto& ref = lua_objs_[ary_index];
    next_free_lua_obj_ = ref.next_free_;

    index = lua_absindex(state_, index);
    lua_rawgeti(state_, LUA_REGISTRYINDEX, lua_obj_table_ref_);
    lua_rotate(state_, index, -1);
    ref.lua_ref_ = luaL_ref(state_, -2);
    ref.ref_count_ = 1;
    lua_pop(state_, 1);

    return ary_index;
}

void xLuaState::PushLuaObj(int ary_index) {
    assert(ary_index >= 0 && ary_index < (int)lua_objs_.size());

    lua_rawgeti(state_, LUA_REGISTRYINDEX, lua_obj_table_ref_);
    lua_rawgeti(state_, -1, lua_objs_[ary_index].lua_ref_);
    lua_remove(state_, -2);
}

void xLuaState::AddObjRef(int ary_index) {
    assert(ary_index >= 0 && ary_index < (int)lua_objs_.size());
    ++lua_objs_[ary_index].ref_count_;
}

void xLuaState::UnRefObj(int ary_index) {
    assert(ary_index >= 0 && ary_index < (int)lua_objs_.size());
    auto& ref = lua_objs_[ary_index];
    -- ref.ref_count_;
    if (ref.ref_count_ == 0) {
        lua_rawgeti(state_, LUA_REGISTRYINDEX, lua_obj_table_ref_);
        luaL_unref(state_, -1, ref.lua_ref_);
        lua_pop(state_, 1);

        ref.next_free_ = next_free_lua_obj_;
        next_free_lua_obj_ = ary_index;
    }
}

XLUA_NAMESPACE_END
