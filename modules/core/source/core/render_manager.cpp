/**
 * @file render_manager.hpp
 * @author wmbat wmbat@protonmail.com.
 * @date Tuesday, May 5th, 2020.
 * @copyright MIT License.
 */

#include <core/render_manager.hpp>

#include <vkn/device.hpp>
#include <vkn/physical_device.hpp>
#include <vkn/shader.hpp>

#include <util/containers/dynamic_array.hpp>
#include <util/logger.hpp>

namespace core
{
   auto handle_instance_error(const vkn::error&, util::logger* const) -> vkn::instance;
   auto handle_physical_device_error(const vkn::error&, util::logger* const)
      -> vkn::physical_device;
   auto handle_device_error(const vkn::error&, util::logger* const) -> vkn::device;
   auto handle_surface_error(const vkn::error&, util::logger* const) -> vk::SurfaceKHR;
   auto handle_swapchain_error(const vkn::error&, util::logger* const) -> vkn::swapchain;

   render_manager::render_manager(gfx::window* const p_wnd, util::logger* const plogger) :
      mp_window{p_wnd}, mp_logger{plogger}, m_loader{mp_logger}
   {
      m_instance =
         vkn::instance::builder{m_loader, mp_logger}
            .set_application_name("")
            .set_application_version(0, 0, 0)
            .set_engine_name(m_engine_name)
            .set_engine_version(CORE_VERSION_MAJOR, CORE_VERSION_MINOR, CORE_VERSION_PATCH)
            .build()
            .left_map([plogger](auto&& err) {
               return handle_instance_error(err, plogger);
            })
            .join();

      m_device =
         vkn::device::builder{m_loader,
                              vkn::physical_device::selector{m_instance, mp_logger}
                                 .set_surface(mp_window->get_surface(m_instance.value())
                                                 .left_map([plogger](auto&& err) {
                                                    return handle_surface_error(err, plogger);
                                                 })
                                                 .join())
                                 .set_preferred_gpu_type(vkn::physical_device::type::discrete)
                                 .allow_any_gpu_type()
                                 .require_present()
                                 .select()
                                 .left_map([plogger](auto&& err) {
                                    return handle_physical_device_error(err, plogger);
                                 })
                                 .join(),
                              m_instance.version(), mp_logger}
            .build()
            .left_map([plogger](auto&& err) {
               return handle_device_error(err, plogger);
            })
            .join();

      m_swapchain =
         vkn::swapchain::builder{m_device, plogger}
            .set_desired_format({vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear})
            .set_desired_present_mode(vk::PresentModeKHR::eMailbox)
            .add_fallback_present_mode(vk::PresentModeKHR::eFifo)
            .set_clipped(true)
            .set_composite_alpha_flags(vk::CompositeAlphaFlagBitsKHR::eOpaque)
            .build()
            .left_map([plogger](auto&& err) {
               return handle_swapchain_error(err, plogger);
            })
            .join();

      m_render_pass = vkn::render_pass::builder{m_device, m_swapchain, plogger}
                         .build()
                         .left_map([plogger](auto&& err) {
                            log_error(plogger, "[core] Failed to create render pass: \"{0}\"",
                                      err.type.message());
                            abort();

                            return vkn::render_pass{};
                         })
                         .join();

      m_framebuffers.reserve(std::size(m_swapchain.image_views()));
      for (const auto& img_view : m_swapchain.image_views())
      {
         m_framebuffers.emplace_back(vkn::framebuffer::builder{m_device, m_render_pass, plogger}
                                        .add_attachment(img_view)
                                        .set_buffer_height(m_swapchain.extent().width)
                                        .set_buffer_width(m_swapchain.extent().height)
                                        .set_layer_count(1u)
                                        .build()
                                        .left_map([plogger](vkn::error&& err) {
                                           log_error(plogger,
                                                     "[core] Failed to create framebuffer: \"{0}\"",
                                                     err.type.message());
                                           abort();

                                           return vkn::framebuffer{};
                                        })
                                        .join());
      }

      m_shader_codex =
         shader_codex::builder{m_device, plogger}
            .add_shader_filepath("resources/shaders/test_shader.vert")
            .add_shader_filepath("resources/shaders/test_shader.frag")
            .allow_caching()
            .build()
            .left_map([plogger](auto&& err) {
               log_error(plogger, "[core] Failed to create shader codex: \"{0}\"", err.message());
               abort();

               return shader_codex{};
            })
            .join();

      m_command_pool =
         vkn::command_pool::builder{m_device, plogger}
            .set_queue_family_index(m_device.get_queue_index(vkn::queue::type::graphics))
            .set_primary_buffer_count(std::size(m_framebuffers))
            .build()
            .left_map([plogger](auto&& err) {
               log_error(plogger, "[core] Failed to create command pool: \"{0}\"",
                         err.type.message());

               abort();

               return vkn::command_pool{};
            })
            .join();

      m_graphics_pipeline =
         vkn::graphics_pipeline::builder{m_device, m_render_pass, plogger}
            .add_shader(m_shader_codex.get_shader("test_shader.vert"))
            .add_shader(m_shader_codex.get_shader("test_shader.frag"))
            .add_viewport({.x = 0.0f,
                           .y = 0.0f,
                           .width = static_cast<float>(m_swapchain.extent().width),
                           .height = static_cast<float>(m_swapchain.extent().height),
                           .minDepth = 0.0f,
                           .maxDepth = 1.0f},
                          {.offset = {0, 0}, .extent = m_swapchain.extent()})
            .set_topology(vk::PrimitiveTopology::eTriangleList)
            .enable_primitive_restart(false)
            .build()
            .left_map([&](vkn::error&& err) {
               log_error(plogger, "[core] Failed to create graphics pipeline: \"{0}\"",
                         err.type.message());

               abort();

               return vkn::graphics_pipeline{};
            })
            .join();

      util::log_info(mp_logger, "[core] recording main rendering command buffers");
      for (std::size_t i = 0; const auto& buffer : m_command_pool.primary_cmd_buffers())
      {
         buffer.begin({.pNext = nullptr, .flags = {}, .pInheritanceInfo = nullptr});

         const auto clear_colour = vk::ClearValue{std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}};
         buffer.beginRenderPass({.pNext = nullptr,
                                 .renderPass = m_render_pass.value(),
                                 .framebuffer = m_framebuffers[i++].value(),
                                 .renderArea = {{0, 0}, m_swapchain.extent()},
                                 .clearValueCount = 1,
                                 .pClearValues = &clear_colour},
                                vk::SubpassContents::eInline);

         buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphics_pipeline.value());

