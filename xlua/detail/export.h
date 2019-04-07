#pragma once
#include "../xlua_def.h"
#include "../xlua_state.h"

XLUA_NAMESPACE_BEGIN

namespace detail {
    class GlobalVar;
    typedef const TypeInfo* (*fnTypeInfo)();
    typedef const ConstInfo* (*fnConstInfo)();

    enum class NodeCategory {
        kType,
        kConst,
        kScript,
    };

    struct NodeBase {
        NodeBase(NodeCategory type);
        ~NodeBase();

        NodeCategory category;
        NodeBase* next;
    };

    struct TypeNode : NodeBase {
        TypeNode(fnTypeInfo fn)
            : NodeBase(NodeCategory::kType)
            , func(fn) {
        }
        const fnTypeInfo func;
    };

    struct ConstNode : NodeBase {
        ConstNode(fnConstInfo fn)
            : NodeBase(NodeCategory::kConst)
            , func_(fn) {
        }
        const fnConstInfo func_;
    };

    struct ScriptNode : NodeBase {
        ScriptNode(const char* s)
            : NodeBase(NodeCategory::kScript)
            , script(s) {
        }
        const char* const script;
    };

    inline ConstValue MakeConstValue() {
        ConstValue cv;
        cv.category = ConstCategory::kNone;
        cv.name = nullptr;
        cv.int_val = 0;
        return cv;
    }

    inline ConstValue MakeConstValue(const char* name, int val) {
        ConstValue cv;
        cv.category = ConstCategory::kInteger;
        cv.name = name;
        cv.int_val = val;
        return cv;
    }

    inline ConstValue MakeConstValue(const char* name, float val) {
        ConstValue cv;
        cv.category = ConstCategory::kFloat;
        cv.name = name;
        cv.float_val = val;
        return cv;
    }

    inline ConstValue MakeConstValue(const char* name, const char* val) {
        ConstValue cv;
        cv.category = ConstCategory::kString;
        cv.name = name;
        cv.string_val = val;
        return cv;
    }

    template <typename Ty>
    struct IsMember {
        static constexpr bool value = std::is_member_pointer<Ty>::value || std::is_member_function_pointer<Ty>::value;
    };

    /* 导出属性萃取 */
    template <typename GetTy, typename SetTy>
    struct IndexerTrait {
    private:
        static constexpr bool all_nullptr = (std::is_null_pointer<GetTy>::value && std::is_null_pointer<SetTy>::value);
        static constexpr bool one_nullptr = (std::is_null_pointer<GetTy>::value || std::is_null_pointer<SetTy>::value);
        static constexpr bool is_get_member = (std::is_member_pointer<GetTy>::value || std::is_member_function_pointer<GetTy>::value);
        static constexpr bool is_set_member = (std::is_member_pointer<SetTy>::value || std::is_member_function_pointer<SetTy>::value);

    public:
        static constexpr bool is_allow = !all_nullptr && (one_nullptr || (is_get_member == is_set_member));
        static constexpr bool is_member = (is_get_member || is_set_member);
    };

    template <typename Ty, typename std::enable_if<IsWeakObjPtr<Ty>::value, int>::type = 0>
    inline Ty* ToDerivedWeakPtr(XLUA_WEAK_OBJ_BASE_TYPE* obj) {
        return dynamic_cast<Ty*>(obj);
    }

    template <typename Ty, typename By>
    struct TypeCasterImpl : ITypeCaster {
        virtual ~TypeCasterImpl() { }

        void* ToWeakPtr(void* obj) override {
            return ToWeakPtrImpl<Ty>(obj);
        }

        void* ToSuper(void* obj, const TypeInfo* dst) override {
            if (info_ == dst)
                return obj;
            return info_->super->caster->ToSuper(static_cast<By*>((Ty*)obj), dst);
        }

