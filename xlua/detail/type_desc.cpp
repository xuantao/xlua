#include "type_desc.h"
#include "export.h"
#include "global.h"
#include <string.h>
#include <memory>
#include <algorithm>

XLUA_NAMESPACE_BEGIN

namespace detail {
    static std::string PerifyTypeName(const char* name) {
        char buf[XLUA_MAX_TYPE_NAME_LENGTH];
        PerifyTypeName(buf, XLUA_MAX_TYPE_NAME_LENGTH, name);
        return std::string(buf);
    }

    TypeDesc::TypeDesc(GlobalVar* mgr, TypeCategory category, const char* name, bool has_obj_index, bool is_wak_obj, const TypeInfo* super)
        : mgr_(mgr)
        , category_(category)
        , has_obj_index_(has_obj_index)
        , is_weak_obj_(is_wak_obj)
        , super_(super) {
        name_ = PerifyTypeName(name);
    }

    void TypeDesc::AddMember(const char* name, LuaFunction func, bool global) {
        auto& vec = global ? global_funcs_ : funcs_;
        name = PerifyMemberName(name);
        CheckRename(name, global);
        vec.push_back(MemberFunc{name, func});
    }

    void TypeDesc::AddMember(const char* name, LuaIndexer getter, LuaIndexer setter, bool global) {
        auto& vec = global ? global_vars_ : vars_;
        name = PerifyMemberName(name);
        CheckRename(name, global);
        vec.push_back(MemberVar{name, getter, setter});
    }

    const TypeInfo* TypeDesc::Finalize() {
        std::unique_ptr<TypeDesc> ptr(this);  // auto free
        TypeInfo* info = (TypeInfo*)mgr_->SerialAlloc(sizeof(TypeInfo));

        info->category = category_;
        info->super = super_;
        info->has_obj_index = has_obj_index_;
        info->is_weak_obj = is_weak_obj_;
        info->caster = caster_;

        info->child = nullptr;
        if (super_) {
            info->brother = super_->child;
            const_cast<TypeInfo*>(super_)->child = info;
        } else {
            info->brother = nullptr;
        }

        char* type_name = (char*)mgr_->SerialAlloc(name_.length()+1);
        snprintf(type_name, name_.length() + 1, name_.c_str());
        info->type_name = type_name;

        vars_.push_back(MemberVar{nullptr, nullptr, nullptr});
        info->vars = (MemberVar*)mgr_->SerialAlloc(sizeof(MemberVar) * vars_.size());
        for (size_t i = 0; i < vars_.size(); ++i)
            info->vars[i] = vars_[i];

        funcs_.push_back(MemberFunc{nullptr, nullptr});
        info->funcs = (MemberFunc*)mgr_->SerialAlloc(sizeof(MemberFunc) * funcs_.size());
        for (size_t i = 0; i < funcs_.size(); ++i)
            info->funcs[i] = funcs_[i];

        global_vars_.push_back(MemberVar{nullptr, nullptr, nullptr});
        info->global_vars = (MemberVar*)mgr_->SerialAlloc(sizeof(MemberVar) * global_vars_.size());
        for (size_t i = 0; i < global_vars_.size(); ++i)
            info->global_vars[i] = global_vars_[i];

        global_funcs_.push_back(MemberFunc{nullptr, nullptr});
        info->global_funcs = (MemberFunc*)mgr_->SerialAlloc(sizeof(MemberFunc) * global_funcs_.size());
        for (size_t i = 0; i < global_funcs_.size(); ++i)
            info->global_funcs[i] = global_funcs_[i];

        mgr_->AddTypeInfo(info);
        return info;
    }

    bool TypeDesc::CheckRename(const char* name, bool global) const {
        const auto& funcs = global ? global_funcs_ : funcs_;
        const auto& vars = global ? global_vars_: vars_;

        auto it_func = std::find_if(funcs.cbegin(), funcs.cend(), [name](const MemberFunc& f) {
            return ::strcmp(name, f.name) == 0;
        });
        auto it_var = std::find_if(vars.cbegin(), vars.cend(), [name](const MemberVar& f) {
            return ::strcmp(name, f.name) == 0;
        });

        if (it_func != funcs.cend() || it_var != vars.cend()) {
            LogError("type:[%s] export same member:[%s]", name_.c_str(), name);
            return false;
        }
        return true;
    }
} // namespace detail

XLUA_NAMESPACE_END
