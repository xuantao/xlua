#pragma once
#include "detail/traits.h"
#include "detail/state.h"
#include <string>
#include <unordered_map>

XLUA_NAMESPACE_BEGIN

namespace detail {
    template <typename Ty> struct Pusher;
    template <typename Ty> struct Loader;
    struct MetaFuncs;
}

/* 引用Lua对象基类
 * Lua对象主要指Table, Function
*/
class xLuaObjBase {
    friend class xLuaState;
protected:
    xLuaObjBase() : ary_index_(-1), lua_(nullptr) {}
    xLuaObjBase(xLuaState* lua, int index) : lua_(lua), ary_index_(index) {}
    ~xLuaObjBase() { UnRef(); }

    xLuaObjBase(const xLuaObjBase& other)
        : lua_(other.lua_)
        , ary_index_(other.ary_index_) {
    }

    xLuaObjBase(xLuaObjBase&& other)
        : lua_(other.lua_)
        , ary_index_(other.ary_index_) {
        other.ary_index_ = -1;
    }

public:
    inline bool IsValid() const { return ary_index_ != -1; }

    xLuaObjBase& operator = (const xLuaObjBase& other) {
        UnRef();
        ary_index_ = other.ary_index_;
        lua_ = other.lua_;
        AddRef();
        return *this;
    }

    xLuaObjBase& operator = (xLuaObjBase&& other) {
        UnRef();
        lua_ = other.lua_;
        ary_index_ = other.ary_index_;
        other.ary_index_ = -1;
        return *this;
    }

    bool operator == (const xLuaObjBase& other) const {
        return ary_index_ == other.ary_index_;
    }

private:
    void AddRef();
    void UnRef();

private:
    xLuaState* lua_;
    int ary_index_;
};

/* lua table*/
class xLuaTable : private xLuaObjBase {
    friend class xLuaState;
    xLuaTable(xLuaState* l, int index) : xLuaObjBase(l, index) {}
public:
    xLuaTable() = default;
    xLuaTable(const xLuaTable&) = default;
    xLuaTable(xLuaTable&&) = default;

public:
    xLuaTable& operator = (const xLuaTable&) = default;
    xLuaTable& operator = (xLuaTable&&) = default;

public:
    inline bool IsValid() const { return xLuaObjBase::IsValid(); }
    inline operator bool() const { return IsValid(); }
};

/* lua function */
class xLuaFunction : private xLuaObjBase {
    friend class xLuaState;
    xLuaFunction(xLuaState* l, int index) : xLuaObjBase(l, index) {}
public:
    xLuaFunction() = default;
    xLuaFunction(const xLuaFunction&) = default;
    xLuaFunction(xLuaFunction&&) = default;

public:
    xLuaFunction& operator = (const xLuaFunction&) = default;
    xLuaFunction& operator = (xLuaFunction&&) = default;

public:
    inline bool IsValid() const { return xLuaObjBase::IsValid(); }
    inline operator bool() const { return IsValid(); }
};

/* lua堆栈守卫 */
class xLuaGuard {
public:
    xLuaGuard(xLuaState* l);
    xLuaGuard(xLuaState* l, int off);
    ~xLuaGuard();

public:
    xLuaGuard(const xLuaGuard&) = delete;
    xLuaGuard& operator = (const xLuaGuard&) = delete;

private:
    int top_;
    xLuaState* l_;
};

/* lua 状态机扩展 */
class xLuaState {
    struct UdCache {
        int lua_ref_;
        detail::FullUserData* user_data_;
    };

    struct LuaObjRef {
        union {
            int lua_ref_;
            int next_free_;
        };
        int ref_count_;
    };

    template <typename Ty> friend struct detail::Loader;
    template <typename Ty> friend struct detail::Pusher;
    friend struct detail::MetaFuncs;
    friend class detail::GlobalVar;
    friend class xLuaObjBase;

private:
    xLuaState(lua_State* l, bool attach);
    ~xLuaState();

public:
    /* 销毁 */
    void Release();

public:
    inline lua_State* GetState() const { return state_; }
    inline int GetTopIndex() const { return lua_gettop(state_); }
    inline void SetTop(int top) { lua_settop(state_, top); }
    inline void PopTop(int num) { lua_pop(state_, num); }

    inline int GetType(int index) const { return lua_type(state_, index); }
    const char* GetTypeName(int index);

    /* 当前lua 调用栈 */
    void LogCallStack() const;
    void GetCallStack(char* buf, size_t size) const;

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
    bool SetGlobal(const char* path);

