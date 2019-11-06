#pragma once
#include "xlua_def.h"
#include <array>
#include <vector>
#include <unordered_map>
#include <assert.h>
#include <lua.hpp>

XLUA_NAMESPACE_BEGIN

#if XLUA_ENABLE_LUD_OPTIMIZE
    #define _IS_TABLE_TYPE(type)    (type == LUA_TTABLE || type == LUA_TLIGHTUSERDATA || type == LUA_TUSERDATA)
#else
    #define _IS_TABLE_TYPE(type)    (type == LUA_TTABLE || type == LUA_TFullData)
#endif

namespace internal {
    template <typename Ty, typename std::enable_if<IsLuaType<Ty>::value, int>::type = 0>
    inline const TypeDesc* GetTypeDesc() {
        return xLuaGetTypeDesc(Identity<Ty>());
    }

    template <typename Ty, typename std::enable_if<IsCollctionType<Ty>::value, int>::type = 0>
    inline ICollection* GetTypeDesc() {
        return xLuaGetCollection(Identity<Ty>());
    }

    enum class UdMajor : int8_t {
        kNone = 0,
        kDeclaredType,
        kCollection,
    };

    enum class UdMinor : int8_t {
        kNone = 0,
        kPtr,
        kSmartPtr,
        kValue,
    };

    struct FullUd {
        // describe user data info
        struct {
            int8_t tag_1_ = _XLUA_TAG_1;
            int8_t tag_2_ = _XLUA_TAG_2;
            UdMajor major = UdMajor::kNone; // major userdata type
            UdMinor minor = UdMinor::kNone; // minor userdata type
        };
        // data information
        union {
            const TypeDesc* desc;
            ICollection* collection;
        };
        // obj data ptr
        union {
            void* ptr;
            WeakObjRef ref;
        };

    public:
        FullUd(void* _ptr, ICollection* _col) {
            major = UdMajor::kCollection;
            minor = UdMinor::kPtr;
            collection = _col;
            ptr = _ptr;
        }

        FullUd(void* _ptr, const TypeDesc* _desc) {
            major = UdMajor::kDeclaredType;
            minor = UdMinor::kPtr;
            desc = _desc;
            ptr = _ptr;
        }

        FullUd(WeakObjRef _ref, const TypeDesc* d) {
            major = UdMajor::kDeclaredType;
            minor = UdMinor::kPtr;
            desc = d;
            ref = _ref;
        }

    protected:
        FullUd(void* _ptr, UdMinor _minor, ICollection* col) {
            major = UdMajor::kCollection;
            minor = _minor;
            collection = col;
            ptr = _ptr;
        }

        FullUd(void* p, UdMinor _minor, const TypeDesc* _desc) {
            major = UdMajor::kDeclaredType;
            minor = _minor;
            desc = _desc;
            ptr = p;
        }

    public:
        inline bool IsValid() {
            return tag_1_ == _XLUA_TAG_1 && tag_2_ == _XLUA_TAG_2 &&
                major != UdMajor::kNone && minor != UdMinor::kNone;
        }
    };

    inline bool IsBaseOf(const TypeDesc* base, const TypeDesc* drive) {
        while (drive && drive != base)
            drive = drive->super;
        return drive == base;
    }

    inline const TypeDesc* GetSuperDesc(const TypeDesc* dest) {
        while (dest->super != nullptr)
            dest = dest->super;
        return dest;
    }

    inline bool IsValid(const FullUd* ud) {
        return ud->tag_1_ == _XLUA_TAG_1 && ud->tag_2_ == _XLUA_TAG_2 &&
            ud->major != UdMajor::kNone && ud->minor != UdMinor::kNone;
    }

    inline bool IsFud(FullUd* ud, const TypeDesc* desc) {
        if (ud->major != UdMajor::kDeclaredType)
            return false;
        if (!IsBaseOf(desc, ud->desc))
            return false;
        return true;
    }

