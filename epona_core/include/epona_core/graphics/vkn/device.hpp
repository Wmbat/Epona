/**
 * @file device.hpp
 * @author wmbat wmbat@protonmail.com
 * @date Saturday, 23rd of June, 2020
 * @copyright MIT License
 */

#pragma once

#include <epona_core/containers/dynamic_array.hpp>
#include <epona_core/graphics/vkn/core.hpp>
#include <epona_core/graphics/vkn/physical_device.hpp>

namespace core::gfx::vkn
{
   namespace queue
   {
      enum class type
      {
         present,
         graphics,
         compute,
         transfer
      };

      enum class error
      {
         present_unavailable,
         graphics_unavailable,
         compute_unavailable,
         transfer_unavailable,
         queue_index_out_of_range,
         invalid_queue_family_index
      };

      struct description
      {
         uint32_t index = 0;
         uint32_t count = 0;
         dynamic_array<float> priorities;
      };
   } // namespace queue

   class device
   {
   public:
      enum class error
      {
         device_extension_not_supported,
         failed_to_create_device
      };

      device() = default;
      device(
         physical_device physical_device, vk::Device device, dynamic_array<const char*> extensions);
      device(const device&) = delete;
      device(device&&) noexcept;
      ~device();

      auto operator=(const device&) -> device& = delete;
      auto operator=(device&&) noexcept -> device&;

      [[nodiscard]] auto get_queue_index(queue::type type) const -> vkn::result<uint32_t>;
      [[nodiscard]] auto get_dedicated_queue_index(queue::type type) const -> vkn::result<uint32_t>;

      [[nodiscard]] auto get_queue(queue::type) const -> vkn::result<vk::Queue>;
      [[nodiscard]] [[nodiscard]] auto get_dedicated_queue(queue::type type) const
         -> vkn::result<vk::Queue>;

      [[nodiscard]] auto value() const -> const vk::Device&;
      [[nodiscard]] auto physical() const -> const physical_device&;

   private:
      vkn::physical_device m_physical_device;

      vk::Device m_device;

      dynamic_array<const char*> m_extensions;

   public:
      class builder
      {
      public:
         builder(const loader& vk_loader, physical_device&& phys_device,
            logger* const plogger = nullptr);

         [[nodiscard]] auto build() -> vkn::result<device>;

         auto set_queue_setup(const dynamic_array<queue::description>& descriptions) -> builder&;
         auto add_desired_extension(const std::string& extension_name) -> builder&;

      private:
         const loader& m_loader;

         logger* const m_plogger;

         struct info
         {
            physical_device phys_device;

            dynamic_array<queue::description> queue_descriptions;
            dynamic_array<const char*> desired_extensions;
         } m_info;
      };
   };
} // namespace core::gfx::vkn
