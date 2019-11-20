#pragma once
#include "xlua_def.h"
#include <array>
#include <vector>
#include <list>
#include <map>
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
    /* wrapper a lambda function object */
    template <typename Ty>
    struct Lambda {
        Lambda(const Ty& v) : lambda(v) {}
        Lambda(Ty&& v) : lambda(std::move(v)) {}

        Ty lambda;
    };

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
        FullUd() = default;

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

    /* object userdata, this ud will destruction on gc */
    struct IObjData {
        virtual ~IObjData() { }
    };

    template <typename Ty>
    struct AloneData : IObjData {
        AloneData(const Ty& o) : obj(o) {}
        AloneData(Ty&& o) : obj(std::move(o)) {}

        virtual ~AloneData() {}
        Ty obj;
    };

    struct ValueData : IObjData {
        ValueData() : index(0) {}

        int index;  // lua userdata reference
    };

    template <typename Ty>
    struct ValueDataImpl : ValueData {
        ValueDataImpl(const Ty& o) : obj(o) {}
        ValueDataImpl(Ty&& o) : obj(std::move(o)) {}
        virtual ~ValueDataImpl() {}

        Ty obj;
    };

    template <typename Ty>
    inline ValueData* ValuePtr2DataPtr(Ty* ptr) {
        return reinterpret_cast<ValueDataImpl<int>*>(
            reinterpret_cast<int8_t*>(ptr) - offsetof(ValueDataImpl<int>, obj));
    }

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

    struct AliasUd : FullUd
    {
        template <typename Ty, typename std::enable_if<std::is_base_of<IObjData, Ty>::value, int>::type = 0>
        inline Ty* As() {
            return reinterpret_cast<Ty*>(storage_);
        }

    private:
        AliasUd() = delete;
        AliasUd(const AliasUd&) = delete;

        char storage_[1];
    };

    template <typename Ty>
    struct ValueUd : FullUd {
        typedef ValueDataImpl<typename std::decay<Ty>::type> ObjData;
        template <typename Dy, typename... Args>
        ValueUd(Dy desc, Args&&... args) : FullUd(nullptr, UdMinor::kValue, desc) {
            auto* p = new (storage_) ObjData(std::forward<Args>(args)...);
            ptr = &p->obj;
        }

        char storage_[sizeof(ObjData)];
    };

    template <typename Sy>
    struct SmartPtrUd : FullUd {
        typedef SmartPtrDataImpl<typename std::decay<Sy>::type> ObjData;

        template <typename Dy, typename... Args>
        SmartPtrUd(void* ptr, Dy desc, Args&&... args) : FullUd(ptr, UdMinor::kSmartPtr, desc) {
            new (storage_) ObjData(std::forward<Args>(args)...);
        }

        char storage_[sizeof(ObjData)];
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
                int64_t ptr : 56;
                int64_t lud_index : 8;
            };
            // weak obj ref
            struct {
                int64_t ref_index : 24;
                int64_t ref_serial : 32;
                int64_t weak_index : 8;     // same as lud_index
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
#endif // XLUA_ENABLE_LUD_OPTIMIZE

    /* check lua stack value is specify type */
    template <size_t Idx>
    struct Checker {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            using traits = SupportTraits<
                typename std::tuple_element<sizeof...(Args) - Idx, std::tuple<Args...>>::type>;
            static_assert(traits::is_support, "not support type");
            using supporter = typename traits::supporter;
            return supporter::Check(s, index) &&
                Checker<Idx - 1>::template Do<Args...>(s, index + 1);
        }
    };

    template <>
    struct Checker<1> {
        template <typename... Args>
        static inline bool Do(State* s, int index) {
            using traits = SupportTraits<
                typename std::tuple_element<sizeof...(Args) - 1, std::tuple<Args...>>::type>;
            static_assert(traits::is_support, "not support type");
            using supporter = typename traits::supporter;
            return supporter::Check(s, index);
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
                return s;
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
            size_t len = str.len > (S - 1) ? (S - 1) : str.len;
            ::memcpy(cache_, str.str, len);
            cache_[len] = 0;
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

    template <typename Ty>
    class LuaObjRefArray {
        struct ObjRef {
            union {
                int ref;    // lua reference index
                int next;   // next empty slot
            };
            Ty value;       // obj value
        };

    public:
        LuaObjRefArray(Ty invalid) : invalid_(invalid_) {
            ObjRef o;
            o.next = 0;
            o.value = invalid_;
            objs_.push_back(o);
        }

        LuaObjRefArray(const LuaObjRefArray&) = delete;
        void operator = (const LuaObjRefArray&) = delete;

    public:
        inline bool IsValid(int index) {
            if (index > 0 && index < (int)objs_.size())
                return objs_[index].value != invalid_;
            return false;
        }

        inline int GetRef(int index) const {
            return objs_[index].ref;
        }

        inline Ty GetValue(int index) const {
            return objs_[index].value;
        }

        inline void SetValue(int index, Ty val) {
            objs_[index].value = val;
        }

        int Alloc(int ref, Ty val) {
            if (Next() == 0) {
                size_t sz = objs_.size();
                size_t ns = sz + XLUA_CONTAINER_INCREMENTAL;
                ObjRef tmp;
                tmp.next = 0;
                tmp.value = invalid_;
                objs_.resize(ns, tmp);
                for (size_t i = ns - 1; i > sz; --i)
                    objs_[i-1].next = (int)i;
                Next() = (int)sz;
            }

            int index = Next();
            auto& obj = objs_[index];
            Next() = obj.next;

            obj.ref = ref;
            obj.value = val;
            return index;
        }

        void Free(int index) {
            auto& obj = objs_[index];
            obj.next = Next();
            obj.value = invalid_;
            Next() = index;
        }

    private:
        inline int& Next() {
            return objs_.front().next;
        }

    private:
        const Ty invalid_;
        std::vector<ObjRef> objs_;
    };

    /* check the path whether is G table */
    inline constexpr bool Is_G(const char* path) {
        return path == nullptr || path[0] == 0 ||
            (path[0] == '_' && path[1] == 'G' && path[2] == 0);
    }

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
            } else if (lty == LUA_TNIL) {
                name = "nil";
            } else {
                name = lua_typename(l_, index);
            }
            return name;
        };

        size_t GetCallStack(char* buff, size_t size) {
            lua_Debug dbg;
            int level = 1;
            int sz = (int)sz;
            int len = snprintf(buff, size, "stack taceback:");
            if (len < 0 || len > sz)
                return sz;
            sz -= len;

            while (sz && lua_getstack(l_, level, &dbg)) {
                lua_getinfo(l_, "nSl", &dbg);
                ++level;

                if (dbg.name)
                    len = snprintf(buff, sz, "    %s:%d, in faild '%s'", dbg.source, dbg.currentline, dbg.name);
                else
                    len = snprintf(buff, sz, "    %s:%d, in main chunk", dbg.source, dbg.currentline);

                if (len < 0 || len > sz)
                    return sz;
                else
                    sz -= len;
            }

            if (level == 1) {
                len = snprintf(buff, sz, "    in C?");
                if (len < 0 || len > sz)
                    return size;
                else
                    sz -= len;
            }
            return size - sz;
        }

        int LoadGlobal(const char* path) {
            if (Is_G(path)) {
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
                } else if (!_IS_TABLE_TYPE(l_ty)) {
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

        template <typename Ty, typename InfoType>
        inline bool IsUd(int index, InfoType* info) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (lua_islightuserdata(l_, index))
                return IsLud(LightUd::Make(lua_touserdata(l_, index)), info);
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            auto* ud = LoadRawUd(index);
            if (ud)
                return IsFud(ud, info);
            return false;
        }

        template <typename Ty, typename InfoTy>
        Ty* LoadUd(int index, InfoTy* info) {
#if XLUA_ENABLE_LUD_OPTIMIZE
            if (lua_islightuserdata(l_, index)) {
                return (Ty*)As(LightUd::Make(lua_touserdata(l_, index)), info);
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE
            auto* ud = LoadRawUd(index);
            if (ud)
                return (Ty*)As(ud, info);
            return nullptr;
        }

        // collection value
        template <typename Ty>
        inline void PushUd(Ty&& obj, ICollection* collection) {
            auto* ud = NewValueUd(std::forward<Ty>(obj), collection);
            SetMetatable(collection);

            auto* data = static_cast<AliasUd*>(ud)->As<ValueData>();
            data->index = value_ud_ary_.Alloc(CacheUd(), data);
        }

        // declared type value
        template <typename Ty>
        inline void PushUd(Ty&& obj, const TypeDesc* desc) {
            auto* ud = NewValueUd(std::forward<Ty>(obj), desc);
            SetMetatable(desc);

            auto* data = static_cast<AliasUd*>(ud)->As<ValueData>();
            if (desc->caster.is_multi_inherit)
                value_ud_refs_.insert(std::make_pair(_XLUA_TO_SUPER_PTR(ud->ptr, desc, nullptr), CacheUd()));
            else
                data->index = value_ud_ary_.Alloc(CacheUd(), data);
        }

        // collection ptr
        template <typename Ty>
        inline void PushUd(Ty* ptr, ICollection* col) {
            if (ptr == nullptr) {
                lua_pushnil(l_);
                return;
            }

            /* lua owned object */
            auto* data = ValuePtr2DataPtr(ptr);
            if (value_ud_ary_.IsValid(data->index) && value_ud_ary_.GetValue(data->index) == data) {
                LoadCache(value_ud_ary_.GetRef(data->index));
                return;
            }

            /* whether cached ptr ud */
            auto it = collection_ptr_uds_.find(static_cast<void*>(ptr));
            if (it == collection_ptr_uds_.end()) {
                auto* ud = NewPtrUd(ptr, col);
                SetMetatable(col);
                collection_ptr_uds_.insert(std::make_pair(ptr, UdCache{CacheUd(), ud}));
            } else {
                LoadCache(it->second.ref);
            }
        }

        // delcared type ptr
        template <typename Ty>
        inline void PushUd(Ty* ptr, const TypeDesc* desc) {
            if (ptr == nullptr) {
                lua_pushnil(l_);
                return;
            }

            /* lua owned object */
            if (desc->caster.is_multi_inherit) {
                auto it = value_ud_refs_.find(_XLUA_TO_SUPER_PTR(ptr, desc, nullptr));
                if (it != value_ud_refs_.end()) {
                    LoadCache(it->second);
                    return;
                }
            } else {
                auto* data = ValuePtr2DataPtr(ptr);
                if (value_ud_ary_.IsValid(data->index) && value_ud_ary_.GetValue(data->index) == data) {
                    LoadCache(value_ud_ary_.GetRef(data->index));
                    return;
                }
            }

#if XLUA_ENABLE_LUD_OPTIMIZE
            if (LightUd ld = PackLightUd(ptr, desc)) {
                lua_pushlightuserdata(l_, ld.value);
                return;
            }
#endif // XLUA_ENABLE_LUD_OPTIMIZE

            if (desc->weak_index) {
                auto weak_ref = desc->weak_proc.maker(ptr);
                auto cache = GetWeakCache(desc->weak_index, weak_ref.index);
                auto* ud = cache.ud;
                if (ud) {
                    if (ud->ref != weak_ref) {              // prev weak obj is discard
                        ud->ref = WeakObjRef{0, 0};         // break the reference
                        ud = NewPtrUd(weak_ref, desc);
                        SetMetatable(desc);
                        UpdateCache(cache.ref);
                        SetWeakCache(desc->weak_index, weak_ref.index, cache.ref, ud);
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
                    ud = NewPtrUd(weak_ref, desc);
                    SetMetatable(desc);
                    SetWeakCache(desc->weak_index, weak_ref.index, CacheUd(), ud);
                }
            } else {
                void* tsp = _XLUA_TO_SUPER_PTR(ptr, desc, nullptr);
                auto it = declared_ptr_uds_.find(tsp);
                if (it != declared_ptr_uds_.end()) {
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
                        it->second.ud = NewPtrUd(ptr, desc);
                        SetMetatable(desc);
                        UpdateCache(it->second.ref);
                    }
                } else {
                    auto* ud = NewPtrUd(ptr, desc);
                    SetMetatable(desc);
                    declared_ptr_uds_.insert(std::make_pair(tsp, UdCache{CacheUd(), ud}));
                }
            }
        }

        // smart ptr
        template <typename Ty, typename Sty>
        inline void PushSmartPtr(Ty* ptr, Sty&& s, size_t tag, const TypeDesc* desc) {
            auto* tsp = _XLUA_TO_SUPER_PTR(ptr, desc, nullptr);
            auto it = smart_ptr_uds_.find(tsp);
            if (it == smart_ptr_uds_.end()) {
                auto* ud = NewSmartPtrUd(ptr, desc, std::forward<Sty>(s), tag);
                SetMetatable(desc);
                smart_ptr_uds_.insert(std::make_pair(tsp, UdCache{CacheUd(), ud}));
            } else {
                assert(static_cast<AliasUd*>(it->second.ud)->As<SmartPtrData>()->tag == tag);
                LoadCache(it->second.ref);
                // if the obj ptr is the derived type, update the ud info to derived type
                if (!IsBaseOf(desc, it->second.ud->desc)) {
                    it->second.ud->ptr = ptr;
                    it->second.ud->desc = desc;
                    SetMetatable(desc);
                }
            }
        }

        template <typename Ty, typename Sty>
        inline void PushSmartPtr(Ty* ptr, Sty&& s, size_t tag, ICollection* col) {
            auto it = smart_ptr_uds_.find(ptr);
            if (it == smart_ptr_uds_.end()) {
                auto* ud = NewSmartPtrUd(ptr, col, std::forward<Sty>(s), tag);
                SetMetatable(col);
                smart_ptr_uds_.insert(std::make_pair(ptr, UdCache{CacheUd(), ud}));
            } else {
                assert(static_cast<AliasUd*>(it->second.ud)->As<SmartPtrData>()->tag == tag);
                LoadCache(it->second.ref);
            }
        }

        template <typename... Args>
        inline FullUd* NewPtrUd(Args... args) {
            auto* d = lua_newuserdata(l_, sizeof(FullUd));
            return new (d) FullUd(args...);
        }

        template <typename Ty, typename Dy>
        inline FullUd* NewValueUd(Ty&& v, Dy info) {
            void* m = (void*)lua_newuserdata(l_, sizeof(ValueUd<Ty>));
            return new (m) ValueUd<Ty>(info, std::forward<Ty>(v));
        }

        template <typename Ty, typename Dy, typename Sy>
        inline FullUd* NewSmartPtrUd(Ty* ptr, Dy desc, Sy&& s, size_t tag) {
            void* d = (void*)lua_newuserdata(l_, sizeof(SmartPtrUd<Sy>));
            return new (d) SmartPtrUd<Sy>(ptr, desc, std::forward<Sy>(s), tag);
        }

        template <typename Ty, typename... Args>
        inline AloneData<Ty>* NewAloneUd(Args&&... args) {
            void* d = (void*)lua_newuserdata(l_, sizeof(AloneData<Ty>));
            auto* o = new (d) AloneData<Ty>(std::forward<Args>(args)...);
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

        /* reference a lua object by c++, invoid lua gc the object */
        int RefObj(int index) {
            int lty = lua_type(l_, index);
            //TODO: lua thread?
            if (lty != LUA_TTABLE && lty != LUA_TUSERDATA && lty != LUA_TFUNCTION)
                return 0;

            // reference lua object
            int ref = 0;
            index = lua_absindex(l_, index);
            lua_rawgeti(l_, LUA_REGISTRYINDEX, obj_ref_);   // load cache table
            lua_pushvalue(l_, index);                       // copy data
            ref = luaL_ref(l_, -2);                         // ref top stack user data
            lua_remove(l_, -1);                             // remove cache table

            return obj_ary_.Alloc(ref, 1);
        }

        inline bool LoadRef(int obj_idx) {
            if (!obj_ary_.IsValid(obj_idx))
                return false;

            lua_rawgeti(l_, LUA_REGISTRYINDEX, obj_ref_);   // load cache table
            lua_geti(l_, -1, obj_ary_.GetRef(obj_idx));     // load lua obj
            lua_remove(l_, -2);                             // remove obj table
            return true;
        }

        inline void AddObjRef(int obj_idx) {
            int count = obj_ary_.GetValue(obj_idx);
            assert(count);
            obj_ary_.SetValue(obj_idx, count + 1);
        }

        void DecObjRef(int obj_idx) {
            int count = obj_ary_.GetValue(obj_idx);
            assert(count);
            --count;

            if (count) {
                obj_ary_.SetValue(obj_idx, count);
            } else {
                lua_rawgeti(l_, LUA_REGISTRYINDEX, obj_ref_);   // load cache table
                luaL_unref(l_, -1, obj_ary_.GetRef(obj_idx));   // unref
                lua_pop(l_, 1);                                 // pop cache table
                obj_ary_.Free(obj_idx);
            }
        }

        /* cache the user data on top lua stack */
        inline int CacheUd() {
            int ref = LUA_NOREF;
            lua_rawgeti(l_, LUA_REGISTRYINDEX, cache_ref_); // load cache table
            lua_pushvalue(l_, -2);                          // copy user data to top
            ref = luaL_ref(l_, -2);                         // ref top stack user data
            lua_pop(l_, 1);                                 // pop cache table
            return ref;
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

        inline UdCache GetWeakCache(int weak_index, int obj_index) const {
            if (weak_index >= weak_obj_caches_.size())
                return UdCache{LUA_NOREF, nullptr};

            const auto& objs = weak_obj_caches_[weak_index];
            if (obj_index >= objs.size())
                return UdCache{LUA_NOREF, nullptr};
            return objs[obj_index];
        }

        inline void SetWeakCache(int weak_index, int obj_index, int ref, FullUd* ud) {
            if (weak_index >= weak_obj_caches_.size())
                weak_obj_caches_.resize(weak_index + 1);

            auto& objs = weak_obj_caches_[weak_index];
            if (obj_index >= objs.size())
                objs.resize((obj_index / XLUA_CONTAINER_INCREMENTAL + 1) * XLUA_CONTAINER_INCREMENTAL, UdCache{LUA_NOREF, nullptr});

            objs[obj_index] = UdCache{ref, ud};
        }

        /* user data gc */
        void OnGc(FullUd* ud) {
            int ref = LUA_NOREF;
            if (ud->minor == UdMinor::kPtr) {
                if (ud->major == UdMajor::kCollection) {
                    auto it = collection_ptr_uds_.find(ud->ptr);
                    ref = it->second.ref;
                    collection_ptr_uds_.erase(it);
                } else if (ud->major == UdMajor::kDeclaredType) {
                    if (ud->ptr) {  // need check the user data whether is discard
                        if (ud->desc->weak_index) {
                            ref = GetWeakCache(ud->desc->weak_index, ud->ref.index).ref;
                        } else {
                            auto it = declared_ptr_uds_.find(_XLUA_TO_SUPER_PTR(ud->ptr, ud->desc, nullptr));
                            ref = it->second.ref;
                            declared_ptr_uds_.erase(it);
                        }
                    }
                }
            } else if (ud->minor == UdMinor::kSmartPtr) {
                void* ptr = ud->ptr;
                if (ud->major == UdMajor::kDeclaredType)
                    ptr = _XLUA_TO_SUPER_PTR(ud->ptr, ud->desc, nullptr);

                auto it = smart_ptr_uds_.find(ptr);
                ref = it->second.ref;
                smart_ptr_uds_.erase(it);

                static_cast<AliasUd*>(ud)->As<IObjData>()->~IObjData();
            } else if (ud->minor == UdMinor::kValue) {
                auto* data = static_cast<AliasUd*>(ud)->As<ValueData>();
                if (ud->major == UdMajor::kDeclaredType && ud->desc->caster.is_multi_inherit) {
                    auto it = value_ud_refs_.find(_XLUA_TO_SUPER_PTR(ud->ptr, ud->desc, nullptr));
                    ref = it->second;
                    value_ud_refs_.erase(it);
                } else {
                    ref = value_ud_ary_.GetRef(data->index);
                    value_ud_ary_.Free(data->index);
                }
                data->~ValueData();
            } else {
                assert(false);
            }

            if (ref != LUA_NOREF) {
                lua_rawgeti(l_, LUA_REGISTRYINDEX, cache_ref_); // load cache table
                luaL_unref(l_, -1, ref);                        // unref cache data
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
        LuaObjRefArray<int> obj_ary_{0};
        /* lua owned userdata */
        LuaObjRefArray<ValueData*> value_ud_ary_{nullptr};
        std::unordered_map<void*, int> value_ud_refs_;
        /* */
        std::unordered_map<void*, UdCache> collection_ptr_uds_;
        std::unordered_map<void*, UdCache> declared_ptr_uds_;
        std::unordered_map<void*, UdCache> smart_ptr_uds_;
        std::vector<std::vector<UdCache>> weak_obj_caches_;
    }; // calss state_data
} // namespace internal

XLUA_NAMESPACE_END
