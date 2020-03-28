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

#include <EWL/widget.hpp>

#include <EGL/render_manager.hpp>

#include <GLFW/glfw3.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace EWL
{
   class window : public widget
   {
      using wnd_ptr = std::unique_ptr<GLFWwindow, std::function<void( GLFWwindow* )>>;

   public:
      window( std::string_view title_in, std::uint32_t width_in, std::uint32_t height_in );

      bool is_open( );

   private:
      std::string title;
      std::uint32_t width;
      std::uint32_t height;

      wnd_ptr p_wnd;

      inline static bool IS_GLFW_INITIALIZED = false;
   };
} // namespace EWL
