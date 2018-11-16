//
// Created by chakmeshma on 25.11.2017.
//

#ifndef VULKAN_TEST_SHADERC_ONLINE_COMPILER_H
#define VULKAN_TEST_SHADERC_ONLINE_COMPILER_H

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <shaderc/shaderc.hpp>
#include "Vulkan Engine Exception.h"

std::vector<uint32_t> compileGLSLShader(const char kShaderSource[], shaderc_shader_kind shaderType);

#endif //VULKAN_TEST_SHADERC_ONLINE_COMPILER_H
