#pragma once

#include <vkn/core.hpp>
#include <vkn/descriptor_set_layout.hpp>
#include <vkn/device.hpp>
#include <vkn/render_pass.hpp>
#include <vkn/shader.hpp>

#include <util/error.hpp>
#include <util/strong_type.hpp>

namespace vkn
{
   enum struct pipeline_type
   {
      graphics,
      compute
   };

   enum struct graphics_pipeline_error
   {
      failed_to_create_descriptor_set_layout,
      failed_to_create_pipeline_layout,
      invalid_vertex_shader_bindings,
      failed_to_create_pipeline
   };

   auto to_string(graphics_pipeline_error err) -> std::string;

   class graphics_pipeline final : public owning_handle<vk::Pipeline>
   {
      static constexpr std::size_t expected_shader_count{2U};

      struct create_info
      {
      };

   public:
      [[nodiscard]] auto layout() const noexcept -> vk::PipelineLayout;
      [[nodiscard]] auto device() const noexcept -> vk::Device;

      auto get_descriptor_set_layout(const std::string& name) const
         -> const vkn::descriptor_set_layout&;
      auto get_push_constant_ranges(const std::string& name) const -> const vk::PushConstantRange&;

   private:
      vk::UniquePipelineLayout m_pipeline_layout{nullptr};

      std::unordered_map<std::string, vkn::descriptor_set_layout> m_set_layouts{};
      std::unordered_map<std::string, vk::PushConstantRange> m_push_constants{};

   public:
      class builder final
      {
         template <typename any_>
         using shader_dynamic_array = util::small_dynamic_array<any_, expected_shader_count>;

         struct layout_info;

      public:
         builder(const vkn::device& device, const vkn::render_pass& render_pass,
                 std::shared_ptr<util::logger> p_logger);

         [[nodiscard]] auto build() const -> util::result<graphics_pipeline>;

         auto add_shader(const vkn::shader& shader) noexcept -> builder&;
         auto add_viewport(const vk::Viewport& viewport, const vk::Rect2D& scissor) noexcept
            -> builder&;
         auto add_vertex_binding(vk::VertexInputBindingDescription&& binding) noexcept -> builder&;
         auto add_vertex_attribute(vk::VertexInputAttributeDescription&& attribute) noexcept
            -> builder&;
         auto add_set_layout(const std::string& name,
                             const util::dynamic_array<vk::DescriptorSetLayoutBinding>& binding)
            -> builder&;
         auto add_push_constant(const std::string& name, vkn::shader_type shader_type,
                                util::size_t offset, util::size_t size) -> builder&;

      private:
         [[nodiscard]] auto create_descriptor_set_layouts() const
            -> util::result<graphics_pipeline>;
         [[nodiscard]] auto create_push_constant_ranges(graphics_pipeline&& pipeline) const
            -> util::result<graphics_pipeline>;
         [[nodiscard]] auto create_pipeline_layout(graphics_pipeline&& pipeline) const
            -> util::result<graphics_pipeline>;
         [[nodiscard]] auto create_pipeline(graphics_pipeline&& pipeline) const
            -> util::result<graphics_pipeline>;

         [[nodiscard]] auto
         check_vertex_attribute_support(const vkn::shader* p_shader) const noexcept -> bool;

      private:
         std::shared_ptr<util::logger> mp_logger;

         struct descriptor_set_layout_info
         {
            std::string name;
            util::dynamic_array<vk::DescriptorSetLayoutBinding> binding;
         };

         struct push_constant_info
         {
            std::string name;
            vkn::shader_type type;
            util::size_t offset;
            util::size_t size;
         };

         struct layout_info
         {
            vk::UniqueDescriptorSetLayout camera_layout;
            util::dynamic_array<vk::UniqueDescriptorSetLayout> set_layouts;

            vk::UniquePipelineLayout pipeline_layout;
         };

         struct info
         {
            vk::Device device;
            vk::RenderPass render_pass;

            shader_dynamic_array<const vkn::shader*> shaders;

            util::small_dynamic_array<vk::Viewport, 1> viewports;
            util::small_dynamic_array<vk::Rect2D, 1> scissors;

            util::dynamic_array<vk::VertexInputBindingDescription> binding_descriptions;
            util::dynamic_array<vk::VertexInputAttributeDescription> attribute_descriptions;

            util::dynamic_array<descriptor_set_layout_info> set_layouts;
            util::dynamic_array<push_constant_info> push_constants;
         } m_info;
      };
   };
} // namespace vkn

namespace std
{
   template <>
   struct is_error_code_enum<vkn::graphics_pipeline_error> : true_type
   {
   };
} // namespace std