         buffer.draw(3u, 1u, 0u, 0u);

         buffer.endRenderPass();
         buffer.end();
      }
   }

   void render_manager::render_frame() {}

   auto handle_instance_error(const vkn::error& err, util::logger* const plogger) -> vkn::instance
   {
      log_error(plogger, "[core] Failed to create instance: {0}", err.type.message());
      abort();

      return {};
   }
   auto handle_physical_device_error(const vkn::error& err, util::logger* const plogger)
      -> vkn::physical_device
   {
      log_error(plogger, "[core] Failed to create physical device: {0}", err.type.message());
      abort();

      return {};
   }
   auto handle_device_error(const vkn::error& err, util::logger* const plogger) -> vkn::device
   {
      log_error(plogger, "[core] Failed to create device: {0}", err.type.message());
      abort();

      return {};
   }

   auto handle_surface_error(const vkn::error& err, util::logger* const plogger) -> vk::SurfaceKHR
   {
      log_error(plogger, "[core] Failed to create surface: {0}", err.type.message());
      abort();

      return {};
   }
   auto handle_swapchain_error(const vkn::error& err, util::logger* const plogger) -> vkn::swapchain
   {
      log_error(plogger, "[core] Failed to create swapchain: {0}", err.type.message());
      abort();

      return {};
   }
} // namespace core
