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

#include <cstdint>
#include <iterator>

namespace ESL
{
   template <typename type_>
   class iterator
   {
   public:
      using iterator_category = std::random_access_iterator_tag;
      using self_type = iterator;
      using value_type = type_;
      using reference = type_&;
      using const_reference = type_ const&;
      using pointer = type_*;
      using const_pointer = type_ const*;
      using difference_type = std::ptrdiff_t;

   public:
      iterator( pointer p_type ) : p_type( p_type ) {}

      bool operator==( self_type rhs ) { return p_type == rhs.p_type; }
      bool operator!=( self_type rhs ) { return p_type != rhs.p_type; }

      bool operator<( self_type rhs ) { return p_type < rhs.p_type; }
      bool operator>( self_type rhs ) { return p_type > rhs.p_type; }
      bool operator<=( self_type rhs ) { return p_type <= rhs.p_type; }
      bool operator>=( self_type rhs ) { return p_type >= rhs.p_type; }

      reference operator*( ) { return *p_type; }
      const_reference operator*( ) const { return *p_type; }
      pointer operator->( ) { return p_type; }
      const_pointer operator->( ) const { return p_type; };

      self_type& operator++( )
      {
         ++p_type;

         return *this;
      }
      self_type& operator--( )
      {
         --p_type;

         return *this;
      }

      self_type operator++( int )
      {
         self_type it = *this;
         ++*this;

         return it;
      }
      self_type operator--( int )
      {
         self_type it = *this;
         --*this;

         return it;
      }

      self_type& operator+=( difference_type diff )
      {
         *this += diff;

         return *this;
      }
      self_type& operator-=( difference_type diff )
      {
         *this -= diff;

         return *this;
      }

      self_type operator+( difference_type rhs ) const
      {
         self_type it = *this;
         it += rhs;

         return it;
      }
      self_type operator-( difference_type rhs ) const
      {
         self_type it = *this;
         it -= rhs;

         return it;
      }

      difference_type operator-( self_type& it ) const { return std::distance( p_type, it ); }

      reference operator[]( difference_type diff ) { return *( *this + diff ); }
      const_reference operator[]( difference_type diff ) const { *( *this + diff ); }

      void swap( self_type& rhs ) { std::swap( p_type, rhs.get_ptr() );}

   private:
      pointer get_ptr( ) { return p_type; }
      const_pointer get_ptr( ) const { return p_type; }

   private:
      pointer p_type;
   };
} // namespace ESL
