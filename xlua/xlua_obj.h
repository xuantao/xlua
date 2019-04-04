#pragma once
#include "xlua_def.h"

XLUA_NAMESPACE_BEGIN

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

XLUA_NAMESPACE_END