    /* 将table成员加载到栈顶并返回数据类型 */
    inline int LoadTableField(int index, int field) {
        if (GetType(index) != LUA_TTABLE) {
            lua_pushnil(state_);
            detail::LogError("attempt to get table field:[%d], target is not table", field);
            return LUA_TNIL;
        }
        return lua_geti(state_, index, field);
    }

    inline int LoadTableField(int index, const char* field) {
        if (GetType(index) != LUA_TTABLE) {
            lua_pushnil(state_);
            detail::LogError("attempt to get table field:[%s], target is not table", field);
            return LUA_TNIL;
        }
        return lua_getfield(state_, index, field);
    }

    /* 将栈顶元素设置为table成员 */
    inline bool SetTableField(int index, int field) {
        if (GetType(index) != LUA_TTABLE) {
            lua_pop(state_, 1);
            detail::LogError("attempt to set table field:[%d], target is not table", field);
            return false;
        }
        lua_seti(state_, index, field);
        return true;
    }

    inline bool SetTableField(int index, const char* field) {
        if (GetType(index) != LUA_TTABLE) {
            lua_pop(state_, 1);
            detail::LogError("attempt to set table field:[%s], target is not table", field);
            return false;
        }
        lua_setfield(state_, index, field);
        return true;
    }

    template <typename Ty>
    inline Ty GetGlobal(const char* path) {
        xLuaGuard guard(this);
        LoadGlobal(path);
        return Load<Ty>(-1);
    }

    template <typename Ty>
    inline bool SetGlobal(const char* path, const Ty& val) {
        xLuaGuard guard(this);
        Push(val);
        return SetGlobal(path);
    }

