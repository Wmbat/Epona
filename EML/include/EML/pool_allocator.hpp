#pragma once

#include <EML/allocator_interface.hpp>
#include <EML/allocator_utils.hpp>

#include <cassert>
#include <cstdint>

namespace EML
{
   class pool_allocator final : public allocator_interface
   {
   private:
      struct header
      {
         header* p_next;
      };

   public:
      pool_allocator( std::size_t const block_count, std::size_t const block_size ) :
         block_count( block_count ), block_size( block_size ), pool_size( block_size * block_count ),
         p_memory( new std::byte[block_count * block_size * sizeof( header )] ), p_first_free( nullptr )
      {
         assert( block_count != 0 && "Cannot have no blocks in memory pool" );
         assert( block_size != 0 && "Cannot have a block size of zero" );

         p_first_free = reinterpret_cast<header*>( p_memory );
         auto* p_base_cpy = p_first_free;

         for ( int i = 1; i < block_count; ++i )
         {
            std::size_t offset = i * ( block_size + sizeof( header ) );

            auto* p_new = reinterpret_cast<header*>( p_memory + offset );
            p_base_cpy->p_next = p_new;
            p_base_cpy = p_new;
            p_base_cpy->p_next = nullptr;
         }
      }

      [[nodiscard]] std::byte* allocate( std::size_t size, std::size_t alignment ) noexcept override
      {
         assert( size != 0 && "Allocation size cannot be zero" );
         assert( size == 1024 && "Allocation size does not match pool block size" );

         if ( p_first_free )
         {
            std::byte* p_chunk_header = reinterpret_cast<std::byte*>( p_first_free );

            p_first_free = p_first_free->p_next;

            used_memory += block_size;
            ++num_allocations;

            return reinterpret_cast<std::byte*>( p_chunk_header + sizeof( header ) );
         }
         else
         {
            return nullptr;
         }
      }

      void free( std::byte* p_location ) noexcept override
      {
         assert( p_location != nullptr && "cannot free a nullptr" );

         auto* p_header = reinterpret_cast<header*>( p_location - sizeof( header ) );
         p_header->p_next = p_first_free;
         p_first_free = p_header;

         used_memory -= block_size;
         --num_allocations;
      }

   private:
      std::size_t const pool_size;
      std::size_t const block_count;
      std::size_t const block_size;

      std::byte* p_memory;
      header* p_first_free;
   };
} // namespace EML
