//#include <type_traits>

#define LUA_EXPORT __attribute__((annotate("xlua")))

struct TestExport {
	LUA_EXPORT void Call(int p_int) {
	}
	
	LUA_EXPORT int CallWithRet(void) {
		return 1;
	}
};

namespace utility {
	 class LUA_EXPORT StringView {
		size_t len{0};
		const char* str{nullptr};
	};
	
	struct Vec2 {
		float x;
		float y
	};
	
	int Compare(StringView l, StringView r) {
		return l.str < r.str;
	}
	
	template <typename Ty>
	struct Traits {
		//static constexpr bool value = std::is_class<Ty>::value;
	};
}
