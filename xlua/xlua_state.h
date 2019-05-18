#pragma once
#include "detail/traits.h"
#include "detail/state.h"

XLUA_NAMESPACE_BEGIN

namespace detail {
    struct MetaFuncs;

    template <typename Ty> void Push(xLuaState* l, const Ty& val);
    template <typename Ty> Ty Load(xLuaState* l, int index);
}

/* lua table*/
class xLuaTable : private detail::ObjBase {
public:
    xLuaTable() = default;
    xLuaTable(detail::State* s, int index) : detail::ObjBase(s, index) {}

    xLuaTable(const xLuaTable&) = default;
    xLuaTable(xLuaTable&&) = default;

public:
    xLuaTable& operator = (const xLuaTable&) = default;
    xLuaTable& operator = (xLuaTable&&) = default;

public:
    inline bool IsValid() const { return detail::ObjBase::IsValid(); }
    inline operator bool() const { return IsValid(); }
};

/* lua function */
class xLuaFunction : private detail::ObjBase {
public:
    xLuaFunction() = default;
    xLuaFunction(detail::State* s, int index) : detail::ObjBase(s, index) {}

    xLuaFunction(const xLuaFunction&) = default;
    xLuaFunction(xLuaFunction&&) = default;

public:
    xLuaFunction& operator = (const xLuaFunction&) = default;
    xLuaFunction& operator = (xLuaFunction&&) = default;

public:
    inline bool IsValid() const { return detail::ObjBase::IsValid(); }
    inline operator bool() const { return IsValid(); }
};

/* lua堆栈守卫 */
class xLuaGuard {
protected:
    xLuaGuard() { }

public:
    xLuaGuard(xLuaState* l);
    xLuaGuard(xLuaState* l, int off);
    ~xLuaGuard();

public:
    xLuaGuard(const xLuaGuard&) = delete;
    xLuaGuard& operator = (const xLuaGuard&) = delete;

protected:
    int top_;
    xLuaState* l_;
};

/* lua函数调用堆栈保护
 * lua函数执行以后返回值存放在栈上, 如果清空栈可能会触发GC
 * 导致引用的局部变量失效造成未定义行为
*/
class xLuaCallGuard : private xLuaGuard {
    friend class xLuaState;
public:
    xLuaCallGuard(xLuaState* l) : xLuaGuard(l) { }
    xLuaCallGuard(xLuaState* l, int off) : xLuaGuard(l, off) { }
    xLuaCallGuard(xLuaCallGuard&& other) : xLuaGuard() {
        top_ = other.top_;
        l_ = other.l_;
        ok_ = other.ok_;
        other.l_ = nullptr;
    }

public:
    explicit operator bool() const { return ok_; }

private:
    bool ok_ = false;
};

/* lua 状态机扩展 */
class xLuaState : private detail::State {
    friend struct detail::MetaFuncs;
    friend class detail::GlobalVar;

private:
    xLuaState(lua_State* l, bool attach);
    ~xLuaState();

public:
    /* 销毁 */
    void Release();

public:
    inline lua_State* GetState() const { return state_; }

    /* lua栈操作 */
    inline int GetTop() const { return lua_gettop(state_); }
    inline void SetTop(int top) { lua_settop(state_, top); }
    inline void PopTop(int num) { lua_pop(state_, num); }

    inline int GetType(int index) const { return lua_type(state_, index); }
    const char* GetTypeName(int index);

    /* 获取调用栈 */
    void GetCallStack(xLuaLogBuffer& lb) const;
    inline void GetCallStack(char* buf, size_t size) const {
        xLuaLogBuffer lb(buf, size);
        GetCallStack(lb);
    }

    /* 执行一段字符串 */
    bool DoString(const char* buff, const char* chunk = nullptr);

    inline void NewTable() {
        lua_newtable(state_);
    }

    /* 新建lua table */
    inline xLuaTable CreateTable() {
        xLuaGuard guard(this);
        lua_newtable(state_);
        return xLuaTable(this, RefLuaObj(-1));
    }

    /* 将全局变量加载到栈顶, 并返回lua type */
    int LoadGlobal(const char* path);

    /* 栈顶元素设置为全局变量 */
    inline bool SetGlobal(const char* path, bool create = false) {
        return DoSetGlobal(path, create, false);
    }

    /* 将table成员加载到栈顶并返回数据类型 */
    int LoadTableField(int index, int field);
    int LoadTableField(int index, const char* field);

    /* 将栈顶元素设置为table成员 */
    bool SetTableField(int index, int field);
    bool SetTableField(int index, const char* field);

    template <typename Ty>
    inline Ty GetGlobalVar(const char* path) {
        xLuaGuard guard(this);
        LoadGlobal(path);
        return Load<Ty>(-1);
    }

    inline bool SetGlobalVar(const char* path, std::nullptr_t) {
        Push(nullptr);
        DoSetGlobal(path, false, false);
        return true;
    }

    template <typename Ty>
    inline bool SetGlobalVar(const char* path, const Ty& val, bool caeate = false) {
        Push(val);
        return DoSetGlobal(path, caeate, false);
    }

