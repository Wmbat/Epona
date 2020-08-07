/**
 * @file shader.hpp
 * @author wmbat wmbat@protonmail.com
 * @date Saturday, 18th of July, 2020
 * @copyright MIT License
 */

#pragma once

#include <vkn/device.hpp>

#include <util/logger.hpp>

#include <filesystem>
#include <string_view>

namespace vkn
{
   class shader
   {
      struct error_category : std::error_category
      {
         [[nodiscard]] auto name() const noexcept -> const char* override;
         [[nodiscard]] auto message(int err) const -> std::string override;
      };

   public:
      enum class error
      {
         no_filepath,
         invalid_filepath,
         filepath_not_a_file,
         failed_to_open_file,
         failed_to_preprocess_shader,
         failed_to_parse_shader,
         failed_to_link_shader,
         failed_to_create_shader_module
      };

      enum class type
      {
         vertex,
         fragment,
         compute,
         geometry,
         tess_eval,
         tess_control,
         count
      };

      struct create_info
      {
         vk::Device device{};
         vk::ShaderModule shader_module{};
         shader::type type{};
      };

   public:
      shader() = default;
      shader(const create_info& info);
      shader(create_info&& info);
      shader(const shader&) = delete;
      shader(shader&& other) noexcept;
      ~shader();

      auto operator=(const shader&) -> shader& = delete;
      auto operator=(shader&& rhs) noexcept -> shader&;

      auto value() noexcept -> vk::ShaderModule&;
      [[nodiscard]] auto value() const noexcept -> const vk::ShaderModule&;
      [[nodiscard]] auto name() const noexcept -> std::string_view;
      [[nodiscard]] auto stage() const noexcept -> type;

      inline static auto make_error_code(error err) -> std::error_code
      {
         return {static_cast<int>(err), m_category};
      }

   private:
      vk::Device m_device;
      vk::ShaderModule m_shader_module;

      type m_type{type::count};
      std::string m_name{};

      inline static const error_category m_category{};

   public:
      class builder
      {
      public:
         builder(const device& device, util::logger* const plogger);

         auto build() -> result<shader>;

         auto set_spirv_binary(const util::dynamic_array<std::uint32_t>& spirv_binary) -> builder&;
         auto set_name(const std::string& name) -> builder&;
         auto set_type(type shader_type) -> builder&;

      private:
         util::logger* const m_plogger{nullptr};

         struct info
         {
            vk::Device device{};
            uint32_t version{0u};

            util::dynamic_array<std::uint32_t> spirv_binary{};

            type m_type{type::count};
            std::string name{};
         } m_info;
      };
   };
} // namespace vkn
