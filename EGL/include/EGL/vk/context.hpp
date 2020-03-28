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

#include <EGL/vk/core.hpp>
#include <ESL/allocators/multipool_allocator.hpp>
#include <ESL/containers/vector.hpp>

#include <string>
#include <string_view>

namespace EGL
{
   class context
   {
   public:
      context( std::string_view app_name_in );
      context( context const& other ) = delete;
      context( context&& other );
      ~context( );

      context& operator=( context const& rhs ) = delete;
      context& operator=( context&& rhs );

   private:
      ESL::vector<char const*, ESL::multipool_allocator> get_instance_extensions( );

   private:
      ESL::multipool_allocator main_allocator;

      std::string app_name;

      VkInstance instance;

      ESL::vector<char const*, ESL::multipool_allocator> instance_extensions;

      inline static bool IS_VOLK_INIT = false;
   };
} // namespace EGL
