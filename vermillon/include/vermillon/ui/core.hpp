#pragma once

#include <vermillon/util/strong_type.hpp>

#include <monads/result.hpp>

#include <system_error>

namespace ui
{
   using error_t = util::strong_type<std::error_code, struct error_tag>;

   template <typename Any>
   using result = monad::result<Any, error_t>;
} // namespace ui
