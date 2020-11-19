#include <water_simulation/camera.hpp>
#include <water_simulation/particle.hpp>
#include <water_simulation/pipeline_codex.hpp>
#include <water_simulation/render_pass.hpp>
#include <water_simulation/render_system.hpp>
#include <water_simulation/renderable.hpp>
#include <water_simulation/shader_codex.hpp>

#include <gfx/data_types.hpp>
#include <gfx/render_manager.hpp>
#include <gfx/window.hpp>

#include <ui/window.hpp>

#include <glm/ext/matrix_transform.hpp>

#include <range/v3/range/conversion.hpp>
#include <range/v3/view/iota.hpp>

#include <cmath>
#include <execution>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

namespace chrono = std::chrono;

auto compute_matrices(const render_system& system) -> camera::matrices
{
   auto dimensions = system.scissor().extent;

   camera::matrices matrices{};
   matrices.projection = glm::perspective(
      glm::radians(90.0F), dimensions.width / (float)dimensions.height, 0.1F, 1000.0F); // NOLINT
   matrices.view =
      glm::lookAt(glm::vec3(20.0f, 20.0f, 30.0f), glm::vec3(-5.0f, 10.0f, -10.0f), // NOLINT
                  glm::vec3(0.0F, 1.0F, 0.0F));
   matrices.projection[1][1] *= -1;

   return matrices;
}

auto get_main_framebuffers(const render_system& system, const std::shared_ptr<util::logger>& logger)
   -> util::dynamic_array<framebuffer::create_info>
{
   util::dynamic_array<framebuffer::create_info> infos;

   const auto swap_extent = system.swapchain().extent();
   for (auto& image_view : system.swapchain().image_views())
   {
      infos.emplace_back(
         framebuffer::create_info{.device = system.device().logical(),
                                  .attachments = {image_view.get(), system.get_depth_attachment()},
                                  .width = swap_extent.width,
                                  .height = swap_extent.height,
                                  .layers = 1,
                                  .logger = logger});
   }

   return infos;
}

auto main_colour_attachment(vk::Format format) -> vk::AttachmentDescription
{
   return {.format = format,
           .samples = vk::SampleCountFlagBits::e1,
           .loadOp = vk::AttachmentLoadOp::eClear,
           .storeOp = vk::AttachmentStoreOp::eStore,
           .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
           .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
           .initialLayout = vk::ImageLayout::eUndefined,
           .finalLayout = vk::ImageLayout::ePresentSrcKHR};
}

auto main_depth_attachment(vkn::device& device) -> vk::AttachmentDescription
{
   const auto res = find_depth_format(device);
   if (auto val = res.value())
   {
      return {.format = val.value(),
              .samples = vk::SampleCountFlagBits::e1,
              .loadOp = vk::AttachmentLoadOp::eClear,
              .storeOp = vk::AttachmentStoreOp::eDontCare,
              .stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
              .stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
              .initialLayout = vk::ImageLayout::eUndefined,
              .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal};
   }

   return {};
}

void compute_density(std::span<particle> particle);
void compute_normals(std::span<particle> particles);
void compute_forces(std::span<particle> particles);
void integrate(std::span<particle> particles);

auto main_logger = std::make_shared<util::logger>("water_simulation"); // NOLINT

