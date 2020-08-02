/**
 * @file render_manager.hpp
 * @author wmbat wmbat@protonmail.com.
 * @date Tuesday, May 5th, 2020.
 * @copyright MIT License.
 */

#pragma once

#include <util/containers/dense_hash_map.hpp>

#include <core/graphics/gui/window.hpp>

#include <vkn/core.hpp>
#include <vkn/device.hpp>
#include <vkn/instance.hpp>
#include <vkn/shader.hpp>
#include <vkn/swapchain.hpp>

#include <util/logger.hpp>

namespace core
{
   class render_manager
   {
   public:
      render_manager(gfx::window* const p_wnd, util::logger* const p_logger = nullptr);

   private:
      [[maybe_unused]] gfx::window* const m_pwindow;
      util::logger* const m_plogger;

      std::string m_engine_name = "Epona";

      vkn::loader m_loader;
      vkn::instance m_instance;
      vkn::device m_device;
      vkn::swapchain m_swapchain;

      vkn::shader m_vert_shader;
      vkn::shader m_frag_shader;
   };
} // namespace core