    inline bool SetGlobal(const char* path, std::nullptr_t) {
        xLuaGuard guard(this);
        lua_pushnil(state_);
        return SetGlobal(path);
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

    inline bool SetTableField(int index, const char* field, std::nullptr_t) {
        index = lua_absindex(state_, index);
        lua_pushnil(state_);
        return SetTableField(index, field);
    }

    template <typename Ty>
    inline Ty GetTableField(xLuaTable table, int field) {
        xLuaGuard guard(this);
        Push(table);
        LoadTableField(-1, field);
        return Load<Ty>(-1);
    }

    template <typename Ty>
    inline Ty GetTableField(xLuaTable table, const char* field) {
        xLuaGuard guard(this);
        Push(table);
        LoadTableField(-1, field);
        return Load<Ty>(-1);
    }

    template <typename Ty>
    inline bool SetTableField(xLuaTable table, int field, const Ty& val) {
        xLuaGuard guard(this);
        Push(table);
        Push(val);
        return SetTableField(-2, field);
    }

    template <typename Ty>
    inline bool SetTableField(xLuaTable table, const char* field, const Ty& val) {
        xLuaGuard guard(this);
        Push(table);
        Push(val);
        return SetTableField(-2, field);
    }

    template <typename Ty>
    inline void Push(const Ty& val) {
        detail::Pusher<typename std::decay<Ty>::type>::Do(this, val);
    }

    template <typename Ty>
    inline Ty Load(int index) {
        static_assert(!std::is_reference<Ty>::value, "can not load reference value");
        return detail::Loader<typename std::decay<Ty>::type>::Do(this, index);
    }

    template<typename... Ty>
    inline void PushMul(Ty&&... vals) {
        using ints = int[];
        (void)ints {0, (Push(std::forward<Ty>(vals)), 0)...};
    }

    template <typename... Ty>
    inline void LoadMul(std::tuple<Ty&...> tp, int index) {
        DoLoadMul(tp, index, detail::make_index_sequence_t<sizeof...(Ty)>());
    }

    inline void Push(bool val) { lua_pushboolean(state_, val); }
    inline void Push(char val) { lua_pushnumber(state_, val); }
    inline void Push(unsigned char val) { lua_pushnumber(state_, val); }
    inline void Push(short val) { lua_pushnumber(state_, val); }
    inline void Push(unsigned short val) { lua_pushnumber(state_, val); }
    inline void Push(int val) { lua_pushnumber(state_, val); }
    inline void Push(unsigned int val) { lua_pushnumber(state_, val); }
    inline void Push(long val) { lua_pushnumber(state_, val); }
    inline void Push(unsigned long val) { lua_pushnumber(state_, val); }
    inline void Push(long long val) { lua_pushinteger(state_, val); }
    inline void Push(unsigned long long val) { lua_pushinteger(state_, val); }
    inline void Push(float val) { lua_pushnumber(state_, val); }
    inline void Push(double val) { lua_pushnumber(state_, val); }
    inline void Push(char* val) { lua_pushstring(state_, val); }
    inline void Push(const char* val) { lua_pushstring(state_, val); }
    inline void Push(const std::string& val) { lua_pushstring(state_, val.c_str()); }
    inline void Push(lua_CFunction func) { lua_pushcfunction(state_, func); }
    inline void Push(const void* val) { Push(const_cast<void*>(val)); }
    inline void Push(void* val) {
#if XLUA_USE_LIGHT_USER_DATA
        assert(detail::IsValidRawPtr(val));
#endif
        if (val == nullptr)
            lua_pushnil(state_);
        else
            lua_pushlightuserdata(state_, val);
    }

    inline void Push(std::nullptr_t) { lua_pushnil(state_); }

    inline void Push(const xLuaTable& val) {
        if (val.lua_ != this)
            Push(nullptr);
        else
            PushLuaObj(val.ary_index_);
    }

    inline void Push(const xLuaFunction& val) {
        if (val.lua_ != this)
            Push(nullptr);
        else
            PushLuaObj(val.ary_index_);
    }

    template<> inline bool Load<bool>(int index) { return lua_toboolean(state_, index); }
    template<> inline char Load<char>(int index) { return (char)lua_tonumber(state_, index); }
    template<> inline unsigned char Load<unsigned char>(int index) { return (unsigned char)lua_tonumber(state_, index); }
    template<> inline short Load<short>(int index) { return (short)lua_tonumber(state_, index); }
    template<> inline unsigned short Load<unsigned short>(int index) { return (unsigned short)lua_tonumber(state_, index); }
    template<> inline int Load<int>(int index) { return (int)lua_tonumber(state_, index); }
    template<> inline unsigned int Load<unsigned int>(int index) { return (int)lua_tonumber(state_, index); }
    template<> inline long Load<long>(int index) { return (long)lua_tonumber(state_, index); }
    template<> inline unsigned long Load<unsigned long>(int index) { return (unsigned long)lua_tonumber(state_, index); }
    template<> inline long long Load<long long>(int index) { return (long long)lua_tointeger(state_, index); }
    template<> inline unsigned long long Load<unsigned long long>(int index) { return (unsigned long long)lua_tointeger(state_, index); }
    template<> inline float Load<float>(int index) { return (float)lua_tonumber(state_, index); }
    template<> inline double Load<double>(int index) { return (double)lua_tonumber(state_, index); }
    template<> inline const char* Load<const char*>(int index) { return lua_tostring(state_, index); }
    template<> inline void* Load<void*>(int index) { return lua_touserdata(state_, index); }
    template<> inline const void* Load<const void*>(int index) { return lua_touserdata(state_, index); }
    template<> inline std::string Load<std::string>(int index) { return std::string(lua_tostring(state_, index)); }
    template<> inline lua_CFunction Load<lua_CFunction>(int index) { return lua_tocfunction(state_, index); }

    template<> inline xLuaTable Load<xLuaTable>(int index) {
        if (lua_type(state_, index) == LUA_TTABLE)
            return xLuaTable(this, RefLuaObj(index));
        return xLuaTable();
    }

    template<> inline xLuaFunction Load<xLuaFunction>(int index) {
        if (lua_type(state_, index) == LUA_TFUNCTION)
            return xLuaFunction(this, RefLuaObj(index));
        return xLuaFunction();
    }

private:
    template <typename... Ty, size_t... Idxs>
    inline void DoLoadMul(std::tuple<Ty&...>& ret, int index, detail::index_sequence<Idxs...>) {
        using ints = int[];
        (void)ints { 0, (std::get<Idxs>(ret) = Load<Ty>(index++), 0)...};
    }

    template <typename Ty, typename... Args>
    inline Ty* NewUserData(const detail::TypeInfo* info, Args&&... args) {
        void* mem = lua_newuserdata(state_, sizeof(Ty));

        int ty = 0;
        ty = lua_rawgeti(state_, LUA_REGISTRYINDEX, meta_table_ref_);    // type metatable table
        assert(ty == LUA_TTABLE);
        ty = lua_rawgeti(state_, -1, info->index);   // metatable
        assert(ty == LUA_TTABLE);
        lua_remove(state_, -2);                 // remove type table
        lua_setmetatable(state_, -2);
        return new (mem) Ty(std::forward<Args>(args)...);
    }

    void PushPtrUserData(const detail::FullUserData& ud);

    template <typename Ty>
    void PushValueData(const Ty& val, const detail::TypeInfo* info) {
        NewUserData<detail::ValueUserData<Ty>>(info, val, info);
    }

    template <typename Ty>
    inline void PushSharedPtr(const std::shared_ptr<Ty>& ptr, const detail::TypeInfo* info) {
        void* root = detail::GetRootPtr(ptr.get(), info);
        auto it = shared_ptrs_.find(root);
        if (it != shared_ptrs_.cend()) {
            UpdateCahce(it->second, ptr.get(), info);
            PushUd(it->second);
        } else {
            UdCache udc = RefCache(NewUserData<detail::ValueUserData<std::shared_ptr<Ty>>>(info, ptr, info));
            shared_ptrs_.insert(std::make_pair(root, udc));
        }
    }

    inline void UpdateCahce(UdCache& cache, void* ptr, const detail::TypeInfo* info) {
        if (!detail::IsBaseOf(info, cache.user_data_->info_)) {
            cache.user_data_->obj_ = ptr;
            cache.user_data_->info_ = info;
        }
    }

    inline void PushUd(UdCache& cache) {
        lua_rawgeti(state_, LUA_REGISTRYINDEX, user_data_table_ref_);
        lua_rawgeti(state_, -1, cache.lua_ref_);
        lua_remove(state_, -2);
    }

    inline UdCache RefCache(detail::FullUserData* data) {
        UdCache udc ={0, data};
        lua_rawgeti(state_, LUA_REGISTRYINDEX, user_data_table_ref_);
        lua_pushvalue(state_, -2);
        udc.lua_ref_ = luaL_ref(state_, -2);
        lua_pop(state_, 1);
        return udc;
    }

    inline void UnRefCachce(UdCache& cache) {
        lua_rawgeti(state_, LUA_REGISTRYINDEX, user_data_table_ref_);
        luaL_unref(state_, -1, cache.lua_ref_);
        lua_pop(state_, 1);
        cache.lua_ref_ = 0;
        cache.user_data_ = nullptr;
    }

    void Gc(detail::FullUserData* user_data);

    bool InitEnv(const char* export_module,
        const std::vector<const detail::ConstInfo*>& consts,
        const std::vector<detail::TypeInfo*>& types,
        const std::vector<const char*>& scripts
    );
    void InitConsts(const char* export_module, const std::vector<const detail::ConstInfo*>& consts);
    void CreateTypeMeta(const detail::TypeInfo* info);
    void CreateTypeGlobal(const char* export_module, const detail::TypeInfo* info);
    void SetTypeMember(const detail::TypeInfo* info);
    void SetGlobalMember(const detail::TypeInfo* info, bool func, bool var);
    void PushClosure(lua_CFunction func);

    int RefLuaObj(int index);
    void PushLuaObj(int ary_index);
    void AddObjRef(int ary_index);
    void UnRefObj(int ary_index);

private:
    bool attach_;
    lua_State* state_;
    int meta_table_ref_ = 0;        // 导出元表索引
    int user_data_table_ref_ = 0;   // user data table
    int lua_obj_table_ref_ = 0;     // table, function
    int next_free_lua_obj_ = -1;    // 下一个的lua对象表空闲槽
    std::vector<LuaObjRef> lua_objs_;
    std::vector<UdCache> lua_obj_ptrs_;
    std::vector<UdCache> weak_obj_ptrs_;
    std::unordered_map<void*, UdCache> raw_ptrs_;
    std::unordered_map<void*, UdCache> shared_ptrs_;
    char type_name_buf_[XLUA_MAX_TYPE_NAME_LENGTH];       // 输出类型名称缓存
};

/* Lua 函数调用器
 * 保护lua栈
*/
class xLuaCaller {
public:
    xLuaCaller(xLuaState* l) : l_(l), top_(-1) {
    }