    template <typename Ty>
    inline bool SetTableField(int index, int field, const Ty& val) {
        index = lua_absindex(state_, index);
        Push(val);
        return SetTableField(index, field);
    }

    template <typename Ty>
    inline Ty GetTableField(int index, int field) {
        xLuaGuard guard(this);
        LoadTableField(index, field);
        return Load<Ty>(-1);
    }

    template <typename Ty>
    inline bool SetTableField(int index, const char* field, const Ty& val) {
        index = lua_absindex(state_, index);
        Push(val);
        return SetTableField(index, field);
    }

    template <typename Ty>
    inline Ty GetTableField(int index, const char* field) {
        xLuaGuard guard(this);
        LoadTableField(index, field);
        return Load<Ty>(-1);
    }

    template <typename Ty>
    inline Ty GetTableField(const xLuaTable& table, int field) {
        xLuaGuard guard(this);
        Push(table);
        LoadTableField(-1, field);
        return Load<Ty>(-1);
    }

    template <typename Ty>
    inline Ty GetTableField(const xLuaTable& table, const char* field) {
        xLuaGuard guard(this);
        Push(table);
        LoadTableField(-1, field);
        return Load<Ty>(-1);
    }

    template <typename Ty>
    inline bool SetTableField(const xLuaTable& table, int field, const Ty& val) {
        xLuaGuard guard(this);
        Push(table);
        Push(val);
        return SetTableField(-2, field);
    }

    template <typename Ty>
    inline bool SetTableField(const xLuaTable& table, const char* field, const Ty& val) {
        xLuaGuard guard(this);
        Push(table);
        Push(val);
        return SetTableField(-2, field);
    }

    template <typename Ty>
    inline void Push(const Ty& val) {
        detail::Push(this, val);
    }

    template <typename Ty>
    inline Ty Load(int index) {
        return detail::Load<Ty>(this, index);
    }

    template<typename... Ty>
    inline void PushMul(Ty&&... vals) {
        using ints = int[];
        (void)ints {
            0, (Push(std::forward<Ty>(vals)), 0)...
        };
    }

    template <typename... Ty>
    inline void LoadMul(int index, std::tuple<Ty&...> tp) {
        DoLoadMul(index, tp, detail::make_index_sequence_t<sizeof...(Ty)>());
    }

    /* 调用栈顶的Lua函数 */
    template<typename... Rys, typename... Args>
    inline xLuaCallGuard Call(std::tuple<Rys&...> ret, Args&&... args) {
        xLuaCallGuard guard(this, -1);
        if (GetType(-1) != LUA_TFUNCTION) {
            detail::LogError("attempt to call is not a function");
        } else {
            guard.ok_ = DoCall(ret, std::forward<Args>(args)...);
        }
        return guard;
    }

    /* 调用全局Lua函数 */
    template <typename... Rys, typename... Args>
    inline xLuaCallGuard Call(const char* global, std::tuple<Rys&...> ret, Args&&... args) {
        xLuaCallGuard guard(this);
        if (LoadGlobal(global) != LUA_TFUNCTION) {
            detail::LogError("global var:[%s] is not a function", global);
        } else {
            guard.ok_ = DoCall(ret, std::forward<Args>(args)...);
        }
        return guard;
    }

    /* 调用指定Lua函数 */
    template <typename... Rys, typename... Args>
    inline xLuaCallGuard Call(const xLuaFunction& func, std::tuple<Rys&...> ret, Args&&... args) {
        xLuaCallGuard guard(this);
        Push(func);
        if (GetType(-1) != LUA_TFUNCTION) {
            detail::LogError("attempt to call is not a function");
        } else {
            guard.ok_ = DoCall(ret, std::forward<Args>(args)...);
        }
        return guard;
    }

    /* table call, table函数是冒号调用的 table:Call(xxx) */
    template <typename... Rys, typename... Args>
    inline xLuaCallGuard Call(const xLuaTable& table, const char* func, std::tuple<Rys&...> ret, Args&&... args) {
        xLuaCallGuard guard(this);
        Push(table);
        if (LoadTableField(-1, func) != LUA_TFUNCTION) {
            detail::LogError("can not get table function:[%s]", func);
        } else {
            guard.ok_ = DoCall(ret, table, std::forward<Args>(args)...);
        }
        return guard;
    }

private:
    bool InitEnv(const char* export_module,
        const std::vector<const detail::ConstInfo*>& consts,
        const std::vector<detail::TypeInfo*>& types,
        const std::vector<detail::ScriptInfo>& scripts
    );
    void InitConsts(const char* export_module, const std::vector<const detail::ConstInfo*>& consts);
    void CreateTypeMeta(const detail::TypeInfo* info);
    void CreateTypeGlobal(const char* export_module, const detail::TypeInfo* info);
    void SetTypeMember(const detail::TypeInfo* info);
    void SetGlobalMember(const detail::TypeInfo* info, bool func, bool var);
    void PushClosure(lua_CFunction func);

    bool DoSetGlobal(const char* path, bool create, bool rawset);

