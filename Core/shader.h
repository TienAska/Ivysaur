#pragma once

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>

class Shader
{
public:
    Shader(const char* filename) : m_SourcePath(s_SourceFolder / filename), m_BinaryPath(s_BinaryFolder / filename), m_Type(GL_TASK_SHADER_NV), m_Id(0)
    {
        if (std::filesystem::is_regular_file(m_SourcePath))
        {
            if (m_SourcePath.extension() == ".mesh")
                m_Type = GL_MESH_SHADER_NV;
            else if (m_SourcePath.extension() == ".frag")
                m_Type = GL_FRAGMENT_SHADER;

            //m_Id = glCreateShader(m_Type);
        }
        else
        {
            LOG_RUNTIME_ERROR("Shader source: {} not exists", m_SourcePath.string());
        }
    }

    ~Shader()
    {
        //glDeleteShader(m_Id);
        if (m_Id != 0)
        {
            glDeleteProgram(m_Id);
        }
    }

    bool Compile()
    {
        bool needUpdate = true;
        if (std::filesystem::exists(m_BinaryPath))
        {
            std::filesystem::file_time_type sourceTime = std::filesystem::last_write_time(m_SourcePath);
            std::filesystem::file_time_type binaryTime = std::filesystem::last_write_time(m_BinaryPath);
            needUpdate = sourceTime > binaryTime;
        }
        if (true)
        {
            std::ifstream file(m_SourcePath);
            std::string source{ std::istreambuf_iterator<char>(file), {} };
            file.close();
            const char* src_Str = source.c_str();

            m_Id = glCreateShaderProgramv(m_Type, 1, &src_Str);
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

                LOG_RUNTIME_ERROR("Program {0} contains error(s):\n\n{1}", m_SourcePath.filename().string().c_str(), linker_log.data());
            }
            return linked;

            //glShaderSource(m_Id, 1, &src_Str, nullptr);
            //glCompileShader(m_Id);

            //GLint compiled; glGetShaderiv(m_Id, GL_COMPILE_STATUS, &compiled);
            //if (compiled)
            //{
            //    GLuint program = glCreateProgram();
            //    glProgramParameteri(program, GL_PROGRAM_SEPARABLE, GL_TRUE);
            //    glAttachShader(program, m_Id);
            //    glLinkProgram(program);
            //    glDetachShader(program, m_Id);

            //    glDeleteShader(m_Id);
            //    m_Id = program;
            //    GLint linked; glGetProgramiv(m_Id, GL_LINK_STATUS, &linked);
            //    if (linked)
            //    {
            //        Save();
            //    }
            //    else
            //    {
            //        GLint maxLength = 0;
            //        glGetProgramiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);

            //        // The maxLength includes the NULL character
            //        std::vector<GLchar> linker_log(maxLength);
            //        glGetProgramInfoLog(m_Id, maxLength, nullptr, linker_log.data());
            //        glDeleteProgram(m_Id);
            //        m_Id = 0;

            //        LOG_RUNTIME_ERROR("Program {0} contains error(s):\n\n{1}", m_SourcePath.filename().string().c_str(), linker_log.data());
            //    }

            //    return linked;
            //}
            //else
            //{
            //    GLint maxLength = 0;
            //    glGetShaderiv(m_Id, GL_INFO_LOG_LENGTH, &maxLength);

            //    // The maxLength includes the NULL character
            //    std::vector<GLchar> compiler_log(maxLength);
            //    glGetShaderInfoLog(m_Id, maxLength, nullptr, compiler_log.data());
            //    glDeleteShader(m_Id);
            //    m_Id = 0;

            //    LOG_RUNTIME_ERROR("Shader {0} contains error(s):\n\n{1}", m_SourcePath.filename().string().c_str(), compiler_log.data());
            //}


            //return compiled;
        }
        else
        {
            return Load();
        }
    }

    GLuint GetID() const
    {
        return m_Id;
    }

    const std::filesystem::path& GetPath() const
    {
        return m_SourcePath;
    }  

private:
    GLuint m_Id;
    GLenum m_Type;
    std::filesystem::path m_SourcePath;
    std::filesystem::path m_BinaryPath;

    static const std::filesystem::path s_SourceFolder;
    static const std::filesystem::path s_BinaryFolder;

    void Save()
    {
        GLint length;
		glGetProgramiv(m_Id, GL_PROGRAM_BINARY_LENGTH, &length);
		char* binary = new char[length];

        GLenum format;
		glGetProgramBinary(m_Id, length, nullptr, &format, binary);

		std::fstream file(m_BinaryPath, std::ios::out | std::ios::trunc | std::ios::binary);
		file.write(binary, length);
		file.close();
		delete[] binary;
    }

	bool Load()
	{
		std::streampos fileSize;
		std::ifstream file(m_BinaryPath, std::ios::binary);
		file.seekg(0, std::ios::end);
		fileSize = file.tellg();
		file.seekg(0, std::ios::beg);
		char* binary = new char[fileSize];
		file.read(binary, fileSize);
		file.close();

		GLint* formats = new GLint;
		glGetIntegerv(GL_PROGRAM_BINARY_FORMATS, formats);
		glProgramBinary(m_Id, static_cast<GLenum>(*formats), binary, static_cast<GLsizei>(fileSize));
		delete formats;
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
        return linked;
	}
};

const std::filesystem::path Shader::s_SourceFolder = "Assets/Shaders/";
const std::filesystem::path Shader::s_BinaryFolder = "Assets/ShaderCache/";


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
		if (!m_Task->Compile() || !m_Mesh->Compile() || !m_Frag->Compile())
			return false;

		glAttachShader(m_Id, m_Task->GetID());
		glAttachShader(m_Id, m_Mesh->GetID());
		glAttachShader(m_Id, m_Frag->GetID());
		glLinkProgram(m_Id);
		glDetachShader(m_Id, m_Task->GetID());
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

    bool Link(std::shared_ptr<Shader> task, std::shared_ptr<Shader> mesh, std::shared_ptr<Shader> frag)
    {
        m_Task = task;
		m_Mesh = mesh;
		m_Frag = frag;

        NameThePath(
            m_Task->GetPath().filename().stem().string().c_str(),
            m_Mesh->GetPath().filename().stem().string().c_str(),
            m_Frag->GetPath().filename().stem().string().c_str()
        );

        bool needUpdate = true;
        if (std::filesystem::exists(m_Path))
        {
            std::filesystem::file_time_type programTime = std::filesystem::last_write_time(m_Path);
            std::filesystem::file_time_type taskTime = std::filesystem::last_write_time(m_Task->GetPath());
            std::filesystem::file_time_type meshTime = std::filesystem::last_write_time(m_Mesh->GetPath());
            std::filesystem::file_time_type fragTime = std::filesystem::last_write_time(m_Frag->GetPath());

            needUpdate = (taskTime > programTime || meshTime > programTime || fragTime > programTime);
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
    std::shared_ptr<Shader> m_Task = nullptr, m_Mesh = nullptr, m_Frag = nullptr;
    std::filesystem::path m_Path;

    static const std::filesystem::path s_Folder;

    void NameThePath(const char* task, const char* mesh, const char* frag)
    {
		if (!std::filesystem::exists(s_Folder))
			std::filesystem::create_directory(s_Folder);

        size_t len = strlen(task) + 1 + strlen(mesh) + 1 + strlen(frag) + 4 + 1;
        char* filename = new char[len];
        filename[0] = '\0';
        strcat_s(filename, len, task);
        strcat_s(filename, len, "_");
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
};

const std::filesystem::path Program::s_Folder = "Assets/ShaderCache/";