    ~xLuaCaller() {
        RecoverTop();
    }

public:
    xLuaCaller(const xLuaCaller&) = delete;
    xLuaCaller& operator = (const xLuaCaller&) = delete;

public:
    /* 调用栈顶的Lua函数 */
    template<typename... Rys, typename... Args>
    bool operator () (std::tuple<Rys&...> ret, Args&&... args) {
        RecordTop(-1);
        if (l_->GetType(-1) != LUA_TFUNCTION) {
            detail::LogError("attempt to call is not a function");
            return false;
        }
        return DoCall(ret, std::forward<Args>(args)...);
    }

    /* 调用全局Lua函数 */
    template <typename... Rys, typename... Args>
    bool operator () (const char* global, std::tuple<Rys&...> ret, Args&&... args) {
        RecordTop(0);
        if (l_->LoadGlobal(global) != LUA_TFUNCTION) {
            detail::LogError("global var:[%s] is not a function", global);
            return false;
        }
        return DoCall(ret, std::forward<Args>(args)...);
    }

    /* 调用执行Lua函数 */
    template <typename... Rys, typename... Args>
    inline bool operator ()(lua_CFunction func, std::tuple<Rys&...> ret, Args&&... args) {
        RecordTop(0);
        l_->Push(func);
        return DoCall(ret, std::forward<Args>(args)...);
    }

