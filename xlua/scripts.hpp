
/* const value table's metatable */
static const char* kConstMetatable = R"V0G0N(
return {
    __newindex = function(t, n, v)
        error("not allow modify const table member");
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
        end
        error(string.format("[%s.%s] member is not exist", md_name, name))
    end,

    __newindex = function(tb, name, value)
        local f = funcs[name]
        if f then
            error(string.format("[%s.%s] member function not allow modify", md_name, name))
            return
        end

        local v = vars[name]
        if v then
            if v[2] then
                indexer(value, v[2], state, desc)
            else
                error(string.format("[%s.%s] member var is read only", md_name, name))
            end
        else
            error(string.format("[%s.%s] member var is not exist", md_name, name))
        end
    end,

    __tostring = function (tb)
        return md_name
    end,

    __pairs = function (tb)
        local idx = 1;
        local mem = {vars, funcs}
        local iter
        iter = function (tb, k)
            local v
            k, v = next(mem[idx], k)
            if v then
                if idx == 1 then
                    return k, indexer(nil, v[1], state, desc)
                else
                    return k, v
                end
            elseif mem[idx + 1] then
                idx = idx + 1
                return iter(tb, nil)
            end
        end
        return iter, tb, nil
    end,
}
)V0G0N";

/* declared type metatable */
static const char* kFudMetatable = R"V0G0N(
local state, indexer, to_string, type_name, funcs, vars = ...
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
        error(string.format("[%s.%s] member is not exist", type_name, name))
    end,

    __newindex = function (obj, name, value)
        local f = funcs[name]
        if f then
            error(string.format("[%s.%s] member function not allow modify", type_name, name))
            return
        end

        local v = vars[name]
        if v then
            if v[2] then
                indexer(value, v[2], state, obj)
            else
                error(string.format("[%s.%s] member var is read only", type_name, name))
            end
        else
            error(string.format("[%s.%s] member var is not exist", type_name, name))
        end
    end,

    __tostring == function (obj)
        return string.format("%s(%s)", type_name, to_string(obj))
    end,

    __pairs = function (obj)
        if not xlua.IsValid(obj) then
            error(string.format("invalid ud data, ptr:%s", tostring(obj)))
            return
        end

        local idx = 1;
        local mem = {vars, funcs}
        local iter
        iter = function (obj, k)
            local v
            k, v = next(mem[idx], k)
            if v then
                if idx == 1 then
                    return k, indexer(nil, v[1], state, obj)
                else
                    return k, v
                end
            elseif mem[idx + 1] then
                idx = idx + 1
                return iter(obj, nil)
            end
        end
        return iter, obj, nil
    end
}
)V0G0N";

/* light user data metatable */
static const char* kLudMetatable = R"V0G0N(
local state, unpack_ptr, indexer, to_string, types = ...
return {
    __index = function (ptr, name)
        local id, ptr, desc = unpack_ptr(ptr)
        local t = types[id]
        if not t then
            error(string.format("invalid light user data, ptr:%s", to_string(ptr)))
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

        error(string.format("[%s.%s] member var is not exist", t.name, name))
    end,

    __newindex = function (ptr, name, value)
        local id, ptr, desc = unpack_ptr(ptr)
        local t = types[id]
        if not t then
            error(string.format("invalid light user data, ptr:%s", to_string(ptr)))
            return
        end

        local f = t.funcs[name]
        if f then
            error(string.format("[%s.%s] member function not allow modify", t.name, name))
            return
        end

        local var = t.vars[name]
        if var then
            if var[2] then
                indexer(value, var[2], state, ptr, desc)
            else
                error(string.format("[%s.%s] member var is read only", t.name, name))
            end
        else
            error(string.format("[%s.%s] member var is not exist", t.name, name))
        end
    end,

    __tostring = function(ptr)
        return to_string(ptr)
    end,

    __pairs = function (ptr)
        local id, obj, desc = unpack_ptr(ptr)
        local t = types[id]
        if not t then
            error(string.format("invalid light user data, ptr:%s", to_string(ptr)))
            return
        end

        local idx = 1;
        local mem = {t.vars, t.funcs}
        local iter
        iter = function (obj, k)
            local v
            k, v = next(mem[idx], k)
            if v then
                if idx == 1 then
                    return k, indexer(nil, v[1], state, obj, desc)
                else
                    return k, v
                end
            elseif mem[idx + 1] then
                idx = idx + 1
                return iter(obj, nil)
            end
        end
        return iter, obj, nil
    end,
}
)V0G0N";