        void* ToDerived(void* obj, const TypeInfo* src) override {
            if (info_ == src)
                return obj;
            obj = info_->super->caster->ToDerived(obj, src);
            return obj ? dynamic_cast<Ty*>(static_cast<By*>(obj)) : nullptr;
        }

#if XLUA_USE_LIGHT_USER_DATA
        bool ToDerived(detail::LightUserData* ud) override {
            if (ud->Ptr() == nullptr) {
                LogError("unknown obj");
                return false;
            }

            const TypeInfo* src = nullptr;
            void* src_ptr = nullptr;
            if (ud->type_ == 0) {
                auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud->index_);
                if (ary_obj == nullptr || ary_obj->serial_num_ != ud->serial_) {
                    LogError("current obj is nil");
                    return false;
                }

                 src_ptr = ary_obj->obj_;
                 src = ary_obj->info_;
            } else {
                const TypeInfo* src_info = GlobalVar::GetInstance()->GetExternalTypeInfo(ud->type_);
                if (src_info == nullptr) {
                    LogError("unknown obj type");
                    return false;
                }
                if (src_info->is_weak_obj) {
                    if (ud->serial_ != ::xLuaGetWeakObjSerialNum(ud->index_)) {
                        LogError("current obj is nil");
                        return false;
                    }

                    src_ptr = ::xLuaGetWeakObjPtr(ud->index_);
                } else {
                    src_ptr = ud->RawPtr();
                }

                src = src_info;
            }

            if (IsBaseOf(info_, src))
                return true; // 不需要向基类转换

            if (!IsBaseOf(src, info_)) {
                LogError("can not cast obj from:[%s] to type:[%s]", src->type_name, info_->type_name);
                return false;
            }

            Ty* d = nullptr;
            if (src->is_weak_obj) {
                d = ToDerivedWeakPtr<Ty>(::xLuaGetWeakObjPtr(ud->index_));
            } else {
                d = (Ty*)ToDerived(src_ptr, src);
            }

            if (d == nullptr) {
                LogError("can not cast obj from:[%s] to type:[%s]", src->type_name, info_->type_name);
                return false;
            }

            *ud = MakeLightUserData(d, info_);
            return true;
        }
#endif // XLUA_USE_LIGHT_USER_DATA

        bool ToDerived(detail::FullUserData* ud) override {
            if (ud->type_ == UserDataCategory::kValue) {
                LogError("can not cast value obj");
                return false;
            }
            if (IsBaseOf(info_, ud->info_)) {
                return true; // 不需要向基类转换
            }
            if (!IsBaseOf(ud->info_, info_)) {
                LogError("can not cast obj from:[%s] to type:[%s]", ud->info_->type_name, info_->type_name);
                return false;
            }

            Ty* d = nullptr;
            if (ud->type_ == UserDataCategory::kSharedPtr) {
                d = (Ty*)ToDerived(ud->obj_, ud->info_);
                if (d == nullptr) {
                    LogError("can not cast obj from:[%s] to type:[%s]", ud->info_->type_name, info_->type_name);
                    return false;
                }

                ud->obj_ = d;
                ud->info_ = info_;
                return true;
            }

            if (ud->type_ == UserDataCategory::kRawPtr) {
                d = (Ty*)ToDerived(ud->obj_, ud->info_);
            } else if (ud->type_ == UserDataCategory::kObjPtr) {
                if (ud->info_->is_weak_obj) {
                    if (ud->serial_ != ::xLuaGetWeakObjSerialNum(ud->index_)) {
                        LogError("obj is nil");
                        return false;
                    }

                    d = ToDerivedWeakPtr<Ty>(::xLuaGetWeakObjPtr(ud->index_));
                } else {
                    auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud->index_);
                    if (ary_obj == nullptr|| ary_obj->serial_num_ != ud->serial_) {
                        LogError("obj is nil");
                        return false;
                    }

                    d = (Ty*)ToDerived(ary_obj->obj_, ary_obj->info_);
                }
            }

            if (d == nullptr) {
                LogError("can not cast obj from:[%s] to type:[%s]", ud->info_->type_name, info_->type_name);
                return false;
            }