auto main() -> int
{
   glfwInit();

   ui::window window{"Water Simulation", 1920, 1080}; // NOLINT

   render_system renderer =
      handle_err(render_system::make({.p_logger = main_logger, .p_window = &window}), main_logger);

   shader_codex shader_codex{renderer, main_logger};
   pipeline_codex pipeline_codex{main_logger};

   const auto vert_shader_info =
      handle_err(shader_codex.insert("resources/shaders/test_vert.spv", vkn::shader_type::vertex),
                 main_logger);
   const auto frag_shader_info =
      handle_err(shader_codex.insert("resources/shaders/test_frag.spv", vkn::shader_type::fragment),
                 main_logger);

   util::dynamic_array<render_pass> render_passes;

   auto depth_attachment = main_depth_attachment(renderer.device());

   render_passes.emplace_back(handle_err(
      render_pass::make({.device = renderer.device().logical(),
                         .swapchain = renderer.swapchain().value(),
                         .colour_attachment = main_colour_attachment(renderer.swapchain().format()),
                         .depth_stencil_attachment = depth_attachment,
                         .framebuffer_create_infos = get_main_framebuffers(renderer, main_logger),
                         .logger = main_logger}),
      main_logger));

   const pipeline_shader_data vertex_shader_data{
      .p_shader = &vert_shader_info.value(),
      .set_layouts = {{.name = "camera_layout",
                       .bindings = {{.binding = 0,
                                     .descriptor_type = vk::DescriptorType::eUniformBuffer,
                                     .descriptor_count = 1}}}},
      .push_constants = {{.name = "mesh_data", .size = sizeof(glm::mat4), .offset = 0}}};

   const pipeline_shader_data fragment_shader_data{.p_shader = &frag_shader_info.value()};

   util::dynamic_array<vk::Viewport> pipeline_viewports;
   pipeline_viewports.emplace_back(renderer.viewport());

   util::dynamic_array<vk::Rect2D> pipeline_scissors;
   pipeline_scissors.emplace_back(renderer.scissor());

   util::dynamic_array<pipeline_shader_data> pipeline_shader_data;
   pipeline_shader_data.push_back(vertex_shader_data);
   pipeline_shader_data.push_back(fragment_shader_data);

   auto main_pipeline_info =
      handle_err(pipeline_codex.insert({.device = renderer.device(),
                                        .render_pass = render_passes[0],
                                        .p_logger = main_logger,
                                        .bindings = renderer.vertex_bindings(),
                                        .attributes = renderer.vertex_attributes(),
                                        .viewports = pipeline_viewports,
                                        .scissors = pipeline_scissors,
                                        .shader_infos = pipeline_shader_data}),
                 main_logger);

   auto camera = create_camera(renderer, main_pipeline_info.value(), main_logger);
   auto sphere = create_renderable(renderer, load_obj("resources/meshes/sphere.obj"));

   constexpr std::size_t x_count = 12u;
   constexpr std::size_t y_count = 30u;
   constexpr std::size_t z_count = 12u;

   util::dynamic_array<particle> particles;
   particles.reserve(x_count * y_count * z_count);

   constexpr float distance_x = water_radius;
   constexpr float distance_y = water_radius;
   constexpr float distance_z = water_radius;

   for (auto i : ranges::views::iota(0U, x_count))
   {
      const float x = (-distance_x * x_count / 2.0f) + distance_x * i;

      for (auto j : ranges::views::iota(0U, y_count))
      {
         const float y = 30.0f + distance_y * j;

         for (auto k : ranges::views::iota(0U, z_count))
         {
            const float z = (-distance_z * z_count / 2.0f) + distance_z * k;

            particles.emplace_back(particle{.position = {x, y, z}, .mass = water_mass});
         }
      }
   }

   util::log_info(main_logger, "particle count = {}", x_count * y_count * z_count);

   chrono::duration<float, std::milli> time_spent{};
   auto start_time = std::chrono::steady_clock::now();

   while (window.is_open())
   {
      window.poll_events();

      if (time_spent.count() >= time_step)
      {
         compute_density(particles);
         compute_normals(particles);
         compute_forces(particles);
         integrate(particles);

         time_spent = {};

         const auto image_index = renderer.begin_frame();

         camera.update(image_index.value(), compute_matrices(renderer));

         render_passes[0].record_render_calls([&](vk::CommandBuffer buffer) {
            auto& pipeline = main_pipeline_info.value();

            buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.value());

            buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.layout(), 0,
                                      {camera.lookup_set(image_index.value())}, {});

            buffer.bindVertexBuffers(0, {sphere.m_vertex_buffer->value()}, {vk::DeviceSize{0}});
            buffer.bindIndexBuffer(sphere.m_index_buffer->value(), 0, vk::IndexType::eUint32);

            for (const auto& particle : particles)
            {
               const auto scale_vector = glm::vec3{scale_factor, scale_factor, scale_factor};
               const auto trans = glm::translate(glm::mat4{1}, particle.position);
               const auto model = glm::scale(trans, scale_vector);

               buffer.pushConstants(pipeline.layout(),
                                    pipeline.get_push_constant_ranges("mesh_data").stageFlags, 0,
                                    sizeof(glm::mat4) * 1, &model);

               buffer.drawIndexed(sphere.m_index_buffer.index_count(), 1, 0, 0, 0);
            }
         });

         renderer.render(render_passes);
         renderer.end_frame();
      }

      const auto old = start_time;
      start_time = chrono::steady_clock::now();
      const chrono::duration<float, std::milli> delta_time = start_time - old;
      time_spent += delta_time;

      util::log_info(main_logger, "dt = {}", delta_time.count());
   }

   renderer.wait();

   return 0;
}

auto poly6_kernel(float r) -> float
{
   constexpr float k_squared = my_pow(kernel_radius, 2.0f);

   return cube(k_squared - square(r));
}
auto poly6_grad_kernel(const glm::vec3& vec, float r) -> glm::vec3
{
   constexpr float k_squared = my_pow(kernel_radius, 2.0f);

   return square(k_squared - square(r)) * vec;
}
auto spiky_kernel(float r) -> float
{
   return cube(kernel_radius - r);
}
auto spiky_grad_kernel(const glm::vec3& vec, float r) -> glm::vec3
{
   return vec * (square(kernel_radius - r) * (1.0f / r));
}
auto viscosity_kernel(float r) -> float
{
   if (r <= kernel_radius)
   {
      constexpr float predicate = 45.0F / (pi * my_pow(kernel_radius, 6.0f));

      return predicate * (kernel_radius - r);
   }

   return 0;
}
auto cohesion_kernel(float r) -> float
{
   static constexpr float predicate = 32.0f / (pi * my_pow(kernel_radius, 9));
   static constexpr float offset = my_pow(kernel_radius, 6) / 64;

   if (r <= half_kernel_radius)
   {
      return predicate * (2.0f * cube(kernel_radius - r) * cube(r) - offset);
   }

   return predicate * cube(kernel_radius - r) * cube(r);
}

