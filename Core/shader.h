#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include <shaderc/shaderc.hpp>

class Shader
{
  public:
    Shader(const char *filename)
    {
        std::filesystem::path path = cacheFolder / filename;
        if (std::filesystem::is_regular_file(path))
        {
            std::streampos fileSize;
            std::ifstream file(path, std::ios::binary);
			file.seekg(0, std::ios::end);
			fileSize = file.tellg();
			file.seekg(0, std::ios::beg);
            std::vector<uint32_t> binaryCache(fileSize / sizeof(uint32_t));
            file.read(reinterpret_cast<char*>(binaryCache.data()), fileSize);
            file.close();

            GLenum type = GL_MESH_SHADER_NV;
            if (path.extension() == ".mesh")
            {
                type = GL_MESH_SHADER_NV;
            }
            else if (path.extension() == ".vert")
            {
                type = GL_VERTEX_SHADER;
            }
            else if (path.extension() == ".frag")
            {
                type = GL_FRAGMENT_SHADER;
            }

            m_Id = glCreateShader(type);
            GLsizei size = static_cast<GLsizei>(binaryCache.size()) * sizeof(uint32_t);
            glShaderBinary(1, &m_Id, GL_SHADER_BINARY_FORMAT_SPIR_V, binaryCache.data(), size);

            glSpecializeShader(m_Id, "main", 0, nullptr, nullptr);

            GLint compiled;
            glGetShaderiv(m_Id, GL_COMPILE_STATUS, &compiled);
            if (compiled == GL_FALSE)
            {
                GLint maxLength = 0;
                glGetShaderiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);

                // The maxLength includes the NULL character
                std::vector<GLchar> compiler_log(maxLength);
                glGetShaderInfoLog(m_Id, static_cast<GLsizei>(compiler_log.size()), nullptr, compiler_log.data());
                glDeleteShader(m_Id);
                m_Id = 0;

				LOG_RUNTIME_ERROR("shader {0} contains error(s):\n\n{1}", path.filename().string().c_str(), compiler_log.data());
            }
        }
        else
        {
            LOG_RUNTIME_ERROR("Shader: {} not exists!", path.string().c_str());
        }
    }

    GLuint GetID() const
    {
        return m_Id;
    }

    static void GenerateCaches()
    {
        for (const auto& entry : std::filesystem::directory_iterator(folder))
        {
			std::filesystem::path path = folder / entry.path().filename();
            if (std::filesystem::is_regular_file(path))
			{
                if (!std::filesystem::exists(cacheFolder))
                {
                    std::filesystem::create_directory(cacheFolder);
                }
				std::filesystem::path cachePath = cacheFolder / entry.path().filename();
                if (std::filesystem::is_regular_file(cachePath)) return;
                
                std::fstream file(path);
                std::string source(std::istreambuf_iterator<char>(file), {});
                file.close();

                shaderc_shader_kind kind = shaderc_mesh_shader;
                if (path.extension() == ".mesh")
                {
                    kind = shaderc_mesh_shader;
                }
                else if (path.extension() == ".vert")
                {
                    kind = shaderc_vertex_shader;
                }
                else if (path.extension() == ".frag")
                {
                    kind = shaderc_fragment_shader;
                }

                shaderc::Compiler compiler;
                shaderc::CompileOptions options;
                options.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
                options.SetOptimizationLevel(shaderc_optimization_level_zero);
                options.SetGenerateDebugInfo();
                shaderc::SpvCompilationResult result =
                    compiler.CompileGlslToSpv(source, kind, path.filename().string().c_str(), options);
                if (result.GetCompilationStatus() != shaderc_compilation_status_success)
                {
                    LOG_RUNTIME_ERROR("\n" + result.GetErrorMessage());
                }
                else
                {
                    file.open(cachePath, std::ios::out | std::ios::trunc | std::ios::binary);
                    file.write(reinterpret_cast<const char*>(result.cbegin()), std::distance(result.cbegin(), result.cend()) * sizeof(uint32_t));
                    file.close();
                }
            }
        }
    }

    static GLuint CreateProgram(const Shader &first, const Shader &second)
    {
        GLuint program = glCreateProgram();
        glAttachShader(program, first.GetID());
        glAttachShader(program, second.GetID());
        glLinkProgram(program);

        GLint linked;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

            // The maxLength includes the NULL character
            std::vector<GLchar> compiler_log(maxLength);
            glGetProgramInfoLog(program, static_cast<GLsizei>(compiler_log.size()), nullptr, compiler_log.data());
            glDeleteProgram(program);
            program = 0;

            LOG_RUNTIME_ERROR("program contains error(s):\n\n{}", compiler_log.data());
        }

        glDetachShader(program, first.GetID());
        glDetachShader(program, second.GetID());
        return program;
    }

    static GLuint CreateProgram(const char* first, const char* second)
    {
        return CreateProgram(Shader(first), Shader(second));
    }

  private:
    static const std::filesystem::path folder;
    static const std::filesystem::path cacheFolder;

    GLuint m_Id = 0;
};

const std::filesystem::path Shader::folder = "Assets/Shaders/";
const std::filesystem::path Shader::cacheFolder = "Assets/ShaderCache/";
