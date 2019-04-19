#pragma once
#include "../xlua_def.h"

XLUA_NAMESPACE_BEGIN

#define _XLUA_ALIGN_SIZE(S) (S + (sizeof(void*) - S % sizeof(void*)) % sizeof(void*))

namespace detail {
    class SerialAllocator {
        struct AllocNode {
            AllocNode* next;
            size_t capacity;
        };
    public:
        SerialAllocator(size_t block)
            : block_size_(block)
            , alloc_node_(nullptr) {
        }

        ~SerialAllocator() {
            auto* node = alloc_node_;
            alloc_node_ = nullptr;
            while (node) {
                auto* tmp = node;
                node = node->next;

                delete[] reinterpret_cast<int8_t*>(node);
            }
        }

        SerialAllocator(const SerialAllocator&) = delete;
        SerialAllocator& operator = (const SerialAllocator&) = delete;

    public:
        void* Alloc(size_t s) {
            AllocNode* node = alloc_node_;
            s = _XLUA_ALIGN_SIZE(s);

            while (node && node->capacity < s)
                node = node->next;

            if (node == nullptr) {
                size_t ns = s + kNodeSize;
                ns = block_size_ * (1 + ns / block_size_);

                node = reinterpret_cast<AllocNode*>(new int8_t[ns]);
                node->next = alloc_node_;
                node->capacity = ns - kNodeSize;
            }

            node->capacity -= s;
            return reinterpret_cast<int8_t*>(node) + kNodeSize + node->capacity;
        }

    private:
        static constexpr size_t kNodeSize = _XLUA_ALIGN_SIZE(sizeof(AllocNode));

    private:
        size_t block_size_;
        AllocNode* alloc_node_;
    };
}

XLUA_NAMESPACE_END
