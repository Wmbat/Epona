#include <vkn/render_pass.hpp>

#include <monads/try.hpp>

namespace vkn
{
   namespace detail
   {
      auto to_string(render_pass::error err) -> std::string
      {
         using error = render_pass::error;

         switch (err)
         {
            case error::no_device_provided:
               return "no_device_provided";
            case error::failed_to_create_render_pass:
               return "failed_to_create_render_pass";
            default:
               return "UNKNOWN";
         }
      };
   } // namespace detail

   auto render_pass::error_category::name() const noexcept -> const char*
   {
      return "vkn_render_pass";
   }
   auto render_pass::error_category::message(int err) const -> std::string
   {
      return detail::to_string(static_cast<render_pass::error>(err));
   }

   render_pass::render_pass(create_info&& info) noexcept :
      m_render_pass{std::move(info.render_pass)}, m_swapchain_format{info.format}
   {}

   auto render_pass::operator->() noexcept -> pointer { return &m_render_pass.get(); }
   auto render_pass::operator->() const noexcept -> const_pointer { return &m_render_pass.get(); }

   auto render_pass::operator*() const noexcept -> value_type { return value(); }

   render_pass::operator bool() const noexcept { return m_render_pass.get(); }

   auto render_pass::value() const noexcept -> vk::RenderPass { return m_render_pass.get(); }
   auto render_pass::device() const noexcept -> vk::Device { return m_render_pass.getOwner(); }

   using builder = render_pass::builder;

   builder::builder(const vkn::device& device, const vkn::swapchain& swapchain,
                    util::logger* plogger) noexcept :
      m_device{device.value()},
      m_swapchain_format{swapchain.format()}, m_swapchain_extent{swapchain.extent()}, m_plogger{
                                                                                         plogger}
   {}

   auto builder::build() -> vkn::result<render_pass>
   {
      if (!m_device)
      {
         return monad::make_error(make_error(error::no_device_provided, {}));
      }

      const std::array attachment_descriptions{
         vk::AttachmentDescription{.flags = {},
                                   .format = m_swapchain_format,
                                   .loadOp = vk::AttachmentLoadOp::eClear,
                                   .storeOp = vk::AttachmentStoreOp::eStore,
                                   .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
                                   .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
                                   .initialLayout = vk::ImageLayout::eUndefined,
                                   .finalLayout = vk::ImageLayout::ePresentSrcKHR}};

      const std::array attachment_references{vk::AttachmentReference{
         .attachment = 0, .layout = vk::ImageLayout::eColorAttachmentOptimal}};

      const std::array subpass_descriptions{
         vk::SubpassDescription{.flags = {},
                                .pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
                                .inputAttachmentCount = 0u,
                                .pInputAttachments = nullptr,
                                .colorAttachmentCount = attachment_references.size(),
                                .pColorAttachments = attachment_references.data(),
                                .pResolveAttachments = nullptr,
                                .pDepthStencilAttachment = nullptr,
                                .preserveAttachmentCount = 0u,
                                .pPreserveAttachments = nullptr}};

      const std::array subpass_dependencies{
         vk::SubpassDependency{.srcSubpass = VK_SUBPASS_EXTERNAL,
                               .dstSubpass = 0,
                               .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                               .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
                               .srcAccessMask = {},
                               .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
                               .dependencyFlags = {}}};

      const auto pass_info = vk::RenderPassCreateInfo{}
                                .setPNext(nullptr)
                                .setFlags({})
                                .setDependencyCount(std::size(subpass_dependencies))
                                .setPDependencies(std::data(subpass_dependencies))
                                .setAttachmentCount(std::size(attachment_descriptions))
                                .setPAttachments(std::data(attachment_descriptions))
                                .setSubpassCount(std::size(subpass_descriptions))
                                .setPSubpasses(std::data(subpass_descriptions));

      return monad::try_wrap<vk::SystemError>([&] {
                return m_device.createRenderPassUnique(pass_info);
             })
         .map_error([](auto&& err) {
            return make_error(error::failed_to_create_render_pass, err.code());
         })
         .map([&](auto&& handle) {
            util::log_info(m_plogger, "[vkn] render pass created");

            return render_pass{{.render_pass = std::move(handle), .format = m_swapchain_format}};
         });
   } // namespace vkn
} // namespace vkn