    inline bool IsFud(FullUd* ud, ICollection* collection) {
        return ud->major == UdMajor::kCollection && ud->collection == collection;
    }

    inline void* As(FullUd* ud, const TypeDesc* desc) {
        if (ud->ptr == nullptr || !IsFud(ud, desc))
            return nullptr;

        void* ptr = nullptr;
        if (ud->minor == UdMinor::kPtr && ud->desc->weak_index)
            ptr = ud->desc->weak_proc.getter(ud->ref);
        else
            ptr = ud->ptr;
        return _XLUA_TO_SUPER_PTR(ud->ptr, ud->desc, desc);
    }

    inline void* As(FullUd* ud, ICollection* desc) {
        if (!IsFud(ud, desc))
            return nullptr;
        return ud->ptr;
    }

    //template <typename Ty, typename std::enable_if<IsLuaType<Ty>::value, int>::type = 0>
    //inline Ty* As(FullUd* ud) {
    //    if (ud->IsValid())
    //        return static_cast<Ty*>(As(ud, xLuaGetTypeDesc<Identity<Ty>()));
    //    return nullptr;
    //}

    //template <typename Ty, typename std::enable_if<IsCollctionType<Ty>::value, int>::type = 0>
    //inline Ty* As(FullUd* ud) {
    //    if (ud->IsValid())
    //        return static_cast<Ty*>(As(ud, xLuaGetCollection(Identity<Ty>()));
    //    return nullptr;
    //}

#define ASSERT_FUD(ud) assert(ud && ud->IsValid())

    /* object userdata, this ud will destruction on gc */
    struct IObjData {
        virtual ~IObjData() { }
    };

    template <typename Ty>
    struct ObjData : IObjData {
        ObjData(const Ty& o) : obj(o) {}
        ObjData(Ty&& o) : obj(std::move(o)) {}

        virtual ~ObjData() {}

        Ty obj;
    };

    struct SmartPtrData : IObjData {
        SmartPtrData(size_t t, void* d) : tag(t), data(d) {}
        virtual ~SmartPtrData() {}

        size_t tag;
        void* data;
    };

    template <typename Sy>
    struct SmartPtrDataImpl : SmartPtrData {
        SmartPtrDataImpl(const Sy& v, size_t tag) :
            SmartPtrData(tag, &val), val(v) {}

        SmartPtrDataImpl(Sy&& v, size_t tag) :
            SmartPtrData(tag, &val), val(std::move(v)) {}

        virtual ~SmartPtrDataImpl() { }

        Sy val;
    };

    struct ObjUd : FullUd
    {
        template <typename Ty, typename std::enable_if<std::is_base_of<IObjData, Ty>::value, int>::type = 0>
        inline Ty* As() {
            return static_cast<Ty*>(reinterpret_cast<void*>(storage_));
        }

    private:
        ObjUd() = delete;
        ObjUd(const ObjUd&) = delete;

        char storage_[1];
    };

    template <typename Ty>
    struct ObjUdImpl : FullUd {
        template <typename Dy, typename... Args>
        ObjUdImpl(Dy desc, Args&&... args) : FullUd(storage_, UdMinor::kValue, desc) {
            new (storage_) Ty(std::forward<Args>(args)...);
        }

        char storage_[sizeof(Ty)];
    };

    template <typename Sy>
    struct ObjUdImpl<SmartPtrDataImpl<Sy>> : FullUd {
        template <typename Dy, typename... Args>
        ObjUdImpl(void* ptr, Dy desc, Args&&... args) : FullUd(ptr, UdMinor::kSmartPtr, desc) {
            new (storage_) SmartPtrDataImpl<Sy>(std::forward<Args>(args)...);
        }

