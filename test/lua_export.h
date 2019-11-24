#pragma once
#include "example.h"
#include <vector>
#include <xlua_state.h>

XLUA_DECLARE_CLASS(ShapeBase);
XLUA_DECLARE_CLASS(Triangle);
XLUA_DECLARE_CLASS(Square);
XLUA_DECLARE_CLASS(WeakObj);
XLUA_DECLARE_CLASS(TestExportParams);
XLUA_DECLARE_CLASS(Global::TestStaticParams);

XLUA_DECLARE_CLASS(IRenderer);
XLUA_DECLARE_CLASS(Widget);
XLUA_DECLARE_CLASS(Image);
XLUA_DECLARE_CLASS(Label);
XLUA_DECLARE_CLASS(Button);
XLUA_DECLARE_CLASS(LifeTime);
XLUA_DECLARE_CLASS(Object);
XLUA_DECLARE_CLASS(Character);
XLUA_DECLARE_CLASS(Doodad);

XLUA_DECLARE_CLASS(ExportObj);
XLUA_DECLARE_CLASS(TestMember);

XLUA_NAMESPACE_BEGIN

template<>
struct Support<Color> : ValueCategory<Color, false> {
    static inline const char* Name() { return "Color"; }

    static inline bool Check(State* s, int index) {
        return s->GetType(index) == VarType::kTable;
    }

    static inline Color Load(State* s, int index) {
        Color c;
        c.a = s->GetField<int>(index, "a");
        c.r = s->GetField<int>(index, "r");
        c.g = s->GetField<int>(index, "g");
        c.b = s->GetField<int>(index, "b");
        return c;
    }

    static inline void Push(State* s, Color c) {
        s->NewTable();
        s->SetField(-1, "a", c.a);
        s->SetField(-1, "r", c.r);
        s->SetField(-1, "g", c.g);
        s->SetField(-1, "b", c.b);
    }
};

template<>
struct Support<Vec2> : ValueCategory<Vec2, false> {
    static inline const char* Name() { return "Vec2"; }

    static inline bool Check(State* s, int index) {
        return s->GetType(index) == VarType::kTable;
    }

    static inline Vec2 Load(State* s, int index) {
        Vec2 c;
        c.x = s->GetField<float>(index, "x");
        c.y = s->GetField<float>(index, "y");
        return c;
    }

    static inline void Push(State* s, Vec2 v) {
        s->NewTable();
        s->SetField(-1, "x", v.x);
        s->SetField(-1, "y", v.y);
    }
};

template<>
struct Support<Ray> : ValueCategory<Ray, false> {
    static inline const char* Name() { return "Vec2"; }

    static inline bool Check(State* s, int index) {
        return s->GetType(index) == VarType::kTable;
    }

    static inline Ray Load(State* s, int index) {
        Ray r;
        r.orign = s->GetField<Vec2>(index, "orign");
        r.dir = s->GetField<Vec2>(index, "dir");
        return r;
    }

    static inline void Push(State* s, Ray r) {
        s->NewTable();
        s->SetField(-1, "orign", r.orign);
        s->SetField(-1, "dir", r.dir);
    }
};

template <typename Ty>
struct Support<WeakObjPtr<Ty>> : ValueCategory<WeakObjPtr<Ty>, true> {
    typedef typename SupportTraits<Ty*>::supporter supporter;

    static inline const char* Name() { return supporter::Name(); }
    static inline bool Check(State* s, int index) { return supporter::Check(s, index); }

    static inline WeakObjPtr<Ty> Load(State* s, int index) {
        return WeakObjPtr<Ty>(supporter::Load(s, index));
    }
    static inline void Push(State* s, const WeakObjPtr<Ty>& ptr) {
        supporter::Push(s, ptr.GetPtr());
    }
};

XLUA_NAMESPACE_END

struct weak_obj_tag_ex {};

template <typename Ty, typename std::enable_if<std::is_base_of<WeakObj, Ty>::value, int>::type = 0>
inline const xlua::WeakObjProc xLuaQueryWeakObjProc(xlua::Identity<Ty>) {
    return xlua::WeakObjProc{
        typeid(weak_obj_tag_ex).hash_code(),
        [](void* ptr) ->xlua::WeakObjRef {
            int index = WeakObjArray::Instance().AllocIndex(static_cast<Ty*>(ptr));
            int serial = WeakObjArray::Instance().GetSerialNumber(index);
            return xlua::WeakObjRef{index, serial};
        },
        [](xlua::WeakObjRef ref) -> void* {
            if (ref.serial != WeakObjArray::Instance().GetSerialNumber(ref.index))
                return nullptr;
            return static_cast<Ty*>(WeakObjArray::Instance().IndexToObject(ref.index));
        }
    };
}
