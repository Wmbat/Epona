/**
 * @file physical_device.hpp
 * @author wmbat wmbat@protonmail.com
 * @date Saturday, 20th of June, 2020
 * @copyright MIT License
 */

#pragma once

#include <vkn/instance.hpp>

#include <monads/maybe.hpp>

namespace vkn
{
   namespace detail
   {
      /**
       * Get the index of a queue that support graphics operation if it exists.
       */
      auto
      get_graphics_queue_index(const util::range_over<vk::QueueFamilyProperties> auto& families)
         -> monad::maybe<uint32_t>
      {
         for (uint32_t i = 0; const auto& fam : families)
         {
            if (fam.queueFlags & vk::QueueFlagBits::eGraphics)
            {
               return i;
            }

            ++i;
         }

         return monad::none;
      }

      /**
       * Get the index of a queue that support present operation if it exists.
       */
      auto get_present_queue_index(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface,
                                   const util::range_over<vk::QueueFamilyProperties> auto& families)
         -> monad::maybe<uint32_t>
      {
         for (uint32_t i = 0; i < families.size(); ++i)
         {
            VkBool32 present_support = VK_FALSE;
            if (surface)
            {
               if (physical_device.getSurfaceSupportKHR(i, surface, &present_support) !=
                   vk::Result::eSuccess)
               {
                  return monad::none;
               }
            }

            if (present_support == VK_TRUE)
            {
               return i;
            }
         }

         return monad::none;
      }

      /**
       * Get the index of a queue that does compute operation only if it exists.
       */
      auto get_dedicated_compute_queue_index(
         const util::range_over<vk::QueueFamilyProperties> auto& families) -> monad::maybe<uint32_t>
      {
         for (uint32_t i = 0; const auto& fam : families)
         {
            if ((fam.queueFlags & vk::QueueFlagBits::eCompute) &&
                (static_cast<uint32_t>(fam.queueFlags & vk::QueueFlagBits::eGraphics) == 0) &&
                (static_cast<uint32_t>(fam.queueFlags & vk::QueueFlagBits::eTransfer) == 0))
            {
               return i;
            }

            ++i;
         }

         return monad::none;
      }

      /**
       * Get the index of a queue that does transfer operation only, otherwise returns nothing.
       */
      auto get_dedicated_transfer_queue_index(
         const util::range_over<vk::QueueFamilyProperties> auto& families) -> monad::maybe<uint32_t>
      {
         for (uint32_t i = 0; const auto& fam : families)
         {
            if ((fam.queueFlags & vk::QueueFlagBits::eTransfer) &&
                (static_cast<uint32_t>(fam.queueFlags & vk::QueueFlagBits::eGraphics) == 0) &&
                (static_cast<uint32_t>(fam.queueFlags & vk::QueueFlagBits::eCompute) == 0))
            {
               return i;
            }

            ++i;
         }

         return monad::none;
      }

      /**
       * Get a queue that support compute operation but not graphics operation, otherwise returns
       * nothing
       */
      auto get_separated_compute_queue_index(
         const util::range_over<vk::QueueFamilyProperties> auto& families) -> monad::maybe<uint32_t>
      {
         monad::maybe<uint32_t> compute{};
         for (uint32_t i = 0; const auto& fam : families)
         {
            if ((fam.queueFlags & vk::QueueFlagBits::eCompute) &&
                (static_cast<uint32_t>(fam.queueFlags & vk::QueueFlagBits::eGraphics) == 0))
            {
               if (static_cast<uint32_t>(families[i].queueFlags & vk::QueueFlagBits::eTransfer) ==
                   0)
               {
                  return i;
               }

               compute = i;
            }

            ++i;
         }

         return compute;
      }

      /**
       * Get a queue that support transfer operation but not graphics operation, otherwise returns
       * nothing
       */
      auto get_separated_transfer_queue_index(
         const util::range_over<vk::QueueFamilyProperties> auto& families) -> monad::maybe<uint32_t>
      {
         monad::maybe<uint32_t> transfer{};
         for (uint32_t i = 0; const auto& fam : families)
         {
            if ((fam.queueFlags & vk::QueueFlagBits::eTransfer) &&
                (static_cast<uint32_t>(fam.queueFlags & vk::QueueFlagBits::eGraphics) == 0))
            {
               if (static_cast<uint32_t>(fam.queueFlags & vk::QueueFlagBits::eCompute) == 0)
               {
                  return i;
               }

               transfer = i;
            }

            ++i;
         }

         return transfer;
      }
   } // namespace detail

   /**
    * The physical representation of a graphics card
    */
   class physical_device
   {
   public:
      /**
       * The type of graphics card supported
       */
      enum class type
      {
         other = 0,
         integrated = 1,
         discrete = 2,
         virtual_gpu = 3,
         cpu = 4,
      };

      /**
       * The errors that can come from the physical_device class
       */
      enum class error
      {
         failed_to_retrieve_physical_device_count,
         failed_to_enumerate_physical_devices,
         no_physical_device_found,
         no_suitable_device
      };

      /**
       * The information required to construct a physical_device class
       */
      struct create_info
      {
         std::string_view name{};
         vk::PhysicalDeviceFeatures features{};
         vk::PhysicalDeviceProperties properties{};
         vk::PhysicalDeviceMemoryProperties mem_properties{};
         vk::Instance instance{nullptr};
         vk::PhysicalDevice device{nullptr};
         vk::SurfaceKHR surface{nullptr};
         util::dynamic_array<vk::QueueFamilyProperties> queue_families{};
      };

