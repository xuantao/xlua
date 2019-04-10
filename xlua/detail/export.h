﻿#pragma once
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
            if (!ud->IsLightData()) {
                LogError("unknown obj");
                return false;
            }

            const TypeInfo* src = nullptr;
            void* src_ptr = nullptr;
            if (ud->IsInternalType()) {
                auto* ary_obj = GlobalVar::GetInstance()->GetArrayObj(ud->index_);
                if (ary_obj == nullptr || ary_obj->serial_num_ != ud->serial_) {
                    LogError("current obj is nil");
                    return false;
                }

                src_ptr = ary_obj->obj_;
                src = ary_obj->info_;
            }
            else {
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
                }
                else {
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
            }
            else {
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
            }
            else if (ud->type_ == UserDataCategory::kObjPtr) {
                if (ud->info_->is_weak_obj) {
                    if (ud->serial_ != ::xLuaGetWeakObjSerialNum(ud->index_)) {
                        LogError("obj is nil");
                        return false;
                    }

                    d = ToDerivedWeakPtr<Ty>(::xLuaGetWeakObjPtr(ud->index_));
                }
                else {
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

    inline const char* PerifyMemberName(const char* name) {
        while (const char* sub = ::strstr(name, "::"))
            name = sub + 2;

        // remove prefix: "m_"
        if (name[0] == 'm' && name[1] == '_')
            name += 2;

        // remove prefix: "lua_"
        if ((name[0] == 'l' || name[0] == 'L') &&
            (name[1] == 'u' || name[1] == 'U') &&
            (name[2] == 'a' || name[2] == 'A')
            ) {
            name += 3;
            if (name[0] == '_')
                ++name;
        }
        return name;
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
        template <typename Ry, typename...Args>
        inline bool CheckMetaParam(xLuaState* l, int index, LogBuf& lb, Ry(*func)(Args...)) {
            return CheckParam<Args...>(l, index, lb);
        }

        template <typename Ty, typename Ry, typename...Args>
        inline bool CheckMetaParam(xLuaState* l, int index, LogBuf& lb, Ry(Ty::*func)(Args...)) {
            return CheckParam<Args...>(l, index, lb);
        }

        template <typename Ty, typename Ry, typename...Args>
        inline bool CheckMetaParam(xLuaState* l, int index, LogBuf& lb, Ry(Ty::*func)(Args...) const) {
            return CheckParam<Args...>(l, index, lb);
        }

        template <typename Ry>
        inline bool CheckMetaParam(xLuaState* l, int index, LogBuf& lb, Ry* var) {
            return IsType<typename std::decay<Ry>::type>(l, index, 1, lb);
        }

        template <typename Ty, typename Ry>
        inline bool CheckMetaParam(xLuaState* l, int index, LogBuf& lb, Ry Ty::*var) {
            return IsType<typename std::decay<Ry>::type>(l, index, 1, lb);
        }

        inline bool CheckMetaParam(xLuaState* l, int index, int(*func)(xLuaState*)) { return true; }

        template <typename Ty>
        inline bool CheckMetaParam(xLuaState* l, int index, int(Ty::*func)(xLuaState*)) { return true; }

        template <typename Ty>
        inline bool CheckMetaParam(xLuaState* l, int index, int(Ty::*func)(xLuaState*) const) { return true; }

        template <typename Ty>
        inline bool CheckMetaParamEx(xLuaState* l, int index, int(*func)(Ty*, xLuaState*)) { return true; }

        template <typename Ty, typename Ry, typename...Args>
        inline bool CheckMetaParamEx(xLuaState* l, int index, LogBuf& lb, Ry(*func)(Ty*, Args...)) {
            return CheckParam<Args...>(l, index, lb);
        }
    } // namespace param

    template <bool value, typename Ty = int>
    using EnableIfT = typename std::enable_if<value, int>::type;

    /* global function */
    template <typename Ry, typename... Args>
    inline int MetaCall(xLuaState* l, Ry(*func)(Args...)) {
        int index = 0;
        l->Push(func(param::LoadParam<typename param::ParamType<Args>::type>(l, ++index)...));
        return 1;
    }

    template <typename... Args>
    inline int MetaCall(xLuaState* l, void(*func)(Args...)) {
        int index = 0;
        func(param::LoadParam<typename param::ParamType<Args>::type>(l, ++index)...);
        return 0;
    }

    /* extend member function */
    template <typename Ty, typename Ry, typename... Args>
    inline int MetaCall(xLuaState* l, Ty* obj, Ry(*func)(Ty*, Args...)) {
        int index = 1;
        l->Push(func(obj, param::LoadParam<typename param::ParamType<Args>::type>(l, ++index)...));
        return 1;
    }

    template <typename Ty>
    inline int MetaCall(xLuaState* l, Ty* obj, int(*func)(Ty*, xLuaState*)) {
        return func(obj, l);
    }

    template <typename Ty, typename... Args>
    inline int MetaCall(xLuaState* l, Ty* obj, void(*func)(Ty*, Args...)) {
        int index = 1;
        func(obj, param::LoadParam<typename param::ParamType<Args>::type>(l, ++index)...);
        return 0;
    }

    /* normal member function */
    template <typename Obj, typename Ty, typename Ry, typename... Args>
    inline int MetaCall(xLuaState* l, Obj* obj, Ry(Ty::*func)(Args...)) {
        int index = 1;
        l->Push((obj->*func)(param::LoadParam<typename param::ParamType<Args>::type>(l, ++index)...));
        return 1;
    }

    template <typename Obj, typename Ty, typename Ry, typename... Args>
    inline int MetaCall(xLuaState* l, Obj* obj, Ry(Ty::*func)(Args...) const) {
        int index = 1;
        l->Push((obj->*func)(param::LoadParam<typename param::ParamType<Args>::type>(l, ++index)...));
        return 1;
    }

    template <typename Obj, typename Ty, typename... Args>
    inline int MetaCall(xLuaState* l, Obj* obj, void (Ty::*func)(Args...)) {
        int index = 1;
        (obj->*func)(param::LoadParam<typename param::ParamType<Args>::type>(l, ++index)...);
        return 0;
    }

    template <typename Obj, typename Ty, typename... Args>
    inline int MetaCall(xLuaState* l, Obj* obj, void (Ty::*func)(Args...) const) {
        int index = 1;
        (obj->*func)(param::LoadParam<typename param::ParamType<Args>::type>(l, ++index)...);
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
        static inline int Call(xLuaState* l, const TypeInfo* info, const char* func_name, Fy f) {
            LogBufCache<> lb;
            Ty* obj = static_cast<Ty*>(GetMetaCallObj(l, info));
            if (obj == nullptr) {
                l->GetCallStack(lb);
                luaL_error(l->GetState(), "attempt call function[%s:%s] failed, obj is nil\n%s", info->type_name, func_name, lb.Finalize());
                return 0;
            }
            if (!param::CheckMetaParam(l, 2, lb, f)) {
                l->GetCallStack(lb);
                luaL_error(l->GetState(), "attempt call function[%s:%s] failed, parameter is not allow\n%s", info->type_name, func_name, lb.Finalize());
                return 0;
            }

            return MetaCall(l, obj, f);
        }

        template <typename Fy, typename EnableIfT<!std::is_member_function_pointer<Fy>::value> = 0>
        static inline int Call(xLuaState* l, const TypeInfo* info, const char* func_name, Fy f) {
            LogBufCache<> lb;
            if (!param::CheckMetaParam(l, 1, lb, f)) {
                l->GetCallStack(lb);
                luaL_error(l->GetState(), "attempt call function[%s:%s] failed, parameter is not allow\n%s", info->type_name, func_name, lb.Finalize());
                return 0;
            }

            return MetaCall(l, f);
        }
    };

    template <typename Ty>
    struct MetaFuncEx {
        template <typename Fy>
        static inline int Call(xLuaState* l, const TypeInfo* info, const char* func_name, Fy f) {
            LogBufCache<> lb;
            Ty* obj = static_cast<Ty*>(GetMetaCallObj(l, info));
            if (obj == nullptr) {
                l->GetCallStack(lb);
                luaL_error(l->GetState(), "attempt call function[%s:%s] failed, obj is nil\n%s", info->type_name, func_name, lb.Finalize());
                return 0;
            }
            if (!param::CheckMetaParamEx(l, 2, lb, f)) {
                l->GetCallStack(lb);
                luaL_error(l->GetState(), "attempt call function[%s:%s] failed, parameter is not allow\n%s", info->type_name, func_name, lb.Finalize());
                return 0;
            }

            return MetaCall(l, obj, f);
        }
    };

    template <typename Ty>
    struct MetaVar {
        template <typename Fy, typename EnableIfT<IsMember<Fy>::value, int> = 0>
        static inline void Get(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, Fy f) {
            MetaGet(l, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(dst, obj, src)), f);
        }

        template <typename Fy, typename EnableIfT<IsMember<Fy>::value, int> = 0>
        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, const char* var, Fy f) {
            //LogBufCache<> lb;
            //if (!param::CheckMetaParam(l, 3, lb, f)) {
            //    l->GetCallStack(lb);
            //    luaL_error(l->GetState(), "attempt set var [%s.%s] failed, parameter is not allow\n%s", dst->type_name, var, lb.Finalize());
            //} else {
                MetaSet(l, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(dst, obj, src)), f);
            //}
        }

        template <typename Fy, typename EnableIfT<!IsMember<Fy>::value, int> = 0>
        static inline void Get(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, Fy f) {
            MetaGet(l, f);
        }

        template <typename Fy, typename EnableIfT<!IsMember<Fy>::value, int> = 0>
        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, const char* var, Fy f) {
            //LogBufCache<> lb;
            //if (!param::CheckMetaParam(l, 3, lb, f)) {
            //    l->GetCallStack(lb);
            //    luaL_error(l->GetState(), "attempt set var [%s.%s] failed, parameter is not allow\n%s", dst->type_name, var, lb.Finalize());
            //} else {
                MetaSet(l, f);
            //}
        }

        static inline void Get(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, std::nullptr_t) {
            assert(false);
        }

        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, const char* var, std::nullptr_t) {
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
        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, const char* var, Fy f) {
            //LogBufCache<> lb;
            //if (!param::CheckMetaParamEx(l, 3, lb, f)) {
            //    l->GetCallStack(lb);
            //    luaL_error(l->GetState(), "attempt set var [%s.%s] failed, parameter is not allow\n%s", dst->type_name, var, lb.Finalize());
            //} else {
                MetaSet(l, static_cast<Ty*>(_XLUA_TO_SUPER_PTR(dst, obj, src)), f);
            //}
        }

        static inline void Get(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, std::nullptr_t) {
            assert(false);
        }

        static inline void Set(xLuaState* l, void* obj, const TypeInfo* src, const TypeInfo* dst, const char* var, std::nullptr_t) {
            assert(false);
        }
    };
} // namespace detail

XLUA_NAMESPACE_END