        char storage_[sizeof(SmartPtrDataImpl<Sy>)];
    };

#if XLUA_ENABLE_LUD_OPTIMIZE
    /* lightuserdata
     * 64 bit contain typeinfo and object address/ weak object reference
    */
    struct LightUd {
        //TODO: bigedian?
        union {
            // raw ptr
            struct {
                int64_t lud_index : 8;
                int64_t ptr : 56;
            };
            // weak obj ref
            struct {
                int64_t weak_index : 8;     // same as lud_index
                int64_t ref_index : 24;
                int64_t ref_serial : 32;
            };
            // none
            struct {
                void* value;
            };
        };

        inline explicit operator bool() const {
            return lud_index != 0;
        }
        inline void* ToObj() const {
            return reinterpret_cast<void*>(ptr);
        }
        inline WeakObjRef ToWeakRef() const {
            return WeakObjRef{(int)ref_index, (int)ref_serial};
        }

        static inline LightUd Make(int8_t weak_index, int32_t ref_index, int32_t ref_serial) {
            LightUd ld;
            ld.weak_index = weak_index;
            ld.ref_index = ref_index;
            ld.ref_serial = ref_serial;
            return ld;
        }

        static inline LightUd Make(int8_t lud_index, void* p) {
            LightUd ld;
            ld.value = p;
            ld.lud_index = lud_index;
            return ld;
        }

        static inline LightUd Make(void* p) {
            LightUd ld;
            ld.value = p;
            return ld;
        }

        static inline bool IsValid(void* p) {
            auto val = reinterpret_cast<uint64_t>(p);
            return (val & 0xff00000000000000) != 0;
        }
    };

    const TypeDesc* GetLightUdDesc(LightUd);
    bool CheckLightUd(LightUd ld, const TypeDesc* desc);
    void* UnpackLightUd(LightUd ld, const TypeDesc* desc);
    LightUd PackLightUd(void* obj, const TypeDesc* desc);

    inline bool IsLud(LightUd ld, const TypeDesc* desc) {
        return (bool)ld && CheckLightUd(ld, desc);
    }

    inline bool IsLud(LightUd ld, ICollection* collection) {
        return false;
    }

    inline void* As(LightUd ld, const TypeDesc* desc) {
        return (bool)ld ? UnpackLightUd(ld, desc) : nullptr;
    }

    inline void* As(LightUd ld, ICollection* collection) {
        return nullptr;
    }

    //template <typename Ty, , typename std::enable_if<IsLuaType<Ty>::value>::type = 0>
    //inline Ty* As(LightUd ld) {
    //    return static_cast<Ty*>(As(ld, xLuaGetTypeDesc(Identity<Ty>())));
    //}

