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

#include "epona_core/graphics/gui/window.hpp"

#include <memory>

namespace core::gfx
{
   window::window()
   {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

      p_wnd = wnd_ptr(glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                         title.c_str(), nullptr, nullptr),
         glfwDestroyWindow);
   }

   window::window(std::string_view title_in, std::uint32_t width_in, std::uint32_t height_in) :
      title(title_in), width(width_in), height(height_in)
   {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
      glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

      p_wnd = wnd_ptr(glfwCreateWindow(static_cast<int>(width), static_cast<int>(height),
                         title.c_str(), nullptr, nullptr),
         glfwDestroyWindow);
   }

   bool window::is_open() { return !glfwWindowShouldClose(p_wnd.get()); }

   vkn::result<vk::SurfaceKHR> window::get_surface(vk::Instance instance) const
   {
      VkSurfaceKHR surface = VK_NULL_HANDLE;
      const auto res = glfwCreateWindowSurface(instance, p_wnd.get(), nullptr, &surface);

      if (res != VK_SUCCESS)
      {
         return monad::to_error(vkn::error{.type = {}, .result = static_cast<vk::Result>(res)});
      }
      else
      {
         return monad::to_value(vk::SurfaceKHR{surface});
      }
   }
} // namespace core::gfx
