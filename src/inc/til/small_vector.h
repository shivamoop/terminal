// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <cassert>
#include <memory>

namespace til
{
    template<typename T, size_t N>
    class small_vector
    {
    public:
        static_assert(N != 0, "A small_vector without a small buffer isn't be very useful");
        static_assert(std::is_nothrow_move_constructible_v<T>, "small_vector::_grow doesn't guard the malloc pointer for exceptions");

        using value_type = T;
        using allocator_type = std::allocator<T>;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        using iterator = pointer;
        using const_iterator = const_pointer;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        small_vector() :
            _data{ &_buffer[0] }
        {
        }

        template<typename InputIt>
        small_vector(InputIt first, InputIt last) :
            small_vector{}
        {
            for (; first != last; ++first)
            {
                push_back(*first);
            }
        }

        ~small_vector()
        {
            std::destroy_n(_data, _size);
            if (_capacity != N)
            {
                std::free(_data);
            }
        }

        constexpr size_type max_size() const noexcept { return static_cast<size_t>(-1) / sizeof(T); }

        constexpr pointer data() noexcept { return _data; }
        constexpr const_pointer data() const noexcept { return _data; }
        constexpr size_type capacity() const noexcept { return _capacity; }
        constexpr size_type size() const noexcept { return _size; }
        constexpr size_type empty() const noexcept { return !_data; }

        constexpr iterator begin() noexcept { return _data; }
        constexpr const_iterator begin() const noexcept { return _data; }
        constexpr const_iterator cbegin() const noexcept { return _data; }

        constexpr iterator end() noexcept { return _data + _size; }
        constexpr const_iterator end() const noexcept { return _data + _size; }
        constexpr const_iterator cend() const noexcept { return _data + _size; }

        bool operator==(const small_vector& other) const noexcept
        {
            return std::equal(begin(), end(), other.begin(), other.end());
        }

        void swap(small_vector& other) noexcept
        {
            if (_capacity == N && other._capacity == N)
            {
                std::swap_ranges(_data, _capacity, other._data);
            }
        }

        reference front() noexcept
        {
            assert(_size != 0);
            return _data[0];
        }

        const_reference front() const noexcept
        {
            assert(_size != 0);
            return _data[0];
        }

        reference back() noexcept
        {
            assert(_size != 0);
            return _data[_size - 1];
        }

        const_reference back() const noexcept
        {
            assert(_size != 0);
            return _data[_size - 1];
        }

        reference operator[](size_type off) noexcept
        {
            assert(off < _size);
            return _data[off];
        }

        const_reference operator[](size_type off) const noexcept
        {
            assert(off < _size);
            return _data[off];
        }

        reference at(size_type off)
        {
            if (off >= _size)
            {
                _throw_invalid_subscript();
            }
            return _data[off];
        }

        const_reference at(size_type off) const
        {
            if (off >= _size)
            {
                _throw_invalid_subscript();
            }
            return _data[off];
        }

        void clear() noexcept
        {
            std::destroy_n(_data, _size);

            if (_capacity != N)
            {
                std::free(_data);
            }

            _data = &_buffer[0];
            _capacity = N;
            _size = 0;
        }

        void resize(size_type newSize)
        {
            if (newSize < _size)
            {
                std::destroy_n(_data + newSize, _size - newSize);
            }
            else if (newSize > _size)
            {
                if (newSize > _capacity)
                {
                    _grow(newSize - _capacity);
                }

                std::uninitialized_value_construct_n(_data + _size, newSize - _size);
            }

            _size = newSize;
        }

        void resize(size_type newSize, const_reference value)
        {
            if (newSize < _size)
            {
                std::destroy_n(_data + newSize, _size - newSize);
            }
            else if (newSize > _size)
            {
                if (newSize > _capacity)
                {
                    _grow(newSize - _capacity);
                }

                std::uninitialized_fill_n(_data + _size, newSize - _size, value);
            }

            _size = newSize;
        }

        void shrink_to_fit()
        {
            if (_capacity == N || _size == _capacity)
            {
                return;
            }

            auto data = &_buffer[0];
            if (_size > N)
            {
                data = static_cast<T*>(std::malloc(_size * sizeof(T)));
                if (!data)
                {
                    _throw_alloc();
                }
            }

            std::uninitialized_move_n(_data, _size, data);
            std::free(_data);

            _data = data;
            _capacity = _size;
        }

        void push_back(const T& value)
        {
            if (_size == _capacity)
            {
                _grow(1);
            }

            new (_data + _size) T(value);
            _size++;
        }

        void push_back(T&& value)
        {
            if (_size == _capacity)
            {
                _grow(1);
            }

            new (_data + _size) T(value);
            _size++;
        }

        template<typename... Args>
        reference emplace_back(Args&&... args)
        {
            if (_size == _capacity)
            {
                _grow(1);
            }

            const auto& ref = new (_data + _size) T(std::forward<Args>(args)...);
            _size++;
            return ref;
        }

        iterator erase(const_iterator pos)
        {
            assert(pos >= begin() && pos < end());
            std::destroy_at(_data + pos);
        }

        iterator erase(const_iterator first, const_iterator last)
        {
            if (first >= last)
            {
                return last;
            }

            assert(first >= begin() && last <= end());
            const auto it1 = end();
            const auto it0 = std::move(last, it1, first);
            std::destroy(it0, it1);
            _size = it0 - _data;
        }

    private:
        [[noreturn]] void _throw_invalid_subscript() const
        {
            throw std::out_of_range("invalid small_vector subscript");
        }

        [[noreturn]] void _throw_alloc() const
        {
            throw std::bad_alloc();
        }

        [[noreturn]] void _throw_too_long() const
        {
            throw std::length_error("small_vector too long");
        }

        void _grow(size_type add)
        {
            const auto cap = _capacity;
            const auto newCap = cap + std::max(add, cap / 2);

            if (newCap < cap || newCap > max_size())
            {
                _throw_too_long();
            }

            const auto data = static_cast<T*>(std::malloc(newCap * sizeof(T)));
            if (!data)
            {
                _throw_alloc();
            }

            std::uninitialized_move_n(_data, _size, data);

            if (_capacity != N)
            {
                std::free(_data);
            }

            _data = data;
            _capacity = newCap;
        }

        void _alloc()
        {
            const auto data = std::malloc(capacity * sizeof(T));
            if (!data)
            {
                _throw_alloc();
            }
        }

        T* _data;
        size_t _capacity = N;
        size_t _size = 0;
        T _buffer[N];
    };
}
