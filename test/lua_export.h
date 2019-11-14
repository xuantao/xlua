#pragma once
#include "shape.h"
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

XLUA_NAMESPACE_END
