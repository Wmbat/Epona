#pragma once

#include <cstddef>
#include <cstdint>

namespace EML
{
   class allocator_interface
   {
   protected:
      allocator_interface( ) noexcept = default;
      allocator_interface( std::size_t size ) noexcept;
      virtual ~allocator_interface( ) noexcept = default;

   public:
      virtual std::byte* allocate( std::size_t size, std::size_t allignment = sizeof( std::size_t ) ) noexcept = 0;
      virtual void free( std::byte* p_location ) noexcept = 0;

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

      std::size_t max_size( ) const noexcept;

   protected:
      std::size_t total_size;
      std::size_t used_memory;
      std::size_t num_allocations;
   };
} // namespace EML