    //template <typename Ty, , typename std::enable_if<IsCollctionType<Ty>::value>::type = 0>
    //inline Ty* As(LightUd ld) {
    //    return nullptr;
    //}
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    /* check lua stack value is specify type */
    template <size_t Idx>
    struct Checker {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            typedef typename PurifyType<
                typename std::tuple_element<sizeof...(Args) - Idx, std::tuple<Args...>>::type>::type type;
            static_assert(IsSupport<type>::value, "not support type");
            return Support<type>::Check(s, index) &&
                Checker<Idx - 1>::template Do<Args...>(s, index + 1);
        }
    };

    template <>
    struct Checker<1> {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            typedef typename PurifyType<
                typename std::tuple_element<sizeof...(Args) - 1, std::tuple<Args...>>::type>::type type;
            static_assert(IsSupport<type>::value, "not support type");
            return Support<type>::Check(s, index);
        }
    };

    template <>
    struct Checker<0> {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            return true;
        }
    };

    constexpr size_t StrLen(const char* s) {
        if (s == nullptr)
            return 0;

        size_t l = 0;
        while (*s) {
            ++l;
            ++s;
        }
        return l;
    }

    constexpr const char* StrStr(const char* s, const char* p) {
        size_t l = StrLen(p);
        while (*s) {
            size_t i = 0;
            while (s[i] == p[i] && p[i])
                ++i;
            if (i == l)
                return s + l;

            ++s;
        }
        return nullptr;
    }

    struct StringView {
        constexpr StringView(const char* s) : str(s), len(StrLen(s)) {}
        constexpr StringView(const char* s, size_t l) : str(s), len(l) {}

        const char* str;
        size_t len;
    };

    template <size_t S = 64>
    struct StringCache {
        StringCache(StringView str) {
            ::memcpy(cache_, str.str, str.len);
            cache_[S - 1] = 0;
        }

        inline const char* Str() { return cache_; }

    private:
        char cache_[S];
    };

    constexpr StringView PurifyMemberName(const char* name) {
        // find the last scope
        while (const char* sub = StrStr(name, "::"))
            name = sub + 2;
        // remove prefix: &
        if (name[0] == '&')
            ++name;
        // remove prefix: "m_"
        if (name[0] == 'm' && name[1] == '_')
            name += 2;
        // remove prefix: "lua"
        if ((name[0] == 'l' || name[0] == 'L') &&
            (name[1] == 'u' || name[1] == 'U') &&
            (name[2] == 'a' || name[2] == 'A')) {
            name += 3;
        }
        // remove prefix: '_'
        while (name[0] && name[0] == '_')
            ++name;
        // remove surfix: '_'
        size_t len = StrLen(name);
        while (len && name[len-1]=='_')
            --len;

        return StringView(name, len);
    }

    struct UdCache {
        int ref;    // lua reference index
        FullUd* ud; // lua userdata
    };

    struct LuaObjArray {
        struct ObjRef {
            union {
                int ref;    // lua reference index
                int next;   // next empty slot
            };
            int count;      // reference count
        };

        int empty = 0;
        std::vector<ObjRef> objs{ ObjRef() };   // first element is not used
    };

    /* internal state
     * manage all lua data
    */
    struct StateData {
        const char* GetTypeName(int index) const {
            const char* name = nullptr;
            int lty = lua_type(l_, index);
            if (lty == LUA_TLIGHTUSERDATA) {
#if XLUA_ENABLE_LUD_OPTIMIZE
                if (auto ld = LightUd::Make(lua_touserdata(l_, index))) {
                    auto* desc = GetLightUdDesc(ld);
                    if (desc)
                        name = desc->name;
                    else
                        name = "unknown";
                }
#else
                name = "void*";
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            } else if(lty == LUA_TUSERDATA) {
                auto* ud = static_cast<FullUd*>(lua_touserdata(l_, index));
                if (ud->IsValid()) {
                    if (ud->major == UdMajor::kCollection)
                        name = ud->collection->Name();
                    else
                        name = ud->desc->name;
                } else {
                    name = "unknown";
                }
            } else {
                name = lua_typename(l_, index);
            }
            return name;
        };

        //char* GetTypeName(char* buff, size_t sz, int index, int count) const {
        //    buff[0] = 0;
        //    for (int i = 0, tail = count - 1; i < count; ++i) {
        //        std::strncat(buff, GetTypeName(index + i), sz);
        //        if (i < tail)
        //            std::strncat(buff, ", ", sz);
        //    }
        //    return buff;
        //}

        inline bool DoString(const char* script, const char* chunk) {
            luaL_loadbuffer(l_, script, ::strlen(script), chunk);
            return true;
        }

        inline void PushNil() {
            lua_pushnil(l_);
        }

        inline void NewTable() {
            lua_newtable(l_);
        }

        int LoadGlobal(const char* path) {
            if (path == nullptr || path[0] == 0) {
                lua_pushglobaltable(l_);
                return LUA_TTABLE;
            }

            lua_pushglobaltable(l_);
            while (const char* sub = ::strchr(path, '.')) {
                lua_pushlstring(l_, path, sub - path);
                if (lua_gettable(l_, -2) != LUA_TTABLE) {
                    lua_pop(l_, 2);
                    lua_pushnil(l_);
                    return LUA_TNIL;
                }

                lua_remove(l_, -2);
                path = sub + 1;
            }

            int ty = lua_getfield(l_, -1, path);
            lua_remove(l_, -2);
            return ty;
        }

        bool SetGlobal(const char* path, bool create, bool rawset) {
            int top = lua_gettop(l_);
            if (top == 0)
                return false;           // none value

            if (path == nullptr || *path == 0) {
                lua_pop(l_, 1);         // pop value
                return false;           // wrong name
            }

            lua_pushglobaltable(l_);
            while (const char* sub = ::strchr(path, '.')) {
                lua_pushlstring(l_, path, sub - path);
                int l_ty = lua_gettable(l_, -2);
                if (l_ty == LUA_TNIL && create) {
                    lua_pop(l_, 1);                         // pop nil
                    lua_newtable(l_);                       // new table
                    lua_pushlstring(l_, path, sub - path);  // key
                    lua_pushvalue(l_, -2);                  // copy table
                    if (rawset)
                        lua_rawset(l_, -4);
                    else
                        lua_settable(l_, -4);               // set field
                }
                else if (!_IS_TABLE_TYPE(l_ty)) {
                    lua_pop(l_, 3);                         // [1]:value, [2]:table, [3]:unknown value
                    return false;
                }

                lua_remove(l_, -2);
                path = sub + 1;
            }

            lua_pushstring(l_, path);
            lua_rotate(l_, top, -1);
            if (rawset)
                lua_rawset(l_, -3);
            else
                lua_settable(l_, -3);    // set field

            lua_pop(l_, 1);              // [1]: table
            return true;
        }

        FullUd* LoadRawUd(int index) {
            if (lua_type(l_, index) != LUA_TUSERDATA)
                return nullptr;

            auto* ud = static_cast<FullUd*>(lua_touserdata(l_, index));
            if (ud->tag_1_ != _XLUA_TAG_1 || ud->tag_2_ != _XLUA_TAG_2)
                return nullptr;
            return ud;
        }

        template <typename Ty>
        inline bool IsUd(int index) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (lua_islightuserdata(l_, index))
                return IsLud(LightUd::Make(lua_touserdata(l_, index)), GetTypeDesc<Ty>());
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            auto* ud = LoadRawUd(index);
            if (ud)
                return IsFud(ud, GetTypeDesc<Ty>());
            return false;
        }

        template <typename Ty>
        Ty* LoadUd(int index) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (lua_islightuserdata(l_, index)) {
                return (Ty*)As(LightUd::Make(lua_touserdata(l_, index)), GetTypeDesc<Ty>());
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            auto* ud = LoadRawUd(index);
            if (ud)
                return (Ty*)As(ud, GetTypeDesc<Ty>());
            return nullptr;
        }

        // value
        template <typename Ty>
        inline void PushUd(const Ty& obj) {
            auto* desc = GetTypeDesc<Ty>();
            NewObjUd(obj, desc);
            SetMetatable(desc);
        }

        template <typename Ty>
        inline void PushUd(Ty&& obj) {
            auto* desc = GetTypeDesc<Ty>();
            NewObjUd(std::forward<Ty>(obj), desc);
            SetMetatable(desc);
        }

        // collection ptr
        template <typename Ty, typename std::enable_if<IsCollctionType<Ty>::value, int>::type = 0>
        inline void PushUd(Ty* ptr) {
            if (ptr == nullptr) {
                lua_pushnil(l_);
                return;
            }

            auto col = xLuaGetCollection(Identity<Ty>());
            auto it = collection_ptrs_.find(static_cast<void*>(ptr));
            if (it == collection_ptrs_.end()) {
                NewFud(ptr, col);
                SetMetatable(col);
                collection_ptrs_.insert(std::make_pair(ptr, CacheUd()));
            } else {
                LoadCache(it->second.ref);
            }
        }

        // delcared type ptr
        template <typename Ty, typename std::enable_if<IsLuaType<Ty>::value, int>::type = 0>
        inline void PushUd(Ty* ptr) {
            if (ptr == nullptr) {
                lua_pushnil(l_);
                return;
            }

            auto desc = xLuaGetTypeDesc(Identity<Ty>());
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (LightUd ld = PackLightUd(ptr, desc)) {
                lua_pushlightuserdata(l_, ld.value);
                return;
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

            if (desc->weak_index) {
                auto ref = desc->weak_proc.maker(ptr);
                auto& cache = MakeWeakCache(desc->weak_index, ref.index);
                auto* ud = cache.ud;
                if (ud) {
                    if (ud->ref != ref) {                   // prev weak obj is discard
                        ud->ref = WeakObjRef{0, 0};         // break the reference
                        cache.ud = NewFud(ref, desc);
                        SetMetatable(desc);
                        UpdateCache(cache.ref);
                    } else if (IsBaseOf(desc, ud->desc)) { // base type
                        LoadCache(cache.ref);
                    } else if (IsBaseOf(ud->desc, desc)) { // derived type
                        ud->ptr = ptr;
                        ud->desc = desc;
                        LoadCache(cache.ref);
                        SetMetatable(desc);
                    } else {
                        assert(false);
                    }
                } else {
                    NewFud(ref, desc);
                    SetMetatable(desc);
                    cache = CacheUd();
                }
            } else {
                void* tsp = _XLUA_TO_SUPER_PTR(ptr, desc, nullptr);
                auto it = declared_ptrs_.find(tsp);
                if (it != declared_ptrs_.end()) {
                    auto* ud = it->second.ud;
                    if (IsBaseOf(desc, ud->desc)) {         // base type
                        LoadCache(it->second.ref);
                    } else if (IsBaseOf(ud->desc, desc)) {  // derived object
                        ud->ptr = ptr;
                        ud->desc = desc;
                        LoadCache(it->second.ref);
                        SetMetatable(desc);
                    } else {                                // new object
                        ud->ptr = nullptr;                  // mark the ud is discarded
                        it->second.ud = NewFud(ptr, desc);
                        SetMetatable(desc);
                        UpdateCache(it->second.ref);
                    }
                } else {
                    NewFud(ptr, desc);
                    SetMetatable(desc);
                    declared_ptrs_.insert(std::make_pair(tsp, CacheUd()));
                }
            }
        }

        // smart ptr
        template <typename Ty, typename Sty>
        inline void PushSmartPtr(Ty* ptr, Sty&& s, size_t tag) {
            auto* desc = GetTypeDesc<Ty>();
            auto* tsp = _XLUA_TO_SUPER_PTR(ptr, desc, nullptr);
            auto it = smart_ptrs_.find(tsp);
            if (it == smart_ptrs_.end()) {
                NewSmartPtrUd(ptr, desc, std::forward<Sty>(s), tag);
                SetMetatable(desc);
                smart_ptrs_.insert(std::make_pair(tsp, CacheUd()));
            } else {
                assert(static_cast<ObjUd*>(it->second.ud)->As<SmartPtrData>()->tag == tag);
                LoadCache(it->second.ref);
                // if the obj ptr is the derived type, update the ud info to derived type
                if (!IsBaseOf(desc, it->second.ud->desc)) {
                    it->second.ud->ptr = ptr;
                    it->second.ud->desc = desc;
                    SetMetatable(desc);
                }
            }
        }

        template <typename... Args>
        inline FullUd* NewFud(Args... args) {
            auto* d = lua_newuserdata(l_, sizeof(FullUd));
            return new (d) FullUd(args...);
        }

        template <typename Ty, typename Dy>
        inline FullUd* NewObjUd(Ty&& v, Dy desc) {
            typedef ObjUdImpl<ObjData<typename std::decay<Ty>::type>> value_type;
            void* d = (void*)lua_newuserdata(l_, sizeof(value_type));
            return new (d) value_type(desc, std::forward<Ty>(v));
        }

        template <typename Ty, typename Dy, typename Sy>
        inline FullUd* NewSmartPtrUd(Ty* ptr, Dy desc, Sy&& s, size_t tag) {
            typedef ObjUdImpl<SmartPtrDataImpl<typename std::decay<Sy>::type>> value_type;
            void* d = (void*)lua_newuserdata(l_, sizeof(value_type));
            return new (d) value_type(ptr, desc, std::forward<Sy>(s), tag);
        }

        template <typename Ty, typename... Args>
        inline ObjData<Ty>* NewAloneObj(Args&&... args) {
            void* d = (void*)lua_newuserdata(l_, sizeof(ObjData<Ty>));
            auto* o = new (d) ObjData<Ty>(std::forward<Args>(args)...);
            lua_geti(l_, LUA_REGISTRYINDEX, alone_meta_ref_);
            lua_setmetatable(l_, -2);
            return o;
        }

        inline void SetMetatable(const TypeDesc* desc) {
            lua_geti(l_, LUA_REGISTRYINDEX, meta_ref_); // load metatable_list
            lua_geti(l_, -1, desc->id);                 // get type metatable
            lua_remove(l_, -2);                         // remove metatable_list
            lua_setmetatable(l_, -2);                   // set metatable
        }

        inline void SetMetatable(ICollection*) {
            lua_geti(l_, LUA_REGISTRYINDEX, collection_meta_ref_);  // load collection metatable
            lua_setmetatable(l_, -2);                               // set metatable
        }

        /* */
        int RefObj(int index) {
            int lty = lua_type(l_, index);
            //TODO: lua thread?
            if (lty != LUA_TFUNCTION && lty != LUA_TUSERDATA && lty != LUA_TFUNCTION)
                return 0;

            // reference lua object
            int ref = 0;
            index = lua_absindex(l_, index);
            lua_rawgeti(l_, LUA_REGISTRYINDEX, obj_ref_);   // load cache table
            lua_pushvalue(l_, index);                       // copy data
            ref = luaL_ref(l_, -2);                         // ref top stack user data
            lua_remove(l_, -1);                             // remove cache table

            // record reference count
            if (obj_ary_.empty == 0) {
                size_t sz = obj_ary_.objs.size();
                size_t ns = sz + 4096;

                obj_ary_.objs.resize(ns);
                for (size_t i = ns; i > sz; --i) {
                    auto& o = obj_ary_.objs[i - 1];
                    o.next = (int)i;
                    o.count = 0;
                }
                obj_ary_.objs[ns].next = 0;
            }

            int obj_idx = obj_ary_.empty;
            auto& obj = obj_ary_.objs[obj_idx];
            obj_ary_.empty = obj.next;
            obj.ref = ref;
            obj.count = 1;
            return obj_idx;;
        }

        inline bool LoadRef(int obj_idx) {
            if (obj_idx <= 0 || obj_idx >= (int)obj_ary_.objs.size())
                return false;

            lua_rawgeti(l_, LUA_REGISTRYINDEX, obj_ref_);   // load cache table
            lua_geti(l_, -1, obj_ary_.objs[obj_idx].ref);   // load lua obj
            lua_remove(l_, -2);                             // remove obj table
            return true;
        }

        void AddObjRef(int obj_idx) {
            if (obj_idx <= 0 || obj_idx >= (int)obj_ary_.objs.size())
                return;

            auto& obj = obj_ary_.objs[obj_idx];
            assert(obj.count);
            ++obj.count;
        }

        void DecObjRef(int obj_idx) {
            if (obj_idx <= 0 || obj_idx >= (int)obj_ary_.objs.size())
                return;

            auto& obj = obj_ary_.objs[obj_idx];
            assert(obj.count);
            --obj.count;

            if (obj.count == 0) {
                obj.next = obj_ary_.empty;
                obj_ary_.empty = obj_idx;
            }
        }

        /* cache the user data on lua top stack */
        inline UdCache CacheUd() {
            int ref = 0;
            auto* ud = static_cast<FullUd*>(lua_touserdata(l_, -1));
            ASSERT_FUD(ud);

            lua_rawgeti(l_, LUA_REGISTRYINDEX, cache_ref_); // load cache table
            lua_pushvalue(l_, -2);                          // copy user data to top
            ref = luaL_ref(l_, -2);                         // ref top stack user data
            lua_pop(l_, 1);                                 // pop cache table

            return UdCache{ref, ud};
        }

        inline void LoadCache(int ref) {
            lua_rawgeti(l_, LUA_REGISTRYINDEX, cache_ref_); // load cache table
            lua_geti(l_, -1, ref);                          // load cache data
            lua_remove(l_, -2);                             // remove cache table
        }

        inline void UpdateCache(int ref) {
            lua_rawgeti(l_, LUA_REGISTRYINDEX, cache_ref_); // load cache table
            lua_pushvalue(l_, -2);                          // copy user data to top
            lua_seti(l_, -2, ref);                          // modify the cache data
            lua_pop(l_, 1);                                 // pop cache table
        }

        inline UdCache& MakeWeakCache(int weak_index, int obj_index) {
            if (weak_index >= weak_objs_.size())
                weak_objs_.resize(weak_index + 1);
            auto& objs = weak_objs_[weak_index];
            if (obj_index >= objs.size())
                objs.resize((obj_index / 4096 + 1) * 4096, UdCache{LUA_NOREF, nullptr});
            return objs[obj_index];
        }

        /* user data gc */
        void OnGc(FullUd* ud) {
            UdCache cache{LUA_NOREF, nullptr};
            if (ud->minor == UdMinor::kPtr) {
                if (ud->major == UdMajor::kCollection) {
                    auto it = collection_ptrs_.find(ud->ptr);
                    if (it != collection_ptrs_.end()) {
                        cache = it->second;
                        collection_ptrs_.erase(it);
                    }
                } else if (ud->major == UdMajor::kDeclaredType) {
                    if (ud->ptr) {  // need check the user data whether is discard
                        if (ud->desc->weak_index) {
                            cache = MakeWeakCache(ud->desc->weak_index, ud->ref.index);
                        } else {
                            auto it = declared_ptrs_.find(ud->ptr);
                            if (it != declared_ptrs_.end()) {
                                cache = it->second;
                                declared_ptrs_.erase(it);
                            }
                        }
                    }
                }
            } else if (ud->minor == UdMinor::kSmartPtr) {
                auto it = smart_ptrs_.find(ud->ptr);
                if (it != smart_ptrs_.end()) {
                    cache = it->second;
                    smart_ptrs_.erase(it);
                }
                static_cast<ObjUd*>(ud)->As<IObjData>()->~IObjData();
            } else {
                static_cast<ObjUd*>(ud)->As<IObjData>()->~IObjData();
            }

            if (cache.ref != LUA_NOREF) {
                lua_rawgeti(l_, LUA_REGISTRYINDEX, cache_ref_); // load cache table
                luaL_unref(l_, -1, cache.ref);                  // unref cache data
                lua_pop(l_, 1);                                 // remove cache table
            }
        }

        const char* module_;
        lua_State* l_;
        bool is_attach_;
        int desc_ref_;
        int meta_ref_;
        int collection_meta_ref_;
        int alone_meta_ref_;
        int obj_ref_;
        int cache_ref_;

        /* ref lua objects, such as table, function, user data*/
        LuaObjArray obj_ary_;
        /* */
        std::unordered_map<void*, UdCache> collection_ptrs_;
        std::unordered_map<void*, UdCache> declared_ptrs_;
        std::unordered_map<void*, UdCache> smart_ptrs_;
        std::vector<std::vector<UdCache>> weak_objs_;
    }; // calss state_data
} // namespace internal

XLUA_NAMESPACE_END
