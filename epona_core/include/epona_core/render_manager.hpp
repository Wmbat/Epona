/**
 * @file render_manager.hpp
 * @author wmbat wmbat@protonmail.com.
 * @date Tuesday, May 5th, 2020.
 * @copyright MIT License.
 */

#pragma once

#include <epona_core/detail/logger.hpp>
#include <epona_core/graphics/gui/window.hpp>
#include <epona_core/graphics/vkn/core.hpp>
#include <epona_core/graphics/vkn/device.hpp>
#include <epona_core/graphics/vkn/instance.hpp>
#include <epona_core/graphics/vkn/swapchain.hpp>

namespace core
{
   class render_manager
   {
   public:
      render_manager(gfx::window* const p_wnd, logger* const p_logger = nullptr);

   private:
      gfx::window* const m_pwindow;
      logger* const m_plogger;

      std::string m_engine_name = "Epona";

      gfx::vkn::loader m_loader;
      gfx::vkn::instance m_instance;
      gfx::vkn::device m_device;
      gfx::vkn::swapchain m_swapchain;
   };
} // namespace core
