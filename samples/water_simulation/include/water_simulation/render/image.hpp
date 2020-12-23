#pragma once

#include <water_simulation/core.hpp>

#include <vermillon/util/logger.hpp>
#include <vermillon/vulkan/device.hpp>

#include <type_traits>
enum struct image_error
{
   failed_to_find_supported_format,
   failed_to_create_image,
   failed_to_find_memory_type,
   failed_to_allocate_device_memory,
   failed_to_create_image_view
};

/**
 * @brief Convert an image_error value to string
 *
 * @param err The error value to convert.
 *
 * @return The string representation of the error value.
 */
auto to_string(image_error err) -> std::string;
/**
 * @brief Convert an image_error value to a vml::error_t
 */
auto to_err_cond(image_error err) -> vml::error_t;

auto find_memory_type(uint32_t type_filter, const vk::MemoryPropertyFlags& properties,
                      vk::PhysicalDevice device) -> monad::maybe<std::uint32_t>;
/**
 * @brief
 *
 * @param candidates The desired format candidates
 * @param tiling The tiling the format should support
 * @param features The features the format should support
 * @param device
 *
 * @return
 */
auto find_supported_formats(std::span<const vk::Format> candidates, vk::ImageTiling tiling,
                            const vk::FormatFeatureFlags& features, const vkn::device& device)
   -> monad::maybe<vk::Format>;
auto find_depth_format(const vkn::device& device) -> monad::maybe<vk::Format>;
auto find_colour_format(const vkn::device& device) -> monad::maybe<vk::Format>;

enum struct image_flags
{
   colour = 0x01,
   depth_stencil = 0x02,
   transfer_src = 0x04
};

constexpr auto operator&(image_flags bit0, image_flags bit1) noexcept -> image_flags
{
   return static_cast<image_flags>(static_cast<std::underlying_type_t<image_flags>>(bit0) &
                                   static_cast<std::underlying_type_t<image_flags>>(bit1));
}

constexpr auto operator|(image_flags bit0, image_flags bit1) noexcept -> image_flags
{
   return static_cast<image_flags>(static_cast<std::underlying_type_t<image_flags>>(bit0) |
                                   static_cast<std::underlying_type_t<image_flags>>(bit1));
}

constexpr auto operator^(image_flags bit0, image_flags bit1) noexcept -> image_flags
{
   return static_cast<image_flags>(static_cast<std::underlying_type_t<image_flags>>(bit0) ^
                                   static_cast<std::underlying_type_t<image_flags>>(bit1));
}

constexpr auto operator~(image_flags bits) noexcept -> image_flags
{
   return static_cast<image_flags>(~(static_cast<std::underlying_type_t<image_flags>>(bits)));
}

/**
 * @brief Convert a image_flags value to a string.
 *
 * @param flags The bit flags to convert.
 *
 * @return The string representation of the flag value.
 */
auto to_string(image_flags flags) -> std::string;

struct image_create_info
{
   util::logger_wrapper logger;

   vkn::device& device;

   crl::small_dynamic_array<vk::Format, 5> formats;

   vk::ImageTiling tiling;
   vk::MemoryPropertyFlags memory_properties;

   std::uint32_t width{0};
   std::uint32_t height{0};
};

/**
 * @brief
 *
 * @tparam Flags
 */
template <image_flags Flags>
class image
{
public:
   image() = default;
   image(image_create_info&& info) :
      m_tiling{info.tiling}, m_subresource_range{.aspectMask = to_image_aspect_flag(Flags),
                                                 .baseMipLevel = 0,
                                                 .levelCount = 1,
                                                 .baseArrayLayer = 0,
                                                 .layerCount = 1},
      m_memory_properties{info.memory_properties}, m_width{info.width}, m_height{info.height}
   {
      auto fmt_feature_flag = to_format_feature_flags(Flags);

      const auto fmt =
         find_supported_formats(info.formats, m_tiling, fmt_feature_flag, info.device);

      if (!fmt)
      {
         throw vml::runtime_error{to_err_cond(image_error::failed_to_find_supported_format)};
      }

      auto logical = info.device.logical();
      auto physical = info.device.physical();

      m_fmt = fmt.value();
      m_image = create_image(logical);
      m_memory = allocate_memory(logical, physical);

      logical.bindImageMemory(m_image.get(), m_memory.get(), 0);

      m_view = create_image_view(logical);

      info.logger.info("{} image of dimensions ({}, {}) using {} memory with format {} created",
                       to_string(Flags), m_width, m_height, vk::to_string(info.memory_properties),
                       vk::to_string(m_fmt));
   }