    template<typename... Rys, typename... Args>
    inline bool DoCall(std::tuple<Rys&...>& ret, Args&&... args) {
        int top = GetTop();
        PushMul(std::forward<Args>(args)...);
        if (lua_pcall(GetState(), sizeof...(Args), sizeof...(Rys), 0) != LUA_OK) {
            detail::LogError("excute lua function error! %s", lua_tostring(GetState(), -1));
            return false;
        }
        LoadMul(top, ret);
        return true;
    }

    template <typename... Ty, size_t... Idxs>
    inline void DoLoadMul(int index, std::tuple<Ty&...>& ret, detail::index_sequence<Idxs...>) {
        using ints = int[];
        (void)ints { 0, (std::get<Idxs>(ret) = Load<Ty>(index++), 0)... };
    }
};

namespace detail {
    template <typename Fy, typename Ry, typename... Args>
    int DoFunction(Fy f, xLuaState* l);

    inline State* RawState(xLuaState* l) { return (State*)l; }
    inline ObjBase* RawObj(xLuaTable* t) { return (ObjBase*)t; }
    inline ObjBase* RawObj(xLuaFunction* f) { return (ObjBase*)f; }
    inline const ObjBase* RawObj(const xLuaTable* t) { return (const ObjBase*)t; }
    inline const ObjBase* RawObj(const xLuaFunction* f) { return (const ObjBase*)f; }

    namespace pusher {
        /* push operator */
        template <typename Ty>
        struct Pusher {
            typedef typename std::remove_cv<Ty>::type value_type;
            static_assert(IsLuaType<value_type>::value
                || IsExtendPush<value_type>::value
                || std::is_enum<value_type>::value,
                "only type has declared export to lua accept");

            static inline void Do(xLuaState* l, const Ty& val) {
                using tag = typename std::conditional<IsLuaType<value_type>::value, tag_declared,
                    typename std::conditional<std::is_enum<value_type>::value, tag_enum, tag_extend>::type>::type;
                DoPush<Ty>(l, val, tag());
            }

            template <typename U>
            static inline void DoPush(xLuaState* l, const U& val, tag_extend) {
                xLuaPush(l, val);
            }

            template <typename U>
            static inline void DoPush(xLuaState* l, const U& val, tag_enum) {
                lua_pushnumber(l->GetState(), static_cast<int>(val));
            }

            template <typename U>
            static inline void DoPush(xLuaState* l, const U& val, tag_declared) {
                RawState(l)->PushValueData(val, GetTypeInfoImpl<U>());
            }
        };

        template <typename Ty>
        struct Pusher<Ty*> {
            typedef typename std::remove_cv<Ty>::type value_type;
            static_assert(!std::is_const<Ty>::value, "can not push const value pointer");
            static_assert(!IsExtendPush<value_type>::value, "can not push extend type pointer");
            static_assert(IsLuaType<value_type>::value,
                "only type has declared export to lua accept");

            static inline void Do(xLuaState* l, Ty* val) {
                if (val == nullptr) {
                    lua_pushnil(l->GetState());
                } else {
                    const TypeInfo* info = GetTypeInfoImpl<value_type>();
#if XLUA_ENABLE_LUD_OPTIMIZE
                    auto ud = MakeLightUserData(val, info);
                    lua_pushlightuserdata(l->GetState(), ud.Ptr());
#else // XLUA_ENABLE_LUD_OPTIMIZE
                    RawState(l)->PushPtrUserData(MakeFullUserData(val, info));
#endif // XLUA_ENABLE_LUD_OPTIMIZE
                }
            }
        };

        template <typename Ty>
        struct Pusher<std::shared_ptr<Ty>> {
            typedef typename std::remove_cv<Ty>::type value_type;
            static_assert(IsLuaType<value_type>::value, "only type has declared export to lua accept");

            static inline void Do(xLuaState* l, const std::shared_ptr<Ty>& ptr) {
                if (!ptr) {
                    lua_pushnil(l->GetState());
                } else {
                    RawState(l)->PushSharedPtr(ptr, GetTypeInfoImpl<value_type>());
                }
            }
        };

        template <typename Ty>
        struct Pusher<xLuaWeakObjPtr<Ty>> {
            typedef typename std::remove_cv<Ty>::type value_type;
            static_assert(IsLuaType<value_type>::value, "only type has declared export to lua accept");

            static inline void Do(xLuaState* l, const xLuaWeakObjPtr<Ty>& obj) {
                Ty* ptr = ::xLuaGetPtrByWeakObj(obj);
                if (ptr == nullptr) {
                    lua_pushnil(l->GetState());
                } else {
                    Push(l, ptr);
                }
            }
        };

        template <typename Ty>
        void PushValue(xLuaState* l, const Ty& val);

        inline void PushValue(xLuaState* l, bool val) { lua_pushboolean(l->GetState(), val); }
        inline void PushValue(xLuaState* l, char val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, unsigned char val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, short val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, unsigned short val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, int val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, unsigned int val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, long val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, unsigned long val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, long long val) { lua_pushinteger(l->GetState(), val); }
        inline void PushValue(xLuaState* l, unsigned long long val) { lua_pushinteger(l->GetState(), val); }
        inline void PushValue(xLuaState* l, float val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, double val) { lua_pushnumber(l->GetState(), val); }
        inline void PushValue(xLuaState* l, char* val) { lua_pushstring(l->GetState(), val); }
        inline void PushValue(xLuaState* l, const char* val) { lua_pushstring(l->GetState(), val); }
        inline void PushValue(xLuaState* l, const std::string& val) { lua_pushstring(l->GetState(), val.c_str()); }
        inline void PushValue(xLuaState* l, std::nullptr_t) { lua_pushnil(l->GetState()); }

