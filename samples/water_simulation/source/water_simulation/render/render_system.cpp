#include <water_simulation/render/render_system.hpp>

#include <gfx/data_types.hpp>

#include <utility>

auto to_util_error(ui::error_t&& err) -> util::error_t
{
   return {err.value()};
}

struct render_system_data
{
   render_system::create_info& info;

   vkn::context context;
   vkn::device device;
   vkn::swapchain swapchain;

   semaphore_array render_finished_semaphores{};

   std::array<vkn::command_pool, max_frames_in_flight> render_command_pools;
   std::array<vkn::semaphore, max_frames_in_flight> image_available_semaphores{};
   std::array<vkn::fence, max_frames_in_flight> in_flight_fences{};

   depth_buffer buffer;

   ui::window* p_window;

   std::shared_ptr<util::logger> logger;
};

auto create_context(render_system_data&& data) -> result<render_system_data>
{
   return vkn::context::make({.p_logger = data.logger}).map([&](vkn::context&& ctx) {
      data.context = std::move(ctx);
      return std::move(data);
   });
}

auto create_device(render_system_data&& data) -> result<render_system_data>
{
   return data.p_window->get_surface(data.context.instance())
      .map_error(to_util_error)
      .and_then([&](vk::UniqueSurfaceKHR&& surface) {
         return data.context.select_device(std::move(surface)).map([&](vkn::device&& device) {
            data.device = std::move(device);
            return std::move(data);
         });
      });
}

