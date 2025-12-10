# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/davidpc/esp/v5.5.1/esp-idf/components/bootloader/subproject")
  file(MAKE_DIRECTORY "/home/davidpc/esp/v5.5.1/esp-idf/components/bootloader/subproject")
endif()
file(MAKE_DIRECTORY
  "/home/davidpc/Sistemas_en_tiempo_Real-2025/http_sever_and_flash_program/build/bootloader"
  "/home/davidpc/Sistemas_en_tiempo_Real-2025/http_sever_and_flash_program/build/bootloader-prefix"
  "/home/davidpc/Sistemas_en_tiempo_Real-2025/http_sever_and_flash_program/build/bootloader-prefix/tmp"
  "/home/davidpc/Sistemas_en_tiempo_Real-2025/http_sever_and_flash_program/build/bootloader-prefix/src/bootloader-stamp"
  "/home/davidpc/Sistemas_en_tiempo_Real-2025/http_sever_and_flash_program/build/bootloader-prefix/src"
  "/home/davidpc/Sistemas_en_tiempo_Real-2025/http_sever_and_flash_program/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/davidpc/Sistemas_en_tiempo_Real-2025/http_sever_and_flash_program/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/davidpc/Sistemas_en_tiempo_Real-2025/http_sever_and_flash_program/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
