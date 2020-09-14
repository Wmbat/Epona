/**
 * MIT License
 *
 * Copyright (c) 2020 Wmbat
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <compare>
#include <cstdint>
#include <iterator>

namespace util
{
   /**
    * @class random_access_iterator random_access_iterator.hpp
    * <ESL/utils/iterators/random_access_iterator.hpp>
    * @author wmbat wmbat@protonmail.com
    * @date Tuesday, April 7th, 2020
    * @brief An iterator used for random access in containers.
    * @copyright MIT License
    *
    * @tparam  type_    The type of elements the iterator will operate over.
    */
   template <typename type_>
   class random_access_iterator
   {
   public:
      using iterator_category = std::contiguous_iterator_tag;
      using self_type = random_access_iterator;
      using value_type = type_;
      using reference = type_&;
      using pointer = type_*;
      using difference_type = std::ptrdiff_t;

   public:
      constexpr random_access_iterator() noexcept = default;
      constexpr explicit random_access_iterator(pointer p_type) noexcept : p_type(p_type) {}

      constexpr auto operator==(self_type const& rhs) const noexcept -> bool = default;
      constexpr auto operator<=>(self_type const& rhs) const noexcept
         -> std::strong_ordering = default;

      constexpr auto operator*() const noexcept -> reference
      {
         assert(p_type != nullptr && "Cannot use derefence a nullptr");

         return *p_type;
      }
      constexpr auto operator->() const noexcept -> pointer
      {
         assert(p_type != nullptr && "Cannot use derefence a nullptr");

         return p_type;
      }

      constexpr auto operator++() noexcept -> self_type&
      {
         ++p_type;

         return *this;
      }
      constexpr auto operator--() noexcept -> self_type&
      {
         --p_type;

         return *this;
      }

      constexpr auto operator++(int) const noexcept -> self_type
      {
         self_type it = *this;
         ++*this;

         return it;
      }
      constexpr auto operator--(int) const noexcept -> self_type
      {
         self_type it = *this;
         --*this;

         return it;
      }

      constexpr auto operator+=(difference_type diff) noexcept -> self_type&
      {
         this->p_type += diff;

         return *this;
      }
      constexpr auto operator-=(difference_type diff) noexcept -> self_type&
      {
         this->p_type -= diff;

         return *this;
      }

      constexpr auto operator+(difference_type rhs) const noexcept -> self_type
      {
         self_type it = *this;
         it += rhs;

         return it;
      }
      constexpr auto operator-(difference_type rhs) const noexcept -> self_type
      {
         self_type it = *this;
         it -= rhs;

         return it;
      }

      constexpr auto operator+(self_type const& it) const noexcept -> difference_type
      {
         return p_type + it.p_type;
      }
      constexpr auto operator-(self_type const& it) const noexcept -> difference_type
      {
         return p_type - it.p_type;
      }

      friend constexpr auto operator+(difference_type lhs, const self_type& rhs) noexcept
         -> self_type
      {
         return {rhs + lhs};
      }
      friend constexpr auto operator-(difference_type lhs, const self_type& rhs) noexcept
         -> self_type
      {
         return {rhs - lhs};
      }

      constexpr auto operator[](difference_type diff) const noexcept -> reference
      {
         assert(p_type != nullptr && "Cannot use derefence a nullptr");

         return *(*this + diff);
      }

      constexpr void swap(self_type& rhs) noexcept { std::swap(p_type, rhs.p_type); }

   private:
      pointer p_type{nullptr};
   };
} // namespace util