auto create_swapchain(render_system_data&& data) -> result<render_system_data>
{
   return vkn::swapchain::builder{data.device, data.logger}
      .set_desired_format({vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear})
      .set_desired_present_mode(vk::PresentModeKHR::eMailbox)
      .add_fallback_present_mode(vk::PresentModeKHR::eFifo)
      .set_clipped(true)
      .set_composite_alpha_flags(vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .build()
      .map([&](vkn::swapchain&& swap) {
         data.swapchain = std::move(swap);

         return std::move(data);
      });
}

auto create_render_command_pools(render_system_data&& data) -> result<render_system_data>
{
   std::array<vkn::command_pool, max_frames_in_flight> pools;

   for (auto& pool : pools)
   {
      auto result =
         data.device.get_queue_index(vkn::queue_type::graphics).and_then([&](std::uint32_t i) {
            return vkn::command_pool::builder{data.device, data.logger}
               .set_queue_family_index(i)
               .set_primary_buffer_count(1U)
               .build();
         });

      if (auto err = result.error())
      {
         return monad::err(err.value());
      }

      pool = std::move(result).value().value();
   }

   data.render_command_pools = std::move(pools);

   return std::move(data);
}

auto create_in_flight_fences(render_system_data&& data) -> result<render_system_data>
{
   std::array<vkn::fence, max_frames_in_flight> fences;
   for (auto& fence : fences)
   {
      if (auto res = vkn::fence::builder{data.device, data.info.p_logger}.set_signaled().build())
      {
         fence = std::move(res).value().value();
      }
      else
      {
         return monad::err(res.error().value());
      }
   }

   data.in_flight_fences = std::move(fences);

   return std::move(data);
}

auto create_image_available_semaphores(render_system_data&& data) -> result<render_system_data>
{
   std::array<vkn::semaphore, max_frames_in_flight> semaphores;
   for (auto& semaphore : semaphores)
   {
      if (auto res = vkn::semaphore::builder{data.device, data.info.p_logger}.build())
      {
         semaphore = std::move(res).value().value();
      }
      else
      {
         return monad::err(res.error().value());
      }
   }

   data.image_available_semaphores = std::move(semaphores);

   return std::move(data);
}

auto create_render_finished_semaphores(render_system_data&& data) -> result<render_system_data>
{
   util::small_dynamic_array<vkn::semaphore, vkn::expected_image_count.value()> semaphores;

   for ([[maybe_unused]] const auto& _ : data.swapchain.image_views())
   {
      if (auto res = vkn::semaphore::builder{data.device, data.info.p_logger}.build())
      {
         semaphores.emplace_back(std::move(res).value().value());
      }
      else
      {
         return monad::err(res.error().value());
      }
   }

   data.render_finished_semaphores = std::move(semaphores);

   return std::move(data);
}

auto create_depth_buffer(render_system_data&& data) -> result<render_system_data>
{
   auto& extent = data.swapchain.extent();
   return depth_buffer::make(
             {.device = data.device, .width = extent.width, .height = extent.height})
      .map([&](depth_buffer&& b) {
         data.buffer = std::move(b);
         return std::move(data);
      });
}

auto render_system::make(create_info&& info) -> util::result<render_system>
{
   return create_context({.info = info, .p_window = info.p_window, .logger = info.p_logger})
      .and_then(create_device)
      .and_then(create_swapchain)
      .and_then(create_render_command_pools)
      .and_then(create_in_flight_fences)
      .and_then(create_image_available_semaphores)
      .and_then(create_render_finished_semaphores)
      .and_then(create_depth_buffer)
      .map([](render_system_data&& data) {
         render_system rs{};
         rs.mp_logger = data.logger;
         rs.mp_window = data.p_window;
         rs.m_context = std::move(data.context);
         rs.m_device = std::move(data.device);
         rs.m_swapchain = std::move(data.swapchain);
         rs.m_in_flight_fences = std::move(data.in_flight_fences);
         rs.m_image_available_semaphores = std::move(data.image_available_semaphores);
         rs.m_render_finished_semaphores = std::move(data.render_finished_semaphores);
         rs.m_render_command_pools = std::move(data.render_command_pools);
         rs.m_depth_buffer = std::move(data.buffer);
         rs.m_configuration = {.swapchain_image_count = std::size(rs.m_swapchain.image_views())};

         rs.m_images_in_flight.resize(std::size(rs.m_swapchain.image_views()));

         return rs;
      });

   /*
      .and_then([](render_system&& sys) {
         return create_sync_primitives(sys.m_device, sys.m_swapchain, sys.mp_logger)
            .map([&](detail::sync_data&& data) {
               sys.m_render_finished_semaphores = std::move(data.render_finished_semaphores);
               sys.m_image_available_semaphores = std::move(data.image_available_semaphores);
               sys.m_in_flight_fences = std::move(data.in_flight_fences);
               sys.m_images_in_flight.resize(std::size(sys.m_swapchain.image_views()), {nullptr});

               return std::move(sys);
            });
      })
      */
}

auto render_system::begin_frame() -> image_index_t
{
   m_device.logical().waitForFences(
      {vkn::value(m_in_flight_fences.at(m_current_frame_index.value()))}, true,
      std::numeric_limits<std::uint64_t>::max());

   auto [image_res, image_index] = m_device.logical().acquireNextImageKHR(
      vkn::value(m_swapchain), std::numeric_limits<std::uint64_t>::max(),
      vkn::value(m_image_available_semaphores.at(m_current_frame_index.value())), nullptr);
   if (image_res != vk::Result::eSuccess)
   {
      abort();
   }

   util::log_debug(mp_logger, R"([gfx] swapchain image "{}" acquired)", image_index);

   m_device.logical().resetCommandPool(
      m_render_command_pools[m_current_frame_index.value()].value(), {}); // NOLINT

   m_current_image_index = image_index;

   return m_current_image_index;
}

void render_system::render(std::span<render_pass> passes)
{
   util::log_debug(mp_logger, R"([gfx] render command pool "{}" buffer recording)",
                   m_current_frame_index.value());

   std::array<vk::ClearValue, 2> clear_values{};
   clear_values[0].color = {std::array{0.0F, 0.0F, 0.0F, 0.0F}};
   clear_values[1].depthStencil = {1.0f, 0};

   for (const auto& buffer :
        m_render_command_pools[m_current_frame_index.value()].primary_cmd_buffers()) // NOLINT
   {
      buffer.begin({.pNext = nullptr, .flags = {}, .pInheritanceInfo = nullptr});

      for (auto& pass : passes)
      {
         pass.submit_render_calls(buffer, m_current_image_index.value(),
                                  {{0, 0}, m_swapchain.extent()}, clear_values);
      }

      buffer.end();
   }
}

void render_system::end_frame()
{
   if (m_images_in_flight[m_current_image_index.value()])
   {
      m_device.logical().waitForFences(
         {vkn::value(m_images_in_flight.at(m_current_frame_index.value()))}, true,
         std::numeric_limits<std::uint64_t>::max());
   }
   m_images_in_flight[m_current_image_index.value()] =
      vkn::value(m_in_flight_fences.at(m_current_frame_index.value()));

   const std::array wait_semaphores{
      vkn::value(m_image_available_semaphores.at(m_current_frame_index.value()))};
   const std::array signal_semaphores{
      vkn::value(m_render_finished_semaphores.at(m_current_image_index.value()))};
   const std::array command_buffers{
      m_render_command_pools[m_current_frame_index.value()].primary_cmd_buffers()[0]}; // NOLINT
   const std::array<vk::PipelineStageFlags, 1> wait_stages{
      vk::PipelineStageFlagBits::eColorAttachmentOutput};

   m_device.logical().resetFences(
      {vkn::value(m_in_flight_fences.at(m_current_frame_index.value()))});

   const std::array submit_infos{
      vk::SubmitInfo{.waitSemaphoreCount = std::size(wait_semaphores),
                     .pWaitSemaphores = std::data(wait_semaphores),
                     .pWaitDstStageMask = std::data(wait_stages),
                     .commandBufferCount = std::size(command_buffers),
                     .pCommandBuffers = std::data(command_buffers),
                     .signalSemaphoreCount = std::size(signal_semaphores),
                     .pSignalSemaphores = std::data(signal_semaphores)}};

   try
   {
      const auto gfx_queue = *m_device.get_queue(vkn::queue_type::graphics).value();
      gfx_queue.submit(submit_infos,
                       vkn::value(m_in_flight_fences.at(m_current_frame_index.value())));
   }
   catch (const vk::SystemError& err)
   {
      util::log_error(mp_logger, "[core] failed to submit graphics queue");
      abort();
   }

   const std::array swapchains{vkn::value(m_swapchain)};

   const auto present_queue = *m_device.get_queue(vkn::queue_type::present).value();
   if (present_queue.presentKHR({.waitSemaphoreCount = std::size(signal_semaphores),
                                 .pWaitSemaphores = std::data(signal_semaphores),
                                 .swapchainCount = std::size(swapchains),
                                 .pSwapchains = std::data(swapchains),
                                 .pImageIndices = &m_current_image_index.value()}) !=
       vk::Result::eSuccess)
   {
      util::log_error(mp_logger, "[core] failed to present present queue");
      abort();
   }

   m_current_frame_index = (m_current_frame_index + 1) % max_frames_in_flight;
}

void render_system::wait()
{
   m_device.logical().waitIdle();
}

auto render_system::device() const -> const vkn::device&
{
   return m_device;
}
auto render_system::device() -> vkn::device&
{
   return m_device;
}
auto render_system::swapchain() const -> const vkn::swapchain&
{
   return m_swapchain;
}
auto render_system::swapchain() -> vkn::swapchain&
{
   return m_swapchain;
}

auto render_system::get_depth_attachment() const -> vk::ImageView
{
   return m_depth_buffer.view();
}

auto render_system::vertex_bindings() -> vertex_bindings_array
{
   return {
      {.binding = 0, .stride = sizeof(gfx::vertex), .inputRate = vk::VertexInputRate::eVertex}};
}
auto render_system::vertex_attributes() -> vertex_attributes_array
{
   return {{.location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(gfx::vertex, position)},
           {.location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(gfx::vertex, normal)},
           {.location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(gfx::vertex, colour)}};
}

auto render_system::viewport() const -> vk::Viewport
{
   return {.x = 0.0F,
           .y = 0.0F,
           .width = static_cast<float>(m_swapchain.extent().width),
           .height = static_cast<float>(m_swapchain.extent().height),
           .minDepth = 0.0F,
           .maxDepth = 1.0F};
}
auto render_system::scissor() const -> vk::Rect2D
{
   return {.offset = {0, 0}, .extent = m_swapchain.extent()};
}

auto render_system::create_vertex_buffer(const util::dynamic_array<gfx::vertex>& vertices) const
   -> util::result<gfx::vertex_buffer>
{
   return gfx::vertex_buffer::make({.vertices = vertices,
                                    .device = m_device,
                                    .command_pool = m_render_command_pools[0],
                                    .p_logger = mp_logger});
}
auto render_system::create_index_buffer(const util::dynamic_array<std::uint32_t>& indices) const
   -> util::result<gfx::index_buffer>
{
   return gfx::index_buffer::make({.indices = indices,
                                   .device = m_device,
                                   .command_pool = m_render_command_pools[0],
                                   .p_logger = mp_logger});
}

auto render_system::lookup_configuration() const -> const config&
{
   return m_configuration;
}