   /**
    * @brief Access the underlying vulkan image handle
    *
    * @return The handle to the vulkan image.
    */
   auto value() const -> vk::Image { return m_image.get(); } // NOLINT
   /**
    * @brief Access the underlying vulkan image view  handle
    *
    * @return The handle to the vulkan image view.
    */
   auto view() const -> vk::ImageView { return m_view.get(); }   // NOLINT
   auto subresource_layers() const -> vk::ImageSubresourceLayers // NOLINT
   {
      return {.aspectMask = m_subresource_range.aspectMask,
              .mipLevel = m_subresource_range.baseMipLevel,
              .baseArrayLayer = m_subresource_range.baseArrayLayer,
              .layerCount = m_subresource_range.layerCount};
   };
   auto width() const -> std::uint32_t { return m_width; }   // NOLINT
   auto height() const -> std::uint32_t { return m_height; } // NOLINT

private:
   [[nodiscard]] auto create_image(vk::Device device) const -> vk::UniqueImage
   {
      return device.createImageUnique({.imageType = vk::ImageType::e2D,
                                       .format = m_fmt,
                                       .extent = {.width = m_width, .height = m_height, .depth = 1},
                                       .mipLevels = 1,
                                       .arrayLayers = 1,
                                       .samples = vk::SampleCountFlagBits::e1,
                                       .tiling = m_tiling,
                                       .usage = to_usage_flags(Flags),
                                       .sharingMode = vk::SharingMode::eExclusive,
                                       .initialLayout = vk::ImageLayout::eUndefined});
   }
   [[nodiscard]] auto allocate_memory(vk::Device device, vk::PhysicalDevice physical) const
      -> vk::UniqueDeviceMemory
   {
      const auto memory_requirements = device.getImageMemoryRequirements(m_image.get());
      const auto memory_type_index =
         find_memory_type(memory_requirements.memoryTypeBits, m_memory_properties, physical);

      if (memory_type_index)
      {
         return device.allocateMemoryUnique({.allocationSize = memory_requirements.size,
                                             .memoryTypeIndex = memory_type_index.value()});
      }

      throw vml::runtime_error{to_err_cond(image_error::failed_to_find_memory_type)};
   }
   [[nodiscard]] auto create_image_view(vk::Device device) const -> vk::UniqueImageView
   {
      auto t = vk::ImageViewCreateInfo{.image = m_image.get(),
                                       .viewType = vk::ImageViewType::e2D,
                                       .format = m_fmt,
                                       .subresourceRange = m_subresource_range};

      return device.createImageViewUnique(t);
   }

   [[nodiscard]] constexpr auto to_usage_flags(image_flags flags) const noexcept
      -> vk::ImageUsageFlags
   {
      vk::ImageUsageFlags usage;

      if ((flags & image_flags::colour) == image_flags::colour)
      {
         usage |= vk::ImageUsageFlagBits::eColorAttachment;
      }

      if ((flags & image_flags::depth_stencil) == image_flags::depth_stencil)
      {
         usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
      }

      return usage;
   }

   [[nodiscard]] constexpr auto to_format_feature_flags(image_flags flags) const noexcept
      -> vk::FormatFeatureFlagBits
   {
      if ((flags & image_flags::colour) == image_flags::colour)
      {
         return vk::FormatFeatureFlagBits::eColorAttachment;
      }

      if ((flags & image_flags::depth_stencil) == image_flags::depth_stencil)
      {
         return vk::FormatFeatureFlagBits::eDepthStencilAttachment;
      }
   }

   [[nodiscard]] constexpr auto to_image_aspect_flag(image_flags flags) const noexcept
      -> vk::ImageAspectFlagBits
   {
      if ((flags & image_flags::colour) == image_flags::colour)
      {
         return vk::ImageAspectFlagBits::eColor;
      }

      if ((flags & image_flags::depth_stencil) == image_flags::depth_stencil)
      {
         return vk::ImageAspectFlagBits::eDepth;
      }

      return {};
   }

private:
   vk::UniqueImage m_image;
   vk::UniqueDeviceMemory m_memory;
   vk::UniqueImageView m_view;

   vk::Format m_fmt{};
   vk::ImageTiling m_tiling{};
   vk::ImageSubresourceRange m_subresource_range;
   vk::MemoryPropertyFlags m_memory_properties;

   std::uint32_t m_width{0};
   std::uint32_t m_height{0};
};

static constexpr std::array depth_formats = {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
                                             vk::Format::eD24UnormS8Uint};
static constexpr std::array colour_formats = {vk::Format::eR8G8B8A8Srgb};
