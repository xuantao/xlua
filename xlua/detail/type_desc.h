﻿#pragma once
#include "common.h"
#include <vector>
#include <string>

XLUA_NAMESPACE_BEGIN

namespace detail
{
    struct TypeDesc : public ITypeDesc
    {
        friend class GlobalVar;
    public:
        TypeDesc(GlobalVar* mgr, TypeCategory category, const char* name, bool has_obj_index, bool is_wak_obj, const TypeInfo* super);
        virtual ~TypeDesc() { }

    public:
        void SetCaster(ITypeCaster* caster) override { caster_ = caster; }
        void AddMember(const char* name, LuaFunction func, bool global) override;
        void AddMember(const char* name, LuaIndexer getter, LuaIndexer setter, bool global) override;
        const TypeInfo* Finalize() override;

    private:
        bool CheckRename(const char* name, bool global) const;

    private:
        GlobalVar* mgr_ = nullptr;
        TypeCategory category_;
        std::string name_;
        bool has_obj_index_ = false;
        bool is_weak_obj_ = false;
        const TypeInfo* super_ = nullptr;
        ITypeCaster* caster_ = nullptr;
        std::vector<MemberVar> vars_;
        std::vector<MemberFunc> funcs_;
        std::vector<MemberVar> global_vars_;
        std::vector<MemberFunc> global_funcs_;
    };
} // namespace detail

XLUA_NAMESPACE_END
