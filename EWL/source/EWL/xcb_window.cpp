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

#include <EWL/window.hpp>

#if defined( VK_USE_PLATFORM_XCB_KHR )
#   include <xcb/xproto.h>

namespace EWL
{
   window::window( ) : p_connection( xcb_connect( nullptr, nullptr ) )
   {
      xcb_setup_t const* p_setup = xcb_get_setup( p_connection );
      xcb_screen_iterator_t it = xcb_setup_roots_iterator( p_setup );
      xcb_screen_t* p_screen = it.data;

      xcb_window_t window = xcb_generate_id( p_connection );
      xcb_create_window( p_connection, XCB_COPY_FROM_PARENT, window, p_screen->root, 0, 0, 100, 100, 10, XCB_WINDOW_CLASS_INPUT_OUTPUT,
         p_screen->root_visual, 0, NULL );
   }
} // namespace EWL

#endif
