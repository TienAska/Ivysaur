#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>

class Shader
{
public:
    Shader(const char* filename) : m_Path(s_Folder / filename), m_Type(GL_MESH_SHADER_NV), m_Id(0)
    {
        if (std::filesystem::is_regular_file(m_Path))
        {
            if (m_Path.extension() == ".frag")
                m_Type = GL_FRAGMENT_SHADER;

            m_Id = glCreateShader(m_Type);
        }
        else
        {
            LOG_RUNTIME_ERROR("Shader source: {} not exists", m_Path.string());
        }
    }

    ~Shader()
    {
        glDeleteShader(m_Id);
    }

    bool Compile()
    {
        std::ifstream file(m_Path);
        std::string source{ std::istreambuf_iterator<char>(file), {} };
        file.close();
        const char* src_Str = source.c_str();

        glShaderSource(m_Id, 1, &src_Str, nullptr);

        glCompileShader(m_Id);

        GLint compiled; glGetShaderiv(m_Id, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_FALSE)
        {
            GLint maxLength = 0;
            glGetShaderiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);
            
            // The maxLength includes the NULL character
            std::vector<GLchar> compiler_log(maxLength);
            glGetShaderInfoLog(m_Id, maxLength, nullptr, compiler_log.data());
            glDeleteShader(m_Id);
            m_Id = 0;

            LOG_RUNTIME_ERROR("Shader {0} contains error(s):\n\n{1}", m_Path.filename().string().c_str(), compiler_log.data());
        }

        return compiled;
    }

    GLuint GetID() const
    {
        return m_Id;
    }

    const std::filesystem::path& GetPath() const
    {
        return m_Path;
    }  

private:
    GLuint m_Id;
    GLenum m_Type;
    std::filesystem::path m_Path;

    static const std::filesystem::path s_Folder;
};

const std::filesystem::path Shader::s_Folder = "Assets/Shaders/";


class Program
{
public:
    Program()
    {
        m_Id = glCreateProgram();
    }

    ~Program()
    {
		glDeleteProgram(m_Id);
    }

    GLuint GetID() const
    {
        return m_Id;
    }

    bool Update()
    {
		if (!m_Mesh->Compile() || !m_Frag->Compile())
			return false;

		glAttachShader(m_Id, m_Mesh->GetID());
		glAttachShader(m_Id, m_Frag->GetID());
		glLinkProgram(m_Id);
		glDetachShader(m_Id, m_Mesh->GetID());
		glDetachShader(m_Id, m_Frag->GetID());

		GLint linked; glGetProgramiv(m_Id, GL_LINK_STATUS, &linked);
		if (linked)
		{
			Save();
		}
		else
		{
			GLint maxLength = 0;
			glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> linker_log(maxLength);
			glGetProgramInfoLog(m_Id, maxLength, nullptr, linker_log.data());
			glDeleteProgram(m_Id);
			m_Id = 0;

			LOG_RUNTIME_ERROR("program contains error(s):\n\n{}", linker_log.data());
		}
		return linked;
    }

    bool Link(std::shared_ptr<Shader> mesh, std::shared_ptr<Shader> frag)
    {
		m_Mesh = mesh;
		m_Frag = frag;

        NameThePath(mesh->GetPath().filename().stem().string().c_str(), frag->GetPath().filename().stem().string().c_str());
        bool needUpdate = true;
        if (std::filesystem::exists(m_Path))
        {
            std::filesystem::file_time_type programTime = std::filesystem::last_write_time(m_Path);
            std::filesystem::file_time_type meshTime = std::filesystem::last_write_time(mesh->GetPath());
            std::filesystem::file_time_type fragTime = std::filesystem::last_write_time(frag->GetPath());

            needUpdate = (meshTime > programTime || fragTime > programTime);
        }

        if (needUpdate)
        {
            return Update();
        }
        else
        {
            Load();

            return true;
        }
    }

    void Use()
    {
        glUseProgram(m_Id);
    }

private:
    GLuint m_Id = 0;
    GLenum m_Format = GL_NONE;
    GLsizei m_Length = 0;
    std::shared_ptr<Shader> m_Mesh = nullptr, m_Frag = nullptr;
    std::filesystem::path m_Path;

    static const std::filesystem::path s_Folder;

    void NameThePath(const char* mesh, const char* frag)
    {
		if (!std::filesystem::exists(s_Folder))
			std::filesystem::create_directory(s_Folder);

        size_t len = strlen(mesh) + 1 + strlen(frag) + 4 + 1;
        char* filename = new char[len];
        filename[0] = '\0';
        strcat_s(filename, len, mesh);
        strcat_s(filename, len, "_");
        strcat_s(filename, len, frag);
        strcat_s(filename, len, ".bin");
        m_Path = s_Folder / filename;
        delete[] filename;
    }

    void Save()
    {
        glGetProgramiv(m_Id, GL_PROGRAM_BINARY_LENGTH, &m_Length);
        char* binary = new char[m_Length];

        glGetProgramBinary(m_Id, m_Length, nullptr, &m_Format, binary);

		std::fstream file(m_Path, std::ios::out | std::ios::trunc | std::ios::binary);
		file.write(binary, m_Length);
		file.close();
        delete[] binary;
    }

    void Load()
    {
		std::streampos fileSize;
		std::ifstream file(m_Path, std::ios::binary);
		file.seekg(0, std::ios::end);
		fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		char* binary = new char[fileSize];
		file.read(binary, fileSize);
		file.close();

        GLint* formats = new GLint;
        glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, formats);
        m_Format = static_cast<GLenum>(*formats);
        delete formats;
        glProgramBinary(m_Id, m_Format, binary, static_cast<GLsizei>(fileSize));
        delete[] binary;

		GLint linked; glGetProgramiv(m_Id, GL_LINK_STATUS, &linked);
		if (linked == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);

			// The maxLength includes the NULL character
			std::vector<GLchar> linker_log(maxLength);
			glGetProgramInfoLog(m_Id, maxLength, nullptr, linker_log.data());
			glDeleteProgram(m_Id);
			m_Id = 0;

			LOG_RUNTIME_ERROR("program contains error(s):\n\n{}", linker_log.data());
		}
    }

    void Delete()
    {
		glDeleteProgram(m_Id);
		m_Id = 0;
    }
};

const std::filesystem::path Program::s_Folder = "Assets/ShaderCache/";
