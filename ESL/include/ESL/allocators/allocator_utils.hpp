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

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>

#define TO_BYTE_PTR( ptr ) reinterpret_cast<std::byte*>( ptr )

namespace ESL
{
   template <class type_>
   using auto_ptr = std::unique_ptr<type_, std::function<void( type_* )>>;

   constexpr std::size_t get_backward_padding( std::uintptr_t address, std::size_t aligment ) noexcept
   {
      auto const padding = address & ( aligment - 1 );

      return padding == aligment ? 0 : padding;
   }

   constexpr std::size_t get_forward_padding( std::uintptr_t address, std::size_t alignment ) noexcept
   {
      auto const padding = alignment - ( address & ( alignment - 1 ) );

      return padding == alignment ? 0 : padding;
   }

   constexpr std::size_t get_forward_padding(
      std::uintptr_t address, std::size_t alignment, std::size_t header_size ) noexcept
   {
      auto padding = get_forward_padding( address, alignment );

      if ( padding < header_size )
      {
         auto const needed_space = header_size - padding;

         padding += alignment * ( needed_space / alignment );
         if ( needed_space % alignment > 0 )
         {
            padding += alignment;
         }
      }

      return padding;
   }

   namespace allocator_type
   {
      template <typename type_, typename = void>
      struct has_allocate : std::false_type
      {
      };

      template <typename type_>
      struct has_allocate<type_,
         typename std::enable_if_t<std::is_same_v<decltype( std::declval<type_>( ).allocate(
                                                     std::declval<std::size_t>( ), std::declval<std::size_t>( ) ) ),
            std::byte*>>> : std::true_type
      {
      };

      template <typename type_, typename = void>
      struct has_free : std::false_type
      {
      };

      template <typename type_>
      struct has_free<type_,
         typename std::enable_if_t<
            std::is_same_v<decltype( std::declval<type_>( ).free( std::declval<std::byte*>( ) ) ), void>>> :
         std::true_type
      {
      };

      template <typename type_, typename = void>
      struct has_reallocate : std::false_type
      {
      };

      template <typename type_, typename = void>
      struct has_can_allocate : std::false_type
      {
      };

      template <typename type_>
      struct has_can_allocate<type_,
         typename std::enable_if_t<std::is_same_v<decltype( std::declval<type_>( ).can_allocate(
                                                     std::declval<std::size_t>( ), std::declval<std::size_t>( ) ) ),
            bool>>> : std::true_type
      {
      };

      template <typename type_>
      struct has_reallocate<type_,
         typename std::enable_if_t<std::is_same_v<decltype( std::declval<type_>( ).reallocate( ) ), std::byte*>>> :
         std::true_type
      {
      };
   } // namespace allocator_type
} // namespace ESL
