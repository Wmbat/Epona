#include <vkn/framebuffer.hpp>

#include <monads/try.hpp>

namespace vkn
{
   namespace detail
   {
      auto to_string(vkn::framebuffer::error err) -> std::string
      {
         using error = vkn::framebuffer::error;

         switch (err)
         {
            case error::no_device_handle:
               return "no_device_handle";
            case error::failed_to_create_framebuffer:
               return "failed_to_create_framebuffer";
            default:
               return "UNKNOWN";
         }
      };
   }; // namespace detail

   auto framebuffer::error_category::name() const noexcept -> const char*
   {
      return "vkn_framebuffer";
   }
   auto framebuffer::error_category::message(int err) const -> std::string
   {
      return detail::to_string(static_cast<error>(err));
   }

   framebuffer::framebuffer(create_info&& info) noexcept :
      m_framebuffer{std::move(info.framebuffer)}, m_dimensions{info.dimensions}
   {}

   auto framebuffer::value() const noexcept -> vk::Framebuffer { return m_framebuffer.get(); }
   auto framebuffer::device() const noexcept -> vk::Device { return m_framebuffer.getOwner(); }

   using builder = framebuffer::builder;

   builder::builder(const vkn::device& device, const vkn::render_pass& render_pass,
                    util::logger* plogger) noexcept :
      m_device{device.value()},
      m_plogger{plogger}
   {
      m_info.render_pass = render_pass.value();
   }

   auto builder::build() -> vkn::result<framebuffer>
   {
      if (!m_device)
      {
         return monad::make_left(framebuffer::make_error(framebuffer::error::no_device_handle, {}));
      }

      const auto create_info = vk::FramebufferCreateInfo{}
                                  .setPNext(nullptr)
                                  .setFlags({})
                                  .setRenderPass(m_info.render_pass)
                                  .setAttachmentCount(m_info.attachments.size())
                                  .setPAttachments(m_info.attachments.data())
                                  .setWidth(m_info.width)
                                  .setHeight(m_info.height)
                                  .setLayers(m_info.layer_count);

      return monad::try_wrap<vk::SystemError>([&] {
                return m_device.createFramebufferUnique(create_info);
             })
         .left_map([](vk::SystemError&& err) {
            return framebuffer::make_error(error::failed_to_create_framebuffer, err.code());
         })
         .right_map([&](vk::UniqueFramebuffer&& handle) {
            util::log_info(m_plogger, "[vkn] framebuffer created");

            return framebuffer{
               {.framebuffer = std::move(handle), .dimensions = {m_info.width, m_info.height}}};
         });
   }

   auto builder::add_attachment(vk::ImageView image_view) noexcept -> builder&
   {
      m_info.attachments.emplace_back(image_view);
      return *this;
   }
   auto builder::set_attachments(const util::dynamic_array<vk::ImageView>& attachments) -> builder&
   {
      m_info.attachments = attachments;
      return *this;
   }
   auto builder::set_buffer_width(uint32_t width) noexcept -> builder&
   {
      m_info.width = width;
      return *this;
   }
   auto builder::set_buffer_height(uint32_t height) noexcept -> builder&
   {
      m_info.height = height;
      return *this;
   }
   auto builder::set_layer_count(uint32_t count) noexcept -> builder&
   {
      m_info.layer_count = count;
      return *this;
   }
} // namespace vkn