            *ud = MakeFullUserData(d, info_);
            return true;
        }

    public:
        const TypeInfo* info_ = nullptr;
    };

    template <typename Ty>
    struct TypeCasterImpl<Ty, void> : ITypeCaster {
        virtual ~TypeCasterImpl() { }

        void* ToWeakPtr(void* obj) override { return obj; }
        void* ToSuper(void* obj, const TypeInfo* dst) override { return obj; }
        void* ToDerived(void* obj, const TypeInfo* src) override { return obj; }
#if XLUA_USE_LIGHT_USER_DATA
        bool ToDerived(detail::LightUserData* ud) override { return true; }
#endif // XLUA_USE_LIGHT_USER_DATA
        bool ToDerived(detail::FullUserData* ud) override { return true; }

    public:
        const TypeInfo* info_ = nullptr;
    };

    template <typename Ty>
    inline Ty* GetMetaCallFullUserDataPtr(void* user_data, const TypeInfo* info) {
        FullUserData* ud = static_cast<FullUserData*>(user_data);
        if (!IsBaseOf(info, ud->info_)) {
            LogError("can not get obj of type:[%s] from:[%s]", info->type_name, ud->info_->type_name);
            return nullptr;
        }

        if (ud->type_ == UserDataCategory::kObjPtr) {
            if (ud->info_->is_weak_obj) {
                if (ud->serial_ != ::xLuaGetWeakObjSerialNum(ud->index_))
                    return nullptr;
                return (Ty*)_XLUA_TO_WEAKOBJ_PTR(ud->info_, ::xLuaGetWeakObjPtr(ud->index_));
            } else {
                auto ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud->index_);
                if (ary_obj->serial_num_ != ud->serial_)
                    return nullptr;
                return static_cast<Ty*>(_XLUA_TO_SUPER_PTR(info, ary_obj->obj_, ary_obj->info_));
            }
        }
        return static_cast<Ty*>(_XLUA_TO_SUPER_PTR(info, ud->obj_, ud->info_));
    }

    inline void* GetMetaCallObj(xLuaState* l, const TypeInfo* info) {
        UserDataInfo ud_info;
        if (!GetUserDataInfo(l->GetState(), 1, &ud_info))
            return nullptr;
        if (!IsBaseOf(info, ud_info.info))
            return nullptr;

        if (info->is_weak_obj)
            return _XLUA_TO_WEAKOBJ_PTR(info, ud_info.obj);
        else
            return _XLUA_TO_SUPER_PTR(info, ud_info.obj, ud_info.info);
    }

    namespace param {
        template <typename Ty>
        struct Perify {
            typedef typename std::decay<Ty>::type deacy_type;
            typedef typename std::conditional<IsInternal<deacy_type>::value || IsExternal<deacy_type>::value, Ty,
                typename std::remove_reference<Ty>::type>::type type;
        };

        struct UdInfo {
            bool is_nil;
            const TypeInfo* info;
        };

        inline UdInfo GetUdInfo(xLuaState* l, int index, bool check_nil) {
            UdInfo ud_info{ false, nullptr};
            int l_ty = l->GetType(index);

            if (l_ty == LUA_TNIL) {
                ud_info.is_nil = true;
            } else if (l_ty == LUA_TLIGHTUSERDATA) {
#if XLUA_USE_LIGHT_USER_DATA
                LightUserData ud = MakeLightPtr(lua_touserdata(l->GetState(), index));
                if (ud.type_ == 0) {
                    ArrayObj* obj = GlobalVar::GetInstance()->GetArrayObj(ud.index_);
                    if (obj)
                        ud_info.info = obj->info_;
                    if (check_nil)
                        ud_info.is_nil = (obj == nullptr || obj->serial_num_ != ud.serial_);
                } else {
                    ud_info.info = GlobalVar::GetInstance()->GetExternalTypeInfo(ud.type_);
                    if (ud_info.info->is_weak_obj && check_nil)
                        ud_info.is_nil = (ud.serial_ == ::xLuaGetWeakObjSerialNum(ud.index_));
                }
#endif // XLUA_USE_LIGHT_USER_DATA
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
            return ::xLuaIsType(l, index, Identity<Ty>());
        }

        template <typename Ty>
        inline bool IsExtendTypeCheck(xLuaState* l, int index, std::false_type) {
            return true;
        }

        template <typename Ty>
        struct Checker {
            static inline bool Do(xLuaState* l, int index, int param) {
                using tag = typename std::conditional<IsInternal<Ty>::value || IsExternal<Ty>::value, tag_declared,
                    typename std::conditional<IsExtendLoad<Ty>::value, tag_extend,
                    typename std::conditional<std::is_enum<Ty>::value, tag_enum, tag_unknown>::type>::type>::type;
                return Do(l, index, param, tag());
            }

            static inline bool Do(xLuaState* l, int index, int param, tag_unknown) {
                return false;
            }

            static inline bool Do(xLuaState* l, int index, int param, tag_enum) {
                if (l->GetType(index) != LUA_TNUMBER) {
                    LogError("param(%d) error, need:enum(number) got:%s", param, l->GetTypeName(index));
                    return false;
                }
                return true;
            }

            static inline bool Do(xLuaState* l, int index, int param, tag_extend) {
                return IsExtendTypeCheck<Ty>(l, index, std::integral_constant<bool, IsExtendType<Ty>::value>());
            }

            static inline bool Do(xLuaState* l, int index, int param, tag_declared) {
                const TypeInfo* info = GetTypeInfoImpl<Ty>();
                UdInfo ud = GetUdInfo(l, index, true);
                if (ud.is_nil) {
                    LogError("param(%d) error, need:%s got:nil", param, info->type_name);
                    return false;
                }
                if (!IsBaseOf(info, ud.info)) {
                    LogError("param(%d) error, need:%s got:%s", param, info->type_name, l->GetTypeName(index));
                    return false;
                }
                return true;
            }
        };

        template <typename Ty>
        struct Checker<Ty*> {
            static_assert(IsInternal<Ty>::value || IsExternal<Ty>::value, "only declare export to lua types");
            static inline bool Do(xLuaState* l, int index, int param) {
                const TypeInfo* info = GetTypeInfoImpl<Ty>();
                UdInfo ud = GetUdInfo(l, index, false);
                if (ud.is_nil || IsBaseOf(ud.info, info))
                    return true;
                LogError("param(%d) error, need:%s* got:%s", param, info->type_name, l->GetTypeName(index));
                return false;
            }
        };

        template <typename Ty>
        struct Checker<std::shared_ptr<Ty>> {
            static_assert(IsInternal<Ty>::value || IsExternal<Ty>::value, "only declare export to lua types");
            static inline bool Do(xLuaState* l, int index, int param) {
                int l_ty = l->GetType(index);
                const TypeInfo* info = GetTypeInfoImpl<Ty>();
                if (l_ty == LUA_TUSERDATA) {
                    FullUserData* ud = static_cast<FullUserData*>(lua_touserdata(l->GetState(), index));
                    if (ud->type_ == UserDataCategory::kSharedPtr && IsBaseOf(info, ud->info_))
                        return true;
                } else if (l_ty == LUA_TNIL) {
                    return true;
                }

                LogError("param(%d) error, need:std::shared_ptr<%s> got:%s", param, info->type_name, l->GetTypeName(index));
                return false;
            }
        };

        template <typename Ty>
        struct Checker<xLuaWeakObjPtr<Ty>> {
            static inline bool Do(xLuaState* l, int index, int param) {
                return Checker<Ty*>::Do(l, index, param);
            }
        };

        inline bool IsType_1(xLuaState* l, int index, int param, int ty, const char* need) {
            if (l->GetType(index) == ty)
                return true;
            LogError("param(%d) error, need:%s got:%s", param, need, l->GetTypeName(index));
            return false;
        }

        inline bool IsType_2(xLuaState* l, int index, int param, int ty, const char* need) {
            int l_ty = l->GetType(index);
            if (l_ty == ty || l_ty == LUA_TNIL)
                return true;
            LogError("param(%d) error, need:%s got:%s", param, need, l->GetTypeName(index));
            return false;
        }

        template <typename Ty>
        inline bool IsType(xLuaState* l, int index, int param) {
            return Checker<Ty>::Do(l, index, param);
        }

        template <> inline bool IsType<bool>(xLuaState* l, int index, int param) {
            return IsType_2(l, index, param, LUA_TBOOLEAN, "boolean");
        }

        template <> inline bool IsType<char>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "char(number)");
        }

        template <> inline bool IsType<unsigned char>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "unsigned char(number)");
        }

        template <> inline bool IsType<short>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "short(number)");
        }

        template <> inline bool IsType<unsigned short>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "unsigned short(number)");
        }

        template <> inline bool IsType<int>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "int(number)");
        }

        template <> inline bool IsType<unsigned int>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "unsigned int(number)");
        }

        template <> inline bool IsType<long>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "long(number)");
        }

        template <> inline bool IsType<unsigned long>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "unsigned long(number)");
        }

        template <> inline bool IsType<long long>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "short(number)");
        }

        template <> inline bool IsType<unsigned long long>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "unsigned long long(number)");
        }

        template <> inline bool IsType<float>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "float(number)");
        }

        template <> inline bool IsType<double>(xLuaState* l, int index, int param) {
            return IsType_1(l, index, param, LUA_TNUMBER, "double(number)");
        }

        template <> inline bool IsType<char*>(xLuaState* l, int index, int param) {
            return IsType_2(l, index, param, LUA_TSTRING, "char*(number)");
        }

        template <> inline bool IsType<std::string>(xLuaState* l, int index, int param) {
            return IsType_2(l, index, param, LUA_TSTRING, "std::string");
        }

        template <> inline bool IsType<xLuaTable>(xLuaState* l, int index, int param) {
            return IsType_2(l, index, param, LUA_TTABLE, "xLuaTable");
        }

        template <> inline bool IsType<xLuaFunction>(xLuaState* l, int index, int param) {
            return IsType_2(l, index, param, LUA_TFUNCTION, "xLuaFunction");
        }

        inline void* GetUdPtr(xLuaState* l, int index, const TypeInfo* info) {
            UserDataInfo ud_info;
            if (!GetUserDataInfo(l->GetState(), index, &ud_info))
                return nullptr;
            if (!IsBaseOf(info, ud_info.info))
                return nullptr;
            return _XLUA_TO_SUPER_PTR(info, ud_info.obj, ud_info.info);
        }

        template <typename Ty, typename Ry>
        struct Loader {
            static Ry& Do(xLuaState* l, int index) {
                Ty* ptr = static_cast<Ty*>(GetUdPtr(l, index, GetTypeInfoImpl<Ty>()));
                assert(ptr);
                return *ptr;
            }
        };

        template <typename Ty, typename Ry>
        struct Loader<Ty*, Ry> {
            static Ry Do(xLuaState* l, int index) {
                return static_cast<Ty*>(GetUdPtr(l, index, GetTypeInfoImpl<Ty>()));
            }
        };

        template <typename Ty, typename Ry>
        struct Loader<std::shared_ptr<Ty>, Ry> {
            static Ry Do(xLuaState* l, int index) {
                return l->Load<std::shared_ptr<Ty>>(index);
            }
        };

        template <typename Ty, typename Ry>
        struct Loader<xLuaWeakObjPtr<Ty>, Ry> {
            static Ry Do(xLuaState* l, int index) {
                Ty* ptr = GetUdPtr(l, index, GetTypeInfoImpl<Ty>());
                return xLuaWeakObjPtr<Ty>(ptr);
            }
        };
    } // namespace param

    template <typename Ty>
    inline bool CheckParam(xLuaState* l, int index, int param) {
         using decay_ty = typename std::decay<Ty>::type;
        static_assert(!std::is_same<decay_ty, char*>::value || std::is_const<Ty>::value,
            "only accept const char* paramenter");
        return param::IsType<decay_ty>(l, index, param);
    }

    template <typename... Ty>
    inline bool CheckParams(xLuaState* l, int index) {
        size_t count = 0;
        int param = 0;
        using els = int[];
        (void)els { 0, (count += CheckParam<Ty>(l, index++, ++param) ? 1 : 0, 0)... };
        if (count == sizeof...(Ty))
            return true;
        //l->LogCallStack();
        luaL_error(l->GetState(), "whtf");
        return false;
    }

    template <typename Ty>
    inline Ty LoadParam_Un(xLuaState* l, int index) {
        return l->Load<typename std::decay<Ty>::type>(index);
    }

    template <>
    inline const char* LoadParam_Un<const char*>(xLuaState* l, int index) {
        return l->Load<const char*>(index); // const char* is expecial
    }

    template <typename Ty>
    inline Ty LoadParam(xLuaState* l, int index, tag_unknown) {
        return LoadParam_Un<Ty>(l, index);
    }

    template <typename Ty>
    inline Ty LoadParam(xLuaState* l, int index, tag_declared) {
        return detail::param::Loader<typename std::decay<Ty>::type, Ty>::Do(l, index);
    }

    template <typename Ty>
    inline Ty LoadParam(xLuaState* l, int index, tag_extend) {
        return ::xLuaLoad(l, index, Identity<typename std::decay<Ty>::type>());
    }

    template <typename Ty>
    inline Ty LoadParam(xLuaState* l, int index) {
        using decay_ty = typename std::decay<Ty>::type;
        using tag = typename std::conditional<IsInternal<decay_ty>::value || IsExternal<decay_ty>::value, tag_declared,
            typename std::conditional<IsExtendLoad<decay_ty>::value, tag_extend, tag_unknown>::type>::type;
        return LoadParam<Ty>(l, index, tag());
    }

    template <bool value, typename Ty = int>
    using EnableIfT = typename std::enable_if<value, int>::type;

    /* global function */
    template <typename Ry, typename... Args>
    inline int MetaCall(xLuaState* l, Ry(*func)(Args...)) {
        if (!CheckParams<Args...>(l, 1))
            return 0;
        int index = 0;
        l->Push(func(LoadParam<typename param::Perify<Args>::type>(l, ++index)...));
        return 1;
    }

    template <typename... Args>
    inline int MetaCall(xLuaState* l, void(*func)(Args...)) {
        if (!CheckParams<Args...>(l, 1))
            return 0;
        int index = 0;
        func(LoadParam<typename param::Perify<Args>::type>(l, ++index)...);
        return 0;
    }

    /* extend member function */
    template <typename Ty, typename Ry, typename... Args>
    inline int MetaCall(xLuaState* l, Ty* obj, Ry(*func)(Ty*, Args...)) {
        if (!CheckParams<Args...>(l, 2))
            return 0;
        int index = 1;
        l->Push(func(obj, LoadParam<typename param::Perify<Args>::type>(l, ++index)...));
        return 1;
    }

    template <typename Ty>
    inline int MetaCall(xLuaState* l, Ty* obj, int(*func)(Ty*, xLuaState*)) {
        return func(obj, l);
    }

    template <typename Ty, typename... Args>
    inline int MetaCall(xLuaState* l, Ty* obj, void(*func)(Ty*, Args...)) {
        if (!CheckParams<Args...>(l, 2))
            return 0;
        int index = 1;
        func(obj, LoadParam<typename param::Perify<Args>::type>(l, ++index)...);
        return 0;
    }

    /* normal member function */
    template <typename Obj, typename Ty, typename Ry, typename... Args>
    inline int MetaCall(xLuaState* l, Obj* obj, Ry(Ty::*func)(Args...)) {
        if (!CheckParams<Args...>(l, 2))
            return 0;
        int index = 1;
        l->Push((obj->*func)(LoadParam<typename param::Perify<Args>::type>(l, ++index)...));
        return 1;
    }

    template <typename Obj, typename Ty, typename Ry, typename... Args>
    inline int MetaCall(xLuaState* l, Obj* obj, Ry(Ty::*func)(Args...) const) {
        if (!CheckParams<Args...>(l, 2))
            return 0;
        int index = 1;
        l->Push((obj->*func)(LoadParam<typename param::Perify<Args>::type>(l, ++index)...));
        return 1;
    }

    template <typename Obj, typename Ty, typename... Args>
    inline int MetaCall(xLuaState* l, Obj* obj, void (Ty::*func)(Args...)) {
        if (!CheckParams<Args...>(l, 2))
            return 0;
        int index = 1;
        (obj->*func)(LoadParam<typename param::Perify<Args>::type>(l, ++index)...);
        return 0;
    }

    template <typename Obj, typename Ty, typename... Args>
    inline int MetaCall(xLuaState* l, Obj* obj, void (Ty::*func)(Args...) const) {
        if (!CheckParams<Args...>(l, 2))
            return 0;
        int index = 1;
        (obj->*func)(LoadParam<typename param::Perify<Args>::type>(l, ++index)...);
        return 0;
    }

    template <typename Obj, typename Ty>
    inline int MetaCall(xLuaState* l, Obj* obj, int (Ty::*func)(xLuaState*)) {
        return (obj->*func)(l);
    }

    template <typename Obj, typename Ty>
    inline int MetaCall(xLuaState* l, Obj* obj, int (Ty::*func)(xLuaState*) const) {
        return (obj->*func)(l);
    }

    /* 设置字符串数组 */
    inline void MetaSetArray(xLuaState* l, char* buf, size_t sz) {
        const char* s = l->Load<const char*>(3);    // 1:obj, 2:name, 3:value
        if (s)
            snprintf(buf, sz, s);
        else
            buf[0] = 0;
    }

    /* 暂不支持其它类型数组 */
    template <typename Ty>
    inline void MetaSetArray(xLuaState* l, Ty* buf, size_t sz) {
        static_assert(std::is_same<char, Ty>::value, "only support char array");
    }

    template <typename Ry>
    inline void MetaGet(xLuaState* l, Ry* data) {
        l->Push(*data);
    }

    template <typename Ry>
    inline void MetaSet_(xLuaState* l, Ry* data, std::true_type) {
        static_assert(std::extent<Ry>::value > 0, "array size must greater than 0");
        MetaSetArray(l, *data, std::extent<Ry>::value);
    }

    template <typename Ry>
    inline void MetaSet_(xLuaState* l, Ry* data, std::false_type) {
        *data = l->Load<typename std::decay<Ry>::type>(3);
    }

    template <typename Ry>
    inline void MetaSet(xLuaState* l, Ry* data) {
        MetaSet_(l, data, std::is_array<Ry>());
    }

    template <typename Ry>
    inline void MetaGet(xLuaState* l, Ry (*data)()) {
        l->Push(data());
    }

    template <typename Ry>
    inline void MetaSet(xLuaState* l, void (*data)(Ry)) {
        data(l->Load<typename std::decay<Ry>::type>(1));
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaGet(xLuaState* l, Obj* obj, Ry Ty::* data) {
        l->Push(static_cast<Ry>(obj->*data));
    }

    template <typename Ty, typename Ry>
    inline void MetaSet_(xLuaState* l, Ty* obj, Ry Ty::*data, std::true_type) {
        static_assert(std::extent<Ry>::value > 0, "array size must greater than 0");
        MetaSetArray(l , obj->*data, std::extent<Ry>::value);
    }

    template <typename Ty, typename Ry>
    inline void MetaSet_(xLuaState* l, Ty* obj, Ry Ty::*data, std::false_type) {
        obj->*data = l->Load<typename std::decay<Ry>::type>(3);
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaSet(xLuaState* l, Obj* obj, Ry Ty::*data) {
        MetaSet_(l, static_cast<Ty*>(obj), data, std::is_array<Ry>());
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaGet(xLuaState* l, Obj* obj, Ry(Ty::*func)()) {
        l->Push((obj->*func)());
    }

    template <typename Obj, typename Ty, typename Ry>
    inline void MetaSet(xLuaState* l, Obj* obj, void(Ty::*func)(Ry)) {
        (obj->*func)(l->Load<typename std::decay<Ry>::type>(1));
    }

    template <typename Ty, typename Ry>
    inline void MetaGet(xLuaState* l, Ty* obj, Ry (*func)(Ty*)) {
        l->Push(func(obj));
    }

    template <typename Ty, typename Ry>
    inline void MetaSet(xLuaState* l, Ty* obj, void (*func)(Ty*, Ry)) {
        func(obj, l->Load<typename std::decay<Ry>::type>(1));
    }

    template <typename Ty>
    inline void MetaGet(xLuaState* l, Ty* obj, int(Ty::*func)(xLuaState*)) {
        (obj->*func)(l);
    }

    template <typename Ty>
    inline void MetaGet(xLuaState* l, Ty* obj, int(Ty::*func)(xLuaState*) const) {
        (obj->*func)(l);
    }

    template <typename Ty>
    inline void MetaSet(xLuaState* l, Ty* obj, int(Ty::*func)(xLuaState*)) {
        (obj->*func)(l);
    }

    template <typename Ty>
    inline void MetaSet(xLuaState* l, Ty* obj, void(Ty::*func)(xLuaState*)) {
        (obj->*func)(l);
    }

    template <typename Ty>
    inline void MetaGet(xLuaState* l, Ty* obj, int(*func)(Ty*, xLuaState*)) {
        func(obj, l);
    }

    template <typename Ty>
    inline void MetaSet(xLuaState* l, Ty* obj, int(*func)(Ty*, xLuaState*)) {
        func(obj, l);
    }

    template <typename Ty>
    inline void MetaSet(xLuaState* l, Ty* obj, void(*func)(Ty*, xLuaState*)) {
        func(obj, l);
    }

    /* 支持类的成员函数, 静态函数 */
    template <typename Ty>
    struct MetaFunc {
        template <typename Fy, typename EnableIfT<std::is_member_function_pointer<Fy>::value> = 0>
        static inline int Call(xLuaState* l, const TypeInfo* info, Fy f) {
            Ty* obj = static_cast<Ty*>(GetMetaCallObj(l, info));
            if (obj)
                return MetaCall(l, obj, f);
            LogError("call object is nill");
            l->LogCallStack();
            return 0;
        }

        template <typename Fy, typename EnableIfT<!std::is_member_function_pointer<Fy>::value> = 0>
        static inline int Call(xLuaState* l, const TypeInfo* info, Fy f) {
            return MetaCall(l, f);
        }
    };

    template <typename Ty>
    struct MetaFuncEx {
        template <typename Fy>
        static inline int Call(xLuaState* l, const TypeInfo* info, Fy f) {
            Ty* obj = static_cast<Ty*>(GetMetaCallObj(l, info));
            if (obj)
                return MetaCall(l, obj, f);
            LogError("call object is nill");
            l->LogCallStack();
            return 0;
        }
    };

    template <typename Ty>
    struct MetaVar {
        template <typename Fy, typename EnableIfT<IsMember<Fy>::value, int> = 0>
        static inline void Get(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, Fy f) {
            MetaGet(l, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(dst, obj, src)), f);
        }

        template <typename Fy, typename EnableIfT<IsMember<Fy>::value, int> = 0>
        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, Fy f) {
            MetaSet(l, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(dst, obj, src)), f);
        }

        template <typename Fy, typename EnableIfT<!IsMember<Fy>::value, int> = 0>
        static inline void Get(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, Fy f) {
            MetaGet(l, f);
        }

        template <typename Fy, typename EnableIfT<!IsMember<Fy>::value, int> = 0>
        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, Fy f) {
            MetaSet(l, f);
        }

        static inline void Get(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, std::nullptr_t) {
            assert(false);
        }

        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, std::nullptr_t) {
            assert(false);
        }
    };

    template <typename Ty>
    struct MetaVarEx {
        template <typename Fy>
        static inline void Get(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, Fy f) {
            MetaGet(l, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(dst, obj, src)), f);
        }

        template <typename Fy>
        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, Fy f) {
            MetaSet(l, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(dst, obj, src)), f);
        }

        static inline void Get(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, std::nullptr_t) {
            assert(false);
        }

        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, std::nullptr_t) {
            assert(false);
        }
    };
} // namespace detail

XLUA_NAMESPACE_END
