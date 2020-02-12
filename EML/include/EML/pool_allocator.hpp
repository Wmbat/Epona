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

#include <EML/allocator_utils.hpp>

#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>

namespace EML
{
   class pool_allocator final
   {
   private:
      struct block_header
      {
         block_header* p_next;
      };

   public:
      pool_allocator( std::size_t const block_count, std::size_t const block_size ) noexcept;

      [[nodiscard]] std::byte* allocate( std::size_t size, std::size_t alignment ) noexcept;
      void free( std::byte* p_location ) noexcept;

      template <class type_, class... args_>
      [[nodiscard]] type_* make_new( args_&&... args ) noexcept
      {
         if ( auto* p_alloc = allocate( sizeof( type_ ), alignof( type_ ) ) )
         {
            return new ( p_alloc ) type_( args... );
         }
         else
         {
            return nullptr;
         }
      }

      template <class type_>
      void make_delete( type_* p_type ) noexcept
      {
         if ( p_type )
         {
            p_type->~type_( );
            free( reinterpret_cast<std::byte*>( p_type ) );
         }
      }

      void clear( ) noexcept;

      template <class type_, class... args_>
      [[nodiscard]] auto_ptr<type_> make_unique( args_&&... args ) noexcept
      {
         return auto_ptr<type_>( make_new<type_>( args... ), [this]( type_* p_type ) {
            this->make_delete( p_type );
         } );
         /*
         if ( auto* p_alloc = allocate( sizeof( type_ ), alignof( type_ ) ) )
         {
            return auto_ptr<type_>( new ( p_alloc ) type_( args... ), [this]( type_* p_type ) {
               this->make_delete( p_type );
            } );
         }
         else
         {
            return auto_ptr<type_>( nullptr, [this]( type_* p_type ) {
               this->make_delete( p_type );
            } );
         }
         */
      }

      std::size_t max_size( ) const noexcept;
      std::size_t memory_usage( ) const noexcept;
      std::size_t allocation_count( ) const noexcept;

   private:
      std::size_t total_size;
      std::size_t used_memory;
      std::size_t num_allocations;

      std::size_t block_count = 0;
      std::size_t block_size = 0;

      std::unique_ptr<std::byte[]> p_memory;
      block_header* p_first_free = nullptr;
   };
} // namespace EML