        inline void PushValue(xLuaState* l, void* val) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            assert(detail::IsValidRawPtr(val));
#endif
            if (val == nullptr)
                lua_pushnil(l->GetState());
            else
                lua_pushlightuserdata(l->GetState(), val);
        }

        inline void PushValue(xLuaState* l, const void* val) {
            PushValue(l, const_cast<void*>(val));
        }

        inline void PushValue(xLuaState* l, lua_CFunction func) {
            lua_pushcfunction(l->GetState(), func);
        }

        inline void PushValue(xLuaState* l, int(*func)(xLuaState* l)) {
            if (func == nullptr) {
                PushValue(l, nullptr);
                return;
            }

            static lua_CFunction f = [](lua_State* l) -> int {
                auto* xl = (xLuaState*)lua_touserdata(l, lua_upvalueindex(1));
                void* f = lua_touserdata(l, lua_upvalueindex(2));
                return reinterpret_cast<int(*)(xLuaState*)>(f)(static_cast<xLuaState*>(xl));
            };

            lua_pushlightuserdata(l->GetState(), l);
            lua_pushlightuserdata(l->GetState(), reinterpret_cast<void*>(func));
            lua_pushcclosure(l->GetState(), f, 2);
        }

        template <typename Ry, typename... Args>
        void PushValue(xLuaState* l, Ry(*func)(Args...)) {
            if (func == nullptr) {
                PushValue(l, nullptr);
                return;
            }

            static lua_CFunction f = [](lua_State* l) -> int {
                auto* xl = (xLuaState*)lua_touserdata(l, lua_upvalueindex(1));
                void* f = lua_touserdata(l, lua_upvalueindex(2));
                return DoFunction<Ry(*)(Args...), Ry, Args...>(reinterpret_cast<Ry(*)(Args...)>(f), xl/*, std::is_same<Ry, void>()*/);
            };

            lua_pushlightuserdata(l->GetState(), l);
            lua_pushlightuserdata(l->GetState(), reinterpret_cast<void*>(func));
            lua_pushcclosure(l->GetState(), f, 2);
        }

        template <typename Ry, typename... Args>
        void PushValue(xLuaState* l, const std::function<Ry(Args...)>& func) {
            typedef std::function<Ry(Args...)> func_type;
            typedef LonelyUserData<func_type> ud_type;
            if (!func) {
                PushValue(l, nullptr);
                return;
            }

            static lua_CFunction f = [](lua_State* l) -> int {
                auto* xl = (xLuaState*)lua_touserdata(l, lua_upvalueindex(1));
                auto* ud = (ud_type*)lua_touserdata(l, lua_upvalueindex(2));
                return DoFunction<func_type&, Ry, Args...>(
                    *static_cast<func_type*>(ud->GetDataPtr()), xl);
            };

            lua_pushlightuserdata(l->GetState(), l);
            RawState(l)->NewLonlyData(func);
            lua_pushcclosure(l->GetState(), f, 2);
        }

        template <typename Ty, typename Alloc = std::allocator<Ty>>
        void PushValue(xLuaState* l, const std::vector<Ty, Alloc>& vec) {
            l->NewTable();
            int i = 0;
            for (const auto& v : vec)
                l->SetTableField(-1, ++i, v);
        }

        template <typename Ty, typename Alloc = std::allocator<Ty>>
        void PushValue(xLuaState* l, const std::list<Ty, Alloc>& lst) {
            l->NewTable();
            int i = 0;
            for (const auto& v : lst)
                l->SetTableField(-1, ++i, v);
        }

        template <typename Ky, typename Ty, typename Alloc = std::allocator<Ty>>
        void PushValue(xLuaState* l, const std::map<Ky, Ty, Alloc>& map) {
            l->NewTable();
            for (const auto& pair : map)
                l->SetTableField(-1, pair.first, pair.second);
        }

        inline void PushLuaObj(State* s, const ObjBase* obj) {
            if (s != obj->state_)
                lua_pushnil(s->state_);
            else
                s->LoadLuaObj(obj->ary_index_);
        }

        inline void PushValue(xLuaState* l, const xLuaTable& val) {
            PushLuaObj(RawState(l), RawObj(&val));
        }

        inline void PushValue(xLuaState* l, const xLuaFunction& val) {
            PushLuaObj(RawState(l), RawObj(&val));
        }

