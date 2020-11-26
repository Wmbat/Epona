#pragma once

#include <vkn/core.hpp>
#include <vkn/device.hpp>
#include <vkn/swapchain.hpp>

namespace vkn
{
   enum struct render_pass_error
   {
      no_device_provided,
      failed_to_create_render_pass
   };

   class render_pass final : public owning_handle<vk::RenderPass>
   {
   public:
      [[nodiscard]] auto device() const noexcept -> vk::Device;

   private:
      vk::Format m_swapchain_format{};

   public:
      class builder
      {
      public:
         builder(const vkn::device& device, const vkn::swapchain& swapchain,
                 util::logger_wrapper logger) noexcept;

         /**
          * Construct a render_pass object. If construction fails, an error will be
          * returned instead
          */
         auto build() -> util::result<render_pass>;

      private:
         vk::Device m_device;
         vk::Format m_swapchain_format;
         vk::Extent2D m_swapchain_extent;

         util::logger_wrapper m_logger;
      };
   };

   auto to_string(render_pass_error err) -> std::string;
   auto to_err_code(render_pass_error err) -> util::error_t;
} // namespace vkn

namespace std
{
   template <>
   struct is_error_code_enum<vkn::render_pass_error> : true_type
   {
   };
} // namespace std
