// Copyright 2016 The Shaderc Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// The program demonstrates basic shader compilation using the Shaderc C++ API.
// For clarity, each method is deliberately self-contained.
//
// Techniques demonstrated:
//  - Preprocessing GLSL source text
//  - Compiling a shader to SPIR-V assembly text
//  - Compliing a shader to a SPIR-V binary module
//  - Performing optimization with compilation
//  - Setting basic options: setting a preprocessor symbol.
//  - Checking compilation status and extracting an error message.

#include "shaderc_online_compiler.h"

// Returns GLSL shader source text after preprocessing.
std::string preprocess_shader(const std::string& source_name,
	shaderc_shader_kind kind,
	const std::string& source) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// Like -DMY_DEFINE=1
	options.AddMacroDefinition("MY_DEFINE", "1");

	shaderc::PreprocessedSourceCompilationResult result =
		compiler.PreprocessGlsl(source, kind, source_name.c_str(), options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		throw VulkanException(result.GetErrorMessage().c_str());
		return "";
	}

	return { result.cbegin(), result.cend() };
}

// Compiles a shader to SPIR-V assembly. Returns the assembly text
// as a string.
std::string compile_file_to_assembly(const std::string& source_name,
	shaderc_shader_kind kind,
	const std::string& source,
	bool optimize = false) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// Like -DMY_DEFINE=1
	options.AddMacroDefinition("MY_DEFINE", "1");
	if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

	shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(
		source, kind, source_name.c_str(), options);

	if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
		throw VulkanException(result.GetErrorMessage().c_str());
		return "";
	}

	return { result.cbegin(), result.cend() };
}

// Compiles a shader to a SPIR-V binary. Returns the binary as
// a vector of 32-bit words.
std::vector<uint32_t> compile_file(const std::string& source_name,
	shaderc_shader_kind kind,
	const std::string& source,
	bool optimize = false) {
	shaderc::Compiler compiler;
	shaderc::CompileOptions options;

	// Like -DMY_DEFINE=1
	options.AddMacroDefinition("MY_DEFINE", "1");
	if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

	shaderc::SpvCompilationResult module =
		compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

	if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
		throw VulkanException(module.GetErrorMessage().c_str());
		return std::vector<uint32_t>();
	}

	return { module.cbegin(), module.cend() };
}

std::vector<uint32_t> compileGLSLShader(const char kShaderSource[], shaderc_shader_kind shaderType) {
	std::vector<uint32_t> spirv;

	std::string shaderTypeName = "";

	switch (shaderType) {
	case shaderc_glsl_default_vertex_shader:
		shaderTypeName = "Vertex";
		break;
	case shaderc_glsl_default_fragment_shader:
		shaderTypeName = "Fragment";
		break;
	case shaderc_glsl_default_compute_shader:
		shaderTypeName = "Compute";
		break;
	case shaderc_glsl_default_geometry_shader:
		shaderTypeName = "Geometry";
		break;
	case shaderc_glsl_default_tess_control_shader:
		shaderTypeName = "Tesselation Control";
		break;
	case shaderc_glsl_default_tess_evaluation_shader:
		shaderTypeName = "Tesselation Evaluation";
		break;
	}

	{  // Preprocessing
		auto preprocessed = preprocess_shader(
			"shader_src", shaderType, kShaderSource);
		std::cout << "Preprocessed a " << shaderTypeName << " Shader successfully." << std::endl;
	}

	//    {  // Compiling
	//        auto assembly = compile_file_to_assembly(
	//                "shader_src", shaderType, kShaderSource);
	//        std::cout << "SPIR-V assembly:" << std::endl << assembly << std::endl;
	//
	//        auto spirv =
	//                compile_file("shader_src", shaderType, kShaderSource);
	//        std::cout << "Compiled to a binary module with " << spirv.size()
	//                  << " words." << std::endl;
	//    }

#ifdef _ENTBUG
	bool optimize = true;
#else
	bool optimize = false;
#endif

	{  // Compiling with optimizing
		auto assembly =
			compile_file_to_assembly("shader_src", shaderType,
				kShaderSource, /* optimize = */ optimize);

		spirv = compile_file("shader_src", shaderType,
			kShaderSource, /* optimize = */ optimize);
		std::cout << "Compiled to " << ((optimize) ? ("an optimized") : ("a")) << " binary module with " << spirv.size()
			<< " words." << std::endl;
	}

	//    {  // Error case
	//        const char kBadShaderSource[] =
	//                "#version 310 es\nint main() { int main_should_be_void; }\n";
	//
	//        std::cout << std::endl << "Compiling a bad shader:" << std::endl;
	//        compile_file("bad_src", shaderc_glsl_vertex_shader, kBadShaderSource);
	//    }

	//    {  // Compile using the C API.
	//        std::cout << "\n\nCompiling with the C API" << std::endl;
	//
	//        // The first example has a compilation problem.  The second does not.
	//        const char source[2][80] = {"void main() {}", "#version 450\nvoid main() {}"};
	//
	//        shaderc_compiler_t compiler = shaderc_compiler_initialize();
	//        for (int i = 0; i < 2; ++i) {
	//            std::cout << "  Source is:\n---\n" << source[i] << "\n---\n";
	//            shaderc_compilation_result_t result = shaderc_compile_into_spv(
	//                    compiler, source[i], std::strlen(source[i]), shaderc_glsl_vertex_shader,
	//                    "main.vert", "main", nullptr);
	//            auto status = shaderc_result_get_compilation_status(result);
	//            std::cout << "  Result code " << int(status) << std::endl;
	//            if (status != shaderc_compilation_status_success) {
	//                std::cout << "error: " << shaderc_result_get_error_message(result)
	//                          << std::endl;
	//            }
	//            shaderc_result_release(result);
	//        }
	//        shaderc_compiler_release(compiler);
	//    }

	return spirv;
}
