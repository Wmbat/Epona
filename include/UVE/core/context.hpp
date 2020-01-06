#pragma once

#include <UVE/vk/core.hpp>

class logger;

namespace UVE
{
   class context
   {
   public:
      context( );
      context( logger* p_logger );
      context( context const& other ) = delete;
      context( context&& other );
      ~context( );

      context& operator=( context const& rhs ) = delete;
      context& operator=( context&& rhs );

   private:
      void init_volk( );

   private:
      inline static bool IS_VOLK_INITIALIZED = false;

      VkInstance instance;
      VkDebugUtilsMessengerEXT debug_messenger;
      VkPhysicalDevice physical_device;
      VkDevice logical_device;

      logger* p_logger;
   }; // class context
} // namespace UVE