        template <typename Ty>
        inline void PushValue(xLuaState* l, const Ty& val) {
            Pusher<typename std::decay<Ty>::type>::Do(l, val);
        }
    } // namespace pusher

    template <typename Ty> void Push(xLuaState* l, const Ty& val) {
        pusher::PushValue(l, val);
    }

    namespace loader {
        template <typename Ty>
        struct Loader {
            typedef typename std::remove_cv<Ty>::type value_type;
            static_assert(IsLuaType<value_type>::value
                || IsExtendLoad<value_type>::value
                || std::is_enum<value_type>::value, "only type has declared export to lua accept");

            static inline Ty Do(xLuaState* l, int index) {
                using tag = typename std::conditional<IsLuaType<value_type>::value, tag_declared,
                    typename std::conditional<std::is_enum<value_type>::value, tag_enum, tag_extend>::type>::type;
                return DoLoad<value_type>(l, index, tag());
            }

            template <typename U>
            static inline U DoLoad(xLuaState* l, int index, tag_extend) {
                return xLuaLoad(l, index, Identity<U>());
            }

            template <typename U>
            static inline U DoLoad(xLuaState* l, int index, tag_enum) {
                return (U)static_cast<int>(lua_tonumber(l->GetState(), index));
            }

            template <typename U>
            static inline U DoLoad(xLuaState* l, int index, tag_declared) {
                const TypeInfo* info = GetTypeInfoImpl<U>();
                UserDataInfo ud_info;
                if (!GetUserDataInfo(l->GetState(), index, &ud_info) || ud_info.obj == nullptr) {
                    LogError("can not load [%s] value, target is nil", info->type_name);
                    return U();
                }

                if (!IsBaseOf(info, ud_info.info)) {
                    LogError("can not load [%s] value from:[%s]", info->type_name, ud_info.info->type_name);
                    return U();
                }

                return U(*static_cast<U*>(_XLUA_TO_SUPER_PTR(info, ud_info.obj, ud_info.info)));
            }
        };

        template <typename Ty>
        struct Loader<Ty*> {
            typedef typename std::remove_cv<Ty>::type value_type;
            static_assert(!std::is_same<Ty, char>::value, "can not load value as char*, only const char* accept");
            static_assert(!IsExtendLoad<value_type>::value, "can not load extend type pointer");
            static_assert(IsLuaType<value_type>::value, "only type has declared export to lua accept");

            static Ty* Do(xLuaState* l, int index) {
                UserDataInfo ud_info;
                if (!GetUserDataInfo(l->GetState(), index, &ud_info) || ud_info.obj == nullptr)
                    return nullptr;

                const TypeInfo* info = GetTypeInfoImpl<value_type>();
                if (!IsBaseOf(info, ud_info.info)) {
                    LogError("can not load [%s*] pointer from:[%s*]", info->type_name, ud_info.info->type_name);
                    return nullptr;
                }

                return static_cast<Ty*>(_XLUA_TO_SUPER_PTR(info, ud_info.obj, ud_info.info));
            }
        };

        template <typename Ty>
        struct Loader<std::shared_ptr<Ty>> {
            typedef typename std::remove_cv<Ty>::type value_type;
            static_assert(IsLuaType<value_type>::value, "only type has declared export to lua accept");

            static inline std::shared_ptr<Ty> Do(xLuaState* l, int index) {
                if (lua_type(l->GetState(), index) != LUA_TUSERDATA)
                    return nullptr;
                auto* ud = static_cast<FullUserData*>(lua_touserdata(l->GetState(), index));
                if (ud == nullptr || ud->type_ != UserDataCategory::kSharedPtr)
                    return nullptr;
                const TypeInfo* info = GetTypeInfoImpl<value_type>();
                if (!IsBaseOf(info, ud->info_)) {
                    LogError("can not load [%s*] pointer from:[%s*]", info->type_name, ud->info_->type_name);
                    return nullptr;
                }

                return std::shared_ptr<Ty>(*static_cast<std::shared_ptr<Ty>*>(ud->GetDataPtr()),
                    (Ty*)_XLUA_TO_SUPER_PTR(info, ud->obj_, ud->info_));
            }
        };

        template <typename Ty>
        struct Loader<xLuaWeakObjPtr<Ty>> {
            static_assert(IsLuaType<Ty>::value, "only type has declared export to lua accept");

            static inline xLuaWeakObjPtr<Ty> Do(xLuaState* l, int index) {
                return xLuaWeakObjPtr<Ty>(l->Load<Ty*>(index));
            }
        };

        template <typename Ty>
        Ty LoadValue(xLuaState* l, int index);

        template<> inline bool LoadValue<bool>(xLuaState* l, int index) { return lua_toboolean(l->GetState(), index); }
        template<> inline char LoadValue<char>(xLuaState* l, int index) { return (char)lua_tonumber(l->GetState(), index); }
        template<> inline unsigned char LoadValue<unsigned char>(xLuaState* l, int index) { return (unsigned char)lua_tonumber(l->GetState(), index); }
        template<> inline short LoadValue<short>(xLuaState* l, int index) { return (short)lua_tonumber(l->GetState(), index); }
        template<> inline unsigned short LoadValue<unsigned short>(xLuaState* l, int index) { return (unsigned short)lua_tonumber(l->GetState(), index); }
        template<> inline int LoadValue<int>(xLuaState* l, int index) { return (int)lua_tonumber(l->GetState(), index); }
        template<> inline unsigned int LoadValue<unsigned int>(xLuaState* l, int index) { return (int)lua_tonumber(l->GetState(), index); }
        template<> inline long LoadValue<long>(xLuaState* l, int index) { return (long)lua_tonumber(l->GetState(), index); }
        template<> inline unsigned long LoadValue<unsigned long>(xLuaState* l, int index) { return (unsigned long)lua_tonumber(l->GetState(), index); }
        template<> inline long long LoadValue<long long>(xLuaState* l, int index) { return (long long)lua_tointeger(l->GetState(), index); }
        template<> inline unsigned long long LoadValue<unsigned long long>(xLuaState* l, int index) { return (unsigned long long)lua_tointeger(l->GetState(), index); }
        template<> inline float LoadValue<float>(xLuaState* l, int index) { return (float)lua_tonumber(l->GetState(), index); }
        template<> inline double LoadValue<double>(xLuaState* l, int index) { return (double)lua_tonumber(l->GetState(), index); }
        template<> inline const char* LoadValue<const char*>(xLuaState* l, int index) { return lua_tostring(l->GetState(), index); }
        template<> inline void* LoadValue<void*>(xLuaState* l, int index) { return lua_touserdata(l->GetState(), index); }
        template<> inline const void* LoadValue<const void*>(xLuaState* l, int index) { return lua_touserdata(l->GetState(), index); }
        template<> inline std::string LoadValue<std::string>(xLuaState* l, int index) { return std::string(lua_tostring(l->GetState(), index)); }
        template<> inline lua_CFunction LoadValue<lua_CFunction>(xLuaState* l, int index) { return lua_tocfunction(l->GetState(), index); }

        template<> inline xLuaTable LoadValue<xLuaTable>(xLuaState* l, int index) {
            if (lua_type(l->GetState(), index) == LUA_TTABLE)
                return xLuaTable(RawState(l), RawState(l)->RefLuaObj(index));
            return xLuaTable();
        }

        template<> inline xLuaFunction LoadValue<xLuaFunction>(xLuaState* l, int index) {
            if (lua_type(l->GetState(), index) == LUA_TFUNCTION)
                return xLuaFunction(RawState(l), RawState(l)->RefLuaObj(index));
            return xLuaFunction();
        }

        template <typename Ty> Ty LoadValue(xLuaState* l, int index) {
            static_assert(!std::is_reference<Ty>::value, "can not load reference value");
            static_assert(!std::is_same<char*, Ty>::value, "can not load char*, use const char*");
            return Loader<typename std::decay<Ty>::type>::Do(l, index);
        }
    }

    template <typename Ty> Ty Load(xLuaState* l, int index) {
        return loader::LoadValue<Ty>(l, index);
    }

    namespace param {
        /* 原始类型, 去除所有类型修饰(const, pointer, reference, volatile) */
        template <typename Ty>
        struct LiteralType {
            typedef typename std::remove_cv<
                typename std::remove_pointer<
                    typename std::decay<Ty>::type
                >::type
            >::type type;
        };

        /* 参数类型修正 */
        template <typename Ty>
        struct ParamType {
            typedef typename LiteralType<Ty>::type lit_type;
            typedef typename std::conditional<IsLuaType<lit_type>::value,
                typename std::conditional<std::is_pointer<Ty>::value, lit_type*, typename AddReference<lit_type>::type>::type,
                typename std::remove_volatile<typename std::remove_reference<Ty>::type>::type
            >::type type;
        };

        struct UdInfo {
            bool is_nil;
            const TypeInfo* info;
        };

        inline UdInfo GetUdInfo(xLuaState* l, int index, bool check_nil) {
            UdInfo ud_info{false, nullptr};
            int l_ty = l->GetType(index);

            if (l_ty == LUA_TNIL) {
                ud_info.is_nil = true;
            } else if (l_ty == LUA_TLIGHTUSERDATA) {
#if XLUA_ENABLE_LUD_OPTIMIZE
                LightUserData ud = MakeLightPtr(lua_touserdata(l->GetState(), index));
                if (!ud.IsLightData()) {
                    // unknown val
                } else if (ud.IsObjIndex()) {
                    ArrayObj* obj = GlobalVar::GetInstance()->GetArrayObj(ud.index_);
                    if (obj)
                        ud_info.info = obj->info_;
                    if (check_nil)
                        ud_info.is_nil = (obj == nullptr || obj->serial_num_ != ud.serial_);
                } else {
                    ud_info.info = GlobalVar::GetInstance()->GetLUDTypeInfo(ud.type_);
                    if (ud_info.info->is_weak_obj && check_nil)
                        ud_info.is_nil = (ud.serial_ == ::xLuaGetWeakObjSerialNum(ud.index_));
                }
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            } else if (l_ty == LUA_TUSERDATA) {
                FullUserData* ud = static_cast<FullUserData*>(lua_touserdata(l->GetState(), index));
                ud_info.info = ud->info_;
                if (!check_nil) {
                    if (ud->type_ == UserDataCategory::kObjPtr) {
                        if (ud->info_->is_weak_obj) {
                            ud_info.is_nil = (ud->serial_ == ::xLuaGetWeakObjSerialNum(ud->index_));
                        } else {
                            ArrayObj* obj = GlobalVar::GetInstance()->GetArrayObj(ud->index_);
                            ud_info.is_nil = (obj == nullptr || obj->serial_num_ != obj->serial_num_);
                        }
                    }
                }
            }
            return ud_info;
        }

        template <typename Ty>
        inline bool IsExtendTypeCheck(xLuaState* l, int index, std::true_type) {
            return xLuaIsType(l, index, Identity<Ty>());
        }

        template <typename Ty>
        inline bool IsExtendTypeCheck(xLuaState* l, int index, std::false_type) {
            return true;
        }

        template <typename Ty>
        struct ParamChecker {
            typedef typename std::remove_cv<Ty>::type value_type;
            static inline bool Do(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
                using tag = typename std::conditional<IsLuaType<value_type>::value, tag_declared,
                    typename std::conditional<IsExtendLoad<value_type>::value, tag_extend,
                    typename std::conditional<std::is_enum<value_type>::value, tag_enum, tag_unknown>::type>::type>::type;
                return Do<value_type>(l, index, param, lb, tag());
            }

            template <typename U>
            static inline bool Do(xLuaState* l, int index, int param, xLuaLogBuffer& lb, tag_unknown) {
                lb.Log("param(%d), unknown value", param);
                return false;
            }

            template <typename U>
            static inline bool Do(xLuaState* l, int index, int param, xLuaLogBuffer& lb, tag_enum) {
                if (l->GetType(index) != LUA_TNUMBER) {
                    lb.Log("param(%d), need:enum(number) got:%s", param, l->GetTypeName(index));
                    return false;
                }
                return true;
            }

            template <typename U>
            static inline bool Do(xLuaState* l, int index, int param, xLuaLogBuffer& lb, tag_extend) {
                if (IsExtendTypeCheck<U>(l, index, std::integral_constant<bool, IsExtendType<U>::value>()))
                    return true;

                lb.Log("param(%d), is not accept extend value type");
                return false;
            }

            template <typename U>
            static inline bool Do(xLuaState* l, int index, int param, xLuaLogBuffer& lb, tag_declared) {
                const TypeInfo* info = GetTypeInfoImpl<U>();
                UdInfo ud = GetUdInfo(l, index, true);
                if (ud.is_nil) {
                    lb.Log("param(%d), need:%s got:nil", param, info->type_name);
                    return false;
                }
                if (!IsBaseOf(info, ud.info)) {
                    lb.Log("param(%d), need:%s got:%s", param, info->type_name, l->GetTypeName(index));
                    return false;
                }
                return true;
            }
        };

        template <typename Ty>
        struct ParamChecker<Ty*> {
            typedef typename std::remove_cv<Ty>::type value_type;
            static_assert(IsLuaType<value_type>::value, "only type has declared export to lua accept");

            static inline bool Do(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
                const TypeInfo* info = GetTypeInfoImpl<value_type>();
                UdInfo ud = GetUdInfo(l, index, false);
                if (ud.is_nil || IsBaseOf(info, ud.info))
                    return true;
                lb.Log("param(%d), need:%s* got:%s", param, info->type_name, l->GetTypeName(index));
                return false;
            }
        };

        template <typename Ty>
        struct ParamChecker<std::shared_ptr<Ty>> {
            typedef typename std::remove_cv<Ty>::type value_type;
            static_assert(IsLuaType<value_type>::value, "only type has declared export to lua accept");

            static inline bool Do(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
                int l_ty = l->GetType(index);
                const TypeInfo* info = GetTypeInfoImpl<value_type>();
                if (l_ty == LUA_TUSERDATA) {
                    FullUserData* ud = static_cast<FullUserData*>(lua_touserdata(l->GetState(), index));
                    if (ud->type_ == UserDataCategory::kSharedPtr && IsBaseOf(info, ud->info_))
                        return true;
                } else if (l_ty == LUA_TNIL) {
                    return true;
                }

                lb.Log("param(%d), need:std::shared_ptr<%s> got:%s", param, info->type_name, l->GetTypeName(index));
                return false;
            }
        };

        template <typename Ty>
        struct ParamChecker<xLuaWeakObjPtr<Ty>> {
            static inline bool Do(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
                return ParamChecker<Ty*>::Do(l, index, param, lb);
            }
        };

        inline bool IsType_1(xLuaState* l, int index, int param, xLuaLogBuffer& lb, int ty, const char* need) {
            if (l->GetType(index) == ty)
                return true;
            lb.Log("param(%d), need:%s got:%s", param, need, l->GetTypeName(index));
            return false;
        }

        inline bool IsType_2(xLuaState* l, int index, int param, xLuaLogBuffer& lb, int ty, const char* need) {
            int l_ty = l->GetType(index);
            if (l_ty == ty || l_ty == LUA_TNIL)
                return true;
            lb.Log("param(%d), need:%s got:%s", param, need, l->GetTypeName(index));
            return false;
        }

        template <typename Ty>
        inline bool IsType(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return ParamChecker<Ty>::Do(l, index, param, lb);
        }

        template <> inline bool IsType<bool>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_2(l, index, param, lb, LUA_TBOOLEAN, "boolean");
        }

        template <> inline bool IsType<char>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "char(number)");
        }

        template <> inline bool IsType<unsigned char>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "unsigned char(number)");
        }

        template <> inline bool IsType<short>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "short(number)");
        }

        template <> inline bool IsType<unsigned short>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "unsigned short(number)");
        }

        template <> inline bool IsType<int>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "int(number)");
        }

        template <> inline bool IsType<unsigned int>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "unsigned int(number)");
        }

        template <> inline bool IsType<long>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "long(number)");
        }

        template <> inline bool IsType<unsigned long>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "unsigned long(number)");
        }

        template <> inline bool IsType<long long>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "short(number)");
        }

        template <> inline bool IsType<unsigned long long>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "unsigned long long(number)");
        }

        template <> inline bool IsType<float>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "float(number)");
        }

        template <> inline bool IsType<double>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_1(l, index, param, lb, LUA_TNUMBER, "double(number)");
        }

        template <> inline bool IsType<const char*>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_2(l, index, param, lb, LUA_TSTRING, "char*(number)");
        }

        template <> inline bool IsType<std::string>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_2(l, index, param, lb, LUA_TSTRING, "std::string");
        }

        /* void* */
        template <> inline bool IsType<void*>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_2(l, index, param, lb, LUA_TLIGHTUSERDATA, "void*");
        }

        template <> inline bool IsType<const void*>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_2(l, index, param, lb, LUA_TLIGHTUSERDATA, "const void*");
        }

        template <> inline bool IsType<xLuaTable>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_2(l, index, param, lb, LUA_TTABLE, "xLuaTable");
        }

        template <> inline bool IsType<xLuaFunction>(xLuaState* l, int index, int param, xLuaLogBuffer& lb) {
            return IsType_2(l, index, param, lb, LUA_TFUNCTION, "xLuaFunction");
        }

        template <typename... Ty, size_t... Idxs>
        inline bool CheckParam(xLuaState* l, int index, xLuaLogBuffer& lb, index_sequence<Idxs...>) {
            size_t count = 0;
            using els = int[];
            (void)els {
                0, (count += IsType<typename std::decay<Ty>::type>(l, index + Idxs, Idxs + 1, lb) ? 1 : 0, 0)...
            };
            return count == sizeof...(Ty);
        }

        /* 指针类型 */
        template <typename Ty, typename std::enable_if<std::is_pointer<Ty>::value, int>::type = 0>
        inline Ty LoadParamImpl(xLuaState* l, int index, std::true_type) {
            return l->Load<Ty>(index);
        }

        /* 引用类型 */
        template <typename Ty, typename std::enable_if<std::is_reference<Ty>::value, int>::type = 0>
        inline Ty LoadParamImpl(xLuaState* l, int index, std::true_type) {
            using type = typename std::remove_reference<Ty>::type;
            type* p = l->Load<type*>(index);
            assert(p);
            return *p;
        }

        template <typename Ty>
        inline Ty LoadParamImpl(xLuaState* l, int index, std::false_type) {
            return l->Load<typename std::remove_const<Ty>::type>(index);
        }

        template <>
        inline const char* LoadParamImpl<const char*>(xLuaState* l, int index, std::false_type) {
            return l->Load<const char*>(index); // const char* is expecial
        }

        template <typename Ty>
        inline Ty LoadParam(xLuaState* l, int index) {
            using type = typename LiteralType<Ty>::type;
            return LoadParamImpl<Ty>(l, index,
                std::integral_constant<bool, IsLuaType<type>::value>());
        }
    } // namespace param

    template <typename Fy, typename Ry, typename... Args, size_t... Idxs>
    inline auto DoFunctionImpl(Fy f, xLuaState* l, index_sequence<Idxs...> idxs) -> typename std::enable_if<!std::is_void<Ry>::value, int>::type {
        LogBufCache<> lb;
        if (!param::CheckParam<Args...>(l, 1, lb, idxs)) {
            l->GetCallStack(lb);
            luaL_error(l->GetState(), "attempt call function failed, parameter is not allow!\n%s", lb.Finalize());
            return 0;
        } else {
            l->Push(f(param::LoadParam<typename param::ParamType<Args>::type>(l, Idxs + 1)...));
            return 1;
        }
    }

    template <typename Fy, typename Ry, typename... Args, size_t... Idxs>
    inline auto DoFunctionImpl(Fy f, xLuaState* l, index_sequence<Idxs...> idxs) -> typename std::enable_if<std::is_void<Ry>::value, int>::type {
        LogBufCache<> lb;
        if (!param::CheckParam<Args...>(l, 1, lb, idxs)) {
            l->GetCallStack(lb);
            luaL_error(l->GetState(), "attempt call function failed, parameter is not allow!\n%s", lb.Finalize());
        } else {
            f(param::LoadParam<typename param::ParamType<Args>::type>(l, Idxs + 1)...);
        }

        return 0;
    }

    template <typename Fy, typename Ry, typename... Args>
    inline int DoFunction(Fy f, xLuaState* l) {
        return DoFunctionImpl<Fy, Ry, Args...>(f, l, make_index_sequence_t<sizeof...(Args)>());
    }
} // namespace detail

XLUA_NAMESPACE_END