      physical_device() = default;
      physical_device(const create_info& info);
      physical_device(const physical_device&) = delete;
      physical_device(physical_device&&) noexcept;
      ~physical_device();

      auto operator=(const physical_device&) -> physical_device& = delete;
      auto operator=(physical_device&&) noexcept -> physical_device&;

      /**
       * Check if the physical_device instance has a dedicated queue for compute
       * operations
       */
      [[nodiscard]] auto has_dedicated_compute_queue() const -> bool;
      /**
       * Check if the physical_device instance has a dedicated queue for transfer
       * operations
       */
      [[nodiscard]] auto has_dedicated_transfer_queue() const -> bool;

      /**
       * Check if the physical_device instance has a queue for compute
       * operations but that is separated from the graphics queue
       */
      [[nodiscard]] auto has_separated_compute_queue() const -> bool;
      /**
       * Check if the physical_device instance has a queue for transfer
       * operations but that is separated from the graphics queue
       */
      [[nodiscard]] auto has_separated_transfer_queue() const -> bool;

      /**
       * Get a const reference to the underlying vulkan physical device handle
       */
      [[nodiscard]] auto value() const noexcept -> const vk::PhysicalDevice&;
      /**
       * Get a const reference to features of the graphics card
       */
      [[nodiscard]] auto features() const noexcept -> const vk::PhysicalDeviceFeatures&;
      /**
       * Get a const reference to the surface used by the graphics card
       */
      [[nodiscard]] auto surface() const noexcept -> const vk::SurfaceKHR&;
      /**
       * Get the full list of queue family properties of the graphics card
       */
      [[nodiscard]] auto queue_families() const
         -> const util::dynamic_array<vk::QueueFamilyProperties>;

   private:
      std::string m_name{};

      vk::PhysicalDeviceFeatures m_features{};
      vk::PhysicalDeviceProperties m_properties{};
      vk::PhysicalDeviceMemoryProperties m_mem_properties{};

      vk::Instance m_instance{nullptr};
      vk::PhysicalDevice m_device{nullptr};
      vk::SurfaceKHR m_surface{nullptr};

      util::dynamic_array<vk::QueueFamilyProperties> m_queue_families{};

   public:
      /**
       * A helper class used to simplify the graphics card selection
       */
      class selector
      {
      public:
         selector(const instance& instance, util::logger* plogger = nullptr);

         /**
          * Attempt to find a suitable physical device, if no physical devices are found,
          * an error is returned
          */
         [[nodiscard]] auto select() -> vkn::result<physical_device>;

         /**
          * Set the preferred physical device type for selection
          */
         auto set_preferred_gpu_type(physical_device::type type) noexcept -> selector&;
         /**
          * Set the surface the physical device will used for rendering
          */
         auto set_surface(vk::SurfaceKHR&& surface) noexcept -> selector&;
         /**
          * Dictate that all physical device types may be selected
          */
         auto allow_any_gpu_type(bool allow = true) noexcept -> selector&;
         /**
          * Require a queue dedicated to present operations
          */
         auto require_present(bool require = true) noexcept -> selector&;
         /**
          * Require a queue dedicated to compute operations
          */
         auto require_dedicated_compute() noexcept -> selector&;
         /**
          * Require a queue dedicated to transfer operations
          */
         auto require_dedicated_transfer() noexcept -> selector&;
         /**
          * Required a queue that support compute operations, but not graphics
          * operations
          */
         auto require_separated_compute() noexcept -> selector&;
         /**
          * Required a queue that support transfer operations, but not graphics
          * operations
          */
         auto require_separated_transfer() noexcept -> selector&;
         /**
          * Simply select the first physical device found
          */
         auto select_first_gpu() noexcept -> selector&;

      private:
         util::logger* m_plogger;

         struct system_info
         {
            vk::Instance instance{};
            vk::SurfaceKHR surface{};

            util::dynamic_array<const char*> instance_extensions;
         } m_system_info;

         struct selection_info
         {
            physical_device::type prefered_type = physical_device::type::discrete;
            bool allow_any_gpu_type = true;
            bool require_present = true;
            bool require_dedicated_compute = false;
            bool require_dedicated_transfer = false;
            bool require_separated_compute = false;
            bool require_separated_transfer = false;
            bool select_first_gpu = false;
         } m_selection_info;

         struct physical_device_description
         {
            vk::PhysicalDevice phys_device;

            util::small_dynamic_array<vk::QueueFamilyProperties, 16> queue_families{};

            vk::PhysicalDeviceFeatures features{};
            vk::PhysicalDeviceProperties properties{};
            vk::PhysicalDeviceMemoryProperties mem_properties{};
         };

         enum class suitable
         {
            yes,
            partial,
            no
         };

         [[nodiscard]] auto populate_device_details(vk::PhysicalDevice) const
            -> physical_device_description;

         [[nodiscard]] auto is_device_suitable(const physical_device_description& desc) const
            -> suitable;

         auto go_through_available_gpus(
            const util::range_over<physical_device_description> auto& range) const
            -> physical_device_description
         {
            physical_device_description selected;
            for (const auto& desc : range)
            {
               const auto suitable = is_device_suitable(desc);
               if (suitable == suitable::yes)
               {
                  selected = desc;
                  break;
               }
               else if (suitable == suitable::partial)
               {
                  selected = desc;
               }
            }
            return selected;
         }
      };
   };
} // namespace vkn

namespace std
{
   template <>
   struct is_error_code_enum<vkn::physical_device::error> : true_type
   {
   };
} // namespace std
