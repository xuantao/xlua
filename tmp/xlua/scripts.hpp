
/* const value table's metatable */
static const char* kConstMetatable = R"V0G0N(
return {
    __newindex = function(t, n, v)
        print("not allow modify const table member");
    end
}
)V0G0N";

/* global metatable */
static const char* kGlobalMetatable = R"V0G0N(
local indexer, md_name, funcs, vars = ...
return {
    __index = function(tb, name)
        local f = funcs[name]
        if f then
            return f
        end

        local v = vars[name]
        if v then
            return get(v)
        else
            print("not find")
        end
    end,

    __newindex = function(tb, name, value)
        local v = vars[name]
        if v then
            set(v)
        else
            print("not find")
        end
    end,

    __tostring = function (tb)
        return md_name
    end,

    __pairs = function (tb)
        -- TODO:
    end,
}
)V0G0N";

/* declared type metatable */
static const char* kDeclaredMetatable = R"V0G0N(
local get_, set_ type_name, vars, funcs = ...
return {
    __index = function (obj, name)
        local f = funcs[name]
        if f then
            return f
        end

        local v = vars[name]
        if v then
            return get_var(obj, v)
        end
        print("type[%s] does not own member[%s]", type_name, name)
        return nil
    end,

    __newindex = function (obj, name, var)
        local v = vars[name]
        if v then
            set_var(obj, v, var)
            return
        end

        if funcs[name] then
            print("can not modify type:[%s] member function:[%s]", type_name, name)
        else
            print("type[%s] does not own member[%s]", type_name, name)
        end
    end,

    __gc = function (obj)
        --TODO:
    end,

    __tostring == function (obj)
        return type_name
    end,

    __pairs = function ()
        --TODO:
    end
}
)V0G0N";

/* light user data metatable */
static const char* kLudMetatable = R"V0G0N(
local parser, get, set, types = ...
return {
    __index = function (ptr, name)
        local id = parser(ptr)
        local t = types[id]
        if not t then
            print("error")
            return
        end

        local f = t.funcs[name]
        if f then
            return f
        end

        local v = t.vars[name]
        if v then
            return get(ptr, v)
        end

        print("not find")
        return
    end,

    __newindex = function (ptr, name, value)
        local id = parser(ptr)
        local t = types[id]
        if not t then
            print("error")
            return
        end

        local v = t.vars[name]
        if v then
            set(ptr, v)
        else
            print("not find");
        end
    end,

    __pairs = function (ptr)
        --TODO:
    end
}
)V0G0N";

