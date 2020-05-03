/**
 * @file error_handling.hpp.
 * @author wmbat wmbat@protonmail.com.
 * @date Tuesday, May 3rd, 2020.
 * @copyright MIT License.
 */

#pragma once

#include <ESL/utils/logger.hpp>

#include <string_view>

namespace ESL
{
#ifndef ESL_NO_LOGGING

   inline static logger ESL_error_logger{ "ESL error logger" };
#endif

   static void handle_bad_alloc_error( [[maybe_unused]] std::string_view error_msg )
   {
#ifndef ESL_NO_EXCEPTIONS
      throw std::bad_alloc{ };
#else
#   ifndef ESL_NO_LOGGING
      ESL_error_logger.error( error_msg );
#   endif

      abort( );
#endif
   }
} // namespace ESL
