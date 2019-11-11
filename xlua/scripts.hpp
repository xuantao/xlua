
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
local state, desc, indexer, md_name, funcs, vars = ...
return {
    __index = function(tb, name)
        print(tb, name)
        local f = funcs[name]
        if f then
            return f
        end

        local v = vars[name]
        if v and v[1] then
            return indexer(nil, v[1], state, desc)
        else
            print("not find 1")
        end
    end,

    __newindex = function(tb, name, value)
        print(tb, name)
        local v = vars[name]
        if v and v[2] then
            indexer(value, v[2], state, desc)
        else
            print("not find 2")
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
static const char* kFudMetatable = R"V0G0N(
local state, indexer, type_name, funcs, vars = ...
return {
    __index = function (obj, name)
        local f = funcs[name]
        if f then
            return f
        end

        local var = vars[name]
        if var and var[1] then
            return indexer(nil, var[1], state, obj)
        end
        print(string.format("type[%s] does not own member[%s]", type_name, name))
        return nil
    end,

    __newindex = function (obj, name, value)
        local v = vars[name]
        if v then
            indexer(value, v[2], state, obj)
            return
        end

        if funcs[name] then
            print(string.format("can not modify type:[%s] member function:[%s]", type_name, name))
        else
            print(string.format("type[%s] does not own member[%s]", type_name, name))
        end
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
local state, unpack_ptr, indexer, types = ...
return {
    __index = function (ptr, name)
        local id, ptr, desc = unpack_ptr(ptr)
        local t = types[id]
        if not t then
            print("error 11")
            return
        end

        local f = t.funcs[name]
        if f then
            return f
        end

        local var = t.vars[name]
        if var and var[1] then
            return indexer(nil, var[1], state, ptr, desc)
        end

        print("not find 11")
        return
    end,

    __newindex = function (ptr, name, value)
        local id, ptr, desc = unpack_ptr(ptr)
        local t = types[id]
        if not t then
            print("error 22")
            return
        end

        local var = t.vars[name]
        if var and var[2] then
            indexer(value, var[2], state, ptr, desc)
        else
            print("not find 22");
        end
    end,

    __tostring = function(ptr)
        return "light user data"
    end,

    __pairs = function (ptr)
        --TODO:
    end,
}
)V0G0N";
