// Copyright 2025 PenguinEncounter
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef BUFFER_H
#define BUFFER_H

// provides size_t ._.
// ReSharper disable once CppUnusedIncludeDirective
#include <stddef.h> // NOLINT(*-deprecated-headers)

template<typename T> class tagged_buffer {
public:
    size_t size;
    T* ptr;

    tagged_buffer(const size_t size, T* ptr): size(size), ptr(ptr) {}

    ~tagged_buffer() {
        delete[] ptr;
    }

    tagged_buffer(const tagged_buffer&) = delete;

    tagged_buffer(tagged_buffer&& other) noexcept: size(other.size), ptr(other.ptr) {
        other.ptr = nullptr;
    }

    tagged_buffer& operator= (tagged_buffer&) = delete;
    tagged_buffer& operator= (tagged_buffer&& other) = default;

    T next() {
        T result = *ptr;
        ++ptr; --size;
        return result;
    }
};

#endif //BUFFER_H