    template <typename... Rys, typename... Args>
    inline bool operator ()(const xLuaFunction& func, std::tuple<Rys&...> ret, Args&&... args) {
        RecordTop(0);
        l_->Push(func);
        if (l_->GetType(-1) != LUA_TFUNCTION) {
            detail::LogError("attempt to call is not a function");
            l_->PopTop(1);
            return false;
        }
        return DoCall(ret, std::forward<Args>(args)...);
    }

    /* table call, table函数是冒号调用的 table:Call(xxx) */
    template <typename... Rys, typename... Args>
    inline bool operator ()(const xLuaTable& table, const char* func, std::tuple<Rys&...> ret, Args&&... args) {
        RecordTop(0);
        l_->Push(table);
        if (l_->LoadTableField(-1, func) != LUA_TFUNCTION) {
            detail::LogError("can not get table function:[%s]", func);
            return false;
        }
        return DoCall(ret, table, std::forward<Args>(args)...);
    }

private:
    template<typename... Rys, typename... Args>
    inline bool DoCall(std::tuple<Rys&...>& ret, Args&&... args) {
        int top = l_->GetTopIndex();
        l_->PushMul(std::forward<Args>(args)...);
        if (lua_pcall(l_->GetState(), sizeof...(Args), sizeof...(Rys), 0) != LUA_OK) {
            detail::LogError("excute lua function error!\n%s", lua_tostring(l_->GetState() , -1));
            l_->PopTop(1); // pop error message
            return false;
        }
        l_->LoadMul(ret, top);
        return true;
    }

    inline void RecordTop(int off) {
        RecoverTop();
        top_ = l_->GetTopIndex() + off;
    }

    inline void RecoverTop() {
        if (top_ >= 0)
            l_->SetTop(top_);
    }

private:
    xLuaState* l_;
    int top_;
};

namespace detail {
    /* push operator */
    template <typename Ty>
    struct Pusher {
        typedef typename std::remove_cv<Ty>::type value_type;
        static_assert(IsInternal<value_type>::value
            || IsExternal<value_type>::value
            || IsExtendPush<value_type>::value
            || std::is_enum<value_type>::value,
            "only type has declared export to lua accept");

        static inline void Do(xLuaState* l, const Ty& val) {
            using tag = typename std::conditional<IsInternal<value_type>::value || IsExternal<value_type>::value, tag_declared,
                typename std::conditional<std::is_enum<value_type>::value, tag_enum, tag_extend>::type>::type;
            PushValue<Ty>(l, val, tag());
        }

        template <typename U>
        static inline void PushValue(xLuaState* l, const U& val, tag_extend) {
            ::xLuaPush(l, val);
        }

        template <typename U>
        static inline void PushValue(xLuaState* l, const U& val, tag_enum) {
            l->Push(static_cast<int>(val));
        }

        template <typename U>
        static inline void PushValue(xLuaState* l, const U& val, tag_declared) {
            l->PushValueData(val, GetTypeInfoImpl<U>());
        }
    };

    template <typename Ty>
    struct Pusher<Ty*> {
        typedef typename std::remove_cv<Ty>::type value_type;
        static_assert(IsInternal<value_type>::value || IsExternal<value_type>::value,
            "only type has declared export to lua accept");

        static inline void Do(xLuaState* l, Ty* val) {
            if (val == nullptr) {
                l->Push(nullptr);
            } else {
                const TypeInfo* info = GetTypeInfoImpl<value_type>();
#if XLUA_USE_LIGHT_USER_DATA
                auto ud = MakeLightUserData(val, info);
                lua_pushlightuserdata(l->GetState(), ud.Ptr());
#else // XLUA_USE_LIGHT_USER_DATA
                l->PushPtrUserData(MakeFullUserData(val, info));
#endif // XLUA_USE_LIGHT_USER_DATA
            }
        }
    };