void compute_density(std::span<particle> particles)
{
   std::transform(std::execution::par, std::begin(particles), std::end(particles),
                  std::begin(particles), [&](const auto& particle_i) {
                     float density = 0.0F;

                     for (auto& particle_j : particles)
                     {
                        const auto r_ij = particle_j.position - particle_i.position;
                        const auto r = glm::length(r_ij);

                        if (r <= kernel_radius)
                        {
                           density += poly6_kernel(r) * water_mass * poly6_constant;
                        }
                     }
                     float density_ratio = particle_i.density / rest_density;

                     particle r{particle_i};
                     r.density = density;
                     r.pressure = density_ratio < 1.0f ? 0 : std::pow(density_ratio, 7.0f) - 1.0f;

                     return r;
                  });
}
void compute_normals(std::span<particle> particles)
{
   std::transform(std::execution::par, std::begin(particles), std::end(particles),
                  std::begin(particles), [&](const auto& particle_i) {
                     glm::vec3 normal{0.0f, 0.0f, 0.0f};

                     for (const auto& particle_j : particles)
                     {
                        const auto r_ij = particle_j.position - particle_i.position;
                        const auto r = glm::length(r_ij);

                        if (r <= kernel_radius)
                        {
                           normal += poly6_grad_kernel(r_ij, r) / particle_j.density;
                        }
                     }

                     particle r = particle_i;
                     r.normal = normal * kernel_radius * water_radius * poly6_grad_constant;

                     return r;
                  });
}
void compute_forces(std::span<particle> particles)
{
   const glm::vec3 gravity_vector{0.0f, gravity * gravity_multiplier, 0.0f};

   std::transform(
      std::execution::par, std::begin(particles), std::end(particles), std::begin(particles),
      [&](const auto& particle_i) {
         glm::vec3 pressure_force{0.0f, 0.0f, 0.0f};
         glm::vec3 viscosity_force{0.0f, 0.0f, 0.0f};
         glm::vec3 cohesion_force{0.0f, 0.0f, 0.0f};
         glm::vec3 curvature_force{0.0f, 0.0f, 0.0f};
         glm::vec3 gravity_force{0.0f, 0.0f, 0.0f};

         for (const auto& particle_j : particles)
         {
            if (&particle_i != &particle_j)
            {
               auto r_ij = particle_i.position - particle_j.position;
               if (r_ij.x == 0.0f && r_ij.y == 0.0f) // NOLINT
               {
                  r_ij.x += 0.0001f; // NOLINT
                  r_ij.y += 0.0001f; // NOLINT
               }

               const auto r = glm::length(r_ij);

               if (r < kernel_radius)
               {
                  pressure_force -= (particle_i.pressure + particle_j.pressure) /
                     (2.0f * particle_j.density) * spiky_grad_kernel(r_ij, r) * spiky_grad_constant;

                  viscosity_force -= (particle_j.velocity - particle_i.velocity) /
                     particle_j.density * viscosity_kernel(r);

                  const float correction_factor =
                     2.0f * rest_density / (particle_i.density + particle_j.density);

                  cohesion_force += correction_factor * (r_ij / r) * cohesion_kernel(r);
                  curvature_force += correction_factor * (particle_i.normal - particle_j.normal);
               }
            }
         }

         gravity_force += gravity_vector * particle_i.density;
         viscosity_force *= viscosity_constant;
         cohesion_force *= -surface_tension_coefficient * water_mass;
         curvature_force *= -surface_tension_coefficient;

         const auto main_forces =
            (viscosity_force + pressure_force + cohesion_force + curvature_force) * water_mass;

         particle r{particle_i};
         r.force = main_forces + gravity_force;

         return r;
      });
}
void integrate(std::span<particle> particles)
{
   std::for_each(std::execution::par_unseq, std::begin(particles), std::end(particles),
                 [&](auto& particle) {
                    particle.velocity += time_step * particle.force / particle.density;
                    particle.position += time_step * particle.velocity;

                    if (particle.position.x - kernel_radius < -edge) // NOLINT
                    {
                       particle.velocity.x *= -bound_damping;      // NOLINT
                       particle.position.x = kernel_radius - edge; // NOLINT
                    }

                    if (particle.position.x + kernel_radius > edge) // NOLINT
                    {
                       particle.velocity.x *= -bound_damping;      // NOLINT
                       particle.position.x = edge - kernel_radius; // NOLINT
                    }

                    if (particle.position.z - kernel_radius < -edge) // NOLINT
                    {
                       particle.velocity.z *= -bound_damping;      // NOLINT
                       particle.position.z = kernel_radius - edge; // NOLINT
                    }

                    if (particle.position.z + kernel_radius > edge) // NOLINT
                    {
                       particle.velocity.z *= -bound_damping;      // NOLINT
                       particle.position.z = edge - kernel_radius; // NOLINT
                    }

                    if (particle.position.y - kernel_radius < 0.0f) // NOLINT
                    {
                       particle.velocity.y *= -bound_damping; // NOLINT
                       particle.position.y = kernel_radius;   // NOLINT
                    }
                 });
};