    template <typename Ty>
    struct Pusher<std::shared_ptr<Ty>> {
        typedef typename std::remove_cv<Ty>::type value_type;
        static_assert(IsInternal<value_type>::value || IsExternal<value_type>::value, "only type has declared export to lua accept");

        static inline void Do(xLuaState* l, const std::shared_ptr<Ty>& ptr) {
            if (!ptr) {
                l->Push(nullptr);
            } else {
                l->PushSharedPtr(ptr, GetTypeInfoImpl<value_type>());
            }
        }
    };

    template <typename Ty>
    struct Pusher<xLuaWeakObjPtr<Ty>> {
        typedef typename std::remove_cv<Ty>::type value_type;
        static_assert(IsInternal<value_type>::value || IsExternal<value_type>::value, "only type has declared export to lua accept");

        static inline void Do(xLuaState* l, const xLuaWeakObjPtr<Ty>& obj) {
            Ty* ptr = ::xLuaGetPtrByWeakObj(obj);
            if (ptr == nullptr) {
                l->Push(nullptr);
            } else {
                l->Push(ptr);
            }
        }
    };

    template <typename Ty>
    struct Loader {
        typedef typename std::remove_cv<Ty>::type value_type;
        static_assert(IsInternal<value_type>::value
            || IsExternal<value_type>::value
            || IsExtendLoad<value_type>::value
            || std::is_enum<value_type>::value,
            "only type has declared export to lua accept"
        );

        static inline Ty Do(xLuaState* l, int index) {
            using tag = typename std::conditional<IsInternal<value_type>::value || IsExternal<value_type>::value, tag_declared,
                typename std::conditional<std::is_enum<value_type>::value, tag_enum, tag_extend>::type>::type;
            return LoadValue<value_type>(l, index, tag());
        }

        template <typename U>
        static inline U LoadValue(xLuaState* l, int index, tag_extend) {
            return ::xLuaLoad(l, index, Identity<U>());
        }

        template <typename U>
        static inline U LoadValue(xLuaState* l, int index, tag_enum) {
            return (U)static_cast<int>(lua_tonumber(l->GetState(), index));
        }

        template <typename U>
        static inline U LoadValue(xLuaState* l, int index, tag_declared) {
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
        static_assert(IsInternal<value_type>::value || IsExternal<value_type>::value, "only type has declared export to lua accept");

        static Ty* Do(xLuaState* l, int index) {
            UserDataInfo ud_info;
            if (!GetUserDataInfo(l->GetState(), index, &ud_info) || ud_info.obj == nullptr)
                return nullptr;

            const TypeInfo* info = GetTypeInfoImpl<value_type>();
            if (!IsBaseOf(info, ud_info.info))
                return nullptr;

            return static_cast<Ty*>(_XLUA_TO_SUPER_PTR(info, ud_info.obj, ud_info.info));
        }
    };

    template <typename Ty>
    struct Loader<std::shared_ptr<Ty>> {
        typedef typename std::remove_cv<Ty>::type value_type;
        static_assert(IsInternal<value_type>::value || IsExternal<value_type>::value, "only type has declared export to lua accept");

        static inline std::shared_ptr<Ty> Do(xLuaState* l, int index) {
            if (lua_type(l->GetState(), index) != LUA_TUSERDATA)
                return nullptr;
            auto* ud = static_cast<FullUserData*>(lua_touserdata(l->GetState(), index));
            if (ud == nullptr || ud->type_ != UserDataCategory::kSharedPtr)
                return nullptr;
            const TypeInfo* info = GetTypeInfoImpl<value_type>();
            if (!IsBaseOf(info, ud->info_))
                return nullptr;

            return std::shared_ptr<Ty>(*static_cast<std::shared_ptr<Ty>*>(ud->GetDataPtr()),
                (Ty*)_XLUA_TO_SUPER_PTR(info, ud->obj_, ud->info_));
        }
    };

    template <typename Ty>
    struct Loader<xLuaWeakObjPtr<Ty>> {
        static_assert(IsInternal<Ty>::value || IsExternal<Ty>::value, "only type has declared export to lua accept");

        static inline xLuaWeakObjPtr<Ty> Do(xLuaState* l, int index) {
            return xLuaWeakObjPtr<Ty>(l->Load<Ty*>(index));
        }
    };
} // namespace detail

XLUA_NAMESPACE_END
