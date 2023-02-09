#pragma once

#ifdef _WIN32
#include <glad/glad.h>
#else
#include <GLES3/gl3.h>
#endif
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <unordered_map>
#include <optional>
#include <stack>
#include <map>
#include <vector>

#include "imr_core.h"
#include "imr_spine.h"
#include "imr_text.h"
#include "imr_scene.h"
#include "tweeners.h"
#include "animation.h"

#define __GL_ASSERT__
#ifdef __GL_ASSERT__
#define GL_ASSERT() \
do { \
GLenum err; \
while ((err = glGetError()) != GL_NO_ERROR) \
{ \
	assert(false); \
} \
} while (false)
#else
#define GL_ASSERT()
#endif

#define CTX imr::context::instance()

namespace imr
{
	inline static int TEXTURE_REG_0 = 100;
	inline static int TEXTURE_REG_1 = 101;
	inline static int TEXTURE_REG_2 = 102;
	inline static int TEXTURE_REG_3 = 103;
	inline static int TIME_REG = 110;
	struct context;

	bool extension_supported(const char*);

	class array_buffer
	{
	public:
		void bind()
		{
			assert(_buffer > 0);
			glBindBuffer(_target, _buffer);
		}

		array_buffer() = delete;
		array_buffer(int size, GLenum target, GLenum usage, const void* data = nullptr)
		{
			_target = target;
			_usage = usage;
			_capacity = imr::util::min_power_of_2(size);
			glGenBuffers(1, &_buffer);
			glBindBuffer(target, _buffer);
			glBufferData(target, _capacity, data, usage);
			glBindBuffer(target, 0);
		}

		~array_buffer()
		{
			glDeleteBuffers(1, &_buffer);
		}

		void unbind()
		{
			glBindBuffer(_target, 0);
		}

		void sub_data(int offset, int size, const void* data)
		{
			assert(size <= _capacity);
			size = std::min(size, _capacity);
			glBufferSubData(_target, offset, size, data);
			GL_ASSERT();
		}

		std::tuple<int, GLenum, GLenum> key()
		{
			return { _capacity, _target, _usage };
		}

	private:
		GLuint _buffer = 0;
		GLenum _target = {};
		GLenum _usage = {};
		int _capacity = {};
	};

	inline std::unique_ptr<array_buffer> create_array_buffer(const void* data, int size, GLenum target, GLenum usage)
	{
		auto arr_buf = std::make_unique<array_buffer>(size, target, usage, data);
		return std::move(arr_buf);
	}

	struct texture_info : Itexture_info
	{
		std::string texture_name = {};
		GLuint resource = {};
		int _width = {};
		int _height = {};

		~texture_info()
		{
			if (resource > 0)
				destroy();
		}

		const std::string& name() const override { return texture_name; }
		void set_name(const std::string& n) override { texture_name = n; }
		int2 size() const override { return { _width, _height }; }
		int width() const override { return this->_width; }
		int height() const override { return this->_height; }
		void bind() const override;
		void unbind() const override;
		void destroy() override;
	};

	struct frame_buffer : Iframe_buffer
	{
	private:
		int _width = 0;
		int _height = 0;
		bool _created = false;
		int _attachment_count = 1;
		inline const static unsigned int _attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };

	public:
		GLuint buffer = 0;
		GLuint color_textures[4] = {};

		bool discard_on_resolution_changed = true;

		uintptr_t get_color_texture(int idx = 0) override
		{
			return color_textures[idx];
		}

		~frame_buffer()
		{
			if (_created)
			{
				destory();
			}
		}

		imr::result create(int w, int h, int attachment_count = 1) override
		{
			assert(attachment_count <= 4);
			_attachment_count = attachment_count;
			imr::result ret;

			if (w > 0 && h > 0)
			{
				_width = w;
				_height = h;
			}
			else
			{
				ret.type = imr::result_type::fail;
				ret.error_code = 1;
				ret.msg = "frame buffer size must larger than zero";
				return ret;
			}

			glGenFramebuffers(1, &buffer);
			assert(buffer > 0);
			if (buffer <= 0)
			{
				ret.type = imr::result_type::fail;
				ret.error_code = 2;
				ret.msg = "create frame buffers fail";
				return ret;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, buffer);

			glGenTextures(attachment_count, color_textures);
			for (int i = 0; i < attachment_count; ++i)
			{
				assert(color_textures[i] > 0);
				if (color_textures[i] <= 0)
				{
					ret.type = imr::result_type::fail;
					ret.error_code = 3;
					ret.msg = "create color texture fail";
					return ret;
				}

				glBindTexture(GL_TEXTURE_2D, color_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, _attachments[i], GL_TEXTURE_2D, color_textures[i], 0);
			}

			GLuint depth_texture = 0;
			glGenTextures(1, &depth_texture);
			glBindTexture(GL_TEXTURE_2D, depth_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, _width, _height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_texture, 0);

			glBindTexture(GL_TEXTURE_2D, 0);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			_created = true;
			return ret;
		}

		void destory() override
		{
			glDeleteFramebuffers(1, &buffer);
			buffer = 0;
			for (int i = 0; i < 4; ++i)
			{
				if (color_textures[i] > 0)
					glDeleteTextures(1, &color_textures[i]);
				color_textures[i] = 0;
			}
		}

		void bind() override
		{
			glBindFramebuffer(GL_FRAMEBUFFER, buffer);
			glDrawBuffers(_attachment_count, _attachments);
		}

		void unbind() override
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		int width() const override { return _width; }
		int height() const override { return _height; }
		void bind_color_texture(int attachment_idx = 0) override
		{
			glBindTexture(GL_TEXTURE_2D, color_textures[attachment_idx]);
		}
	};

	class program : public Iprogram
	{
	public:
		program(GLuint prg)
			:_program(prg)
		{
		}
		void use() override;
		void unuse() override;
		void destroy() override;
		void bind_attrib_location(int reg, const char* attr)
		{
			auto loc = glGetAttribLocation(_program, attr);
			assert(loc >= 0);
			_attrib_loc[reg] = loc;
		}

		void bind_uniform_location(int reg, const char* uni)
		{
			auto loc = glGetUniformLocation(_program, uni);
			assert(loc >= 0);
			_uniform_loc[reg] = loc;
		}

		GLint get_attrib_location(int reg)
		{
			if (_attrib_loc.find(reg) == _attrib_loc.end())
			{
				assert(false);
				return -1;
			}
			return _attrib_loc[reg];
		}

		const GLint get_uniform_location(int reg) const
		{
			if (_uniform_loc.find(reg) == _uniform_loc.end())
			{
				assert(false);
				return -1;
			}
			return _uniform_loc.at(reg);
		}
	private:
		GLuint _program = 0;
		std::unordered_map<int, GLint> _attrib_loc = {};
		std::unordered_map<int, GLint> _uniform_loc = {};
	};

	class program_builder
	{
	public:
		static std::tuple<imr::result, std::shared_ptr<program>> build(const char* vs, const char* fs);
	private:
		static std::tuple<imr::result, GLuint> load_shader(GLenum type, const char* code);
	};

	struct camera_state
	{
		bool begin = false;
		glm::mat4x4 projection = glm::mat4x4(1.0f);
		glm::mat4x4 view = glm::mat4x4(1.0f);
		Iframe_buffer* frame = {};
		bool try_batch = false;
		float4 world_rect = {};
		std::vector<imr::sprite::draw_args> batches = {};
	};

	struct viewport_state
	{
		int x = 0;
		int y = 0;
		int width = 0;
		int height = 0;
	};

	struct blend_func_state
	{
		GLenum src = {};
		GLenum dst = {};
	};

	struct instancing_state
	{
		static const int MAX_INSTSANCE_COUNT = 100000;
		static const int INSTANCE_FORMAT_COUNT = 20; // translate, scale / rotation, width, height, rev / uv_rect / color / offset, rev
		static const int INSTANCE_FORMAT_SIZE = INSTANCE_FORMAT_COUNT * sizeof(float); // translate, scale / rotation, width, height, rev / uv_rect / color / offset, rev
		inline static std::vector<float> SHARE_BUFFER = std::vector<float>(MAX_INSTSANCE_COUNT * INSTANCE_FORMAT_SIZE);
		bool begin = false;
		int instance_count = 0;
		const Itexture_info* texture_info = {};
		const Itexture_info* texture_info_1 = {};
		const Itexture_info* texture_info_2 = {};
		const Itexture_info* texture_info_3 = {};
	};

	struct mesh_state
	{
		struct vert_attrib_pointer
		{
			int location;
			int count;
			int stride;
			int offset;
		};
		bool begin = false;
		bool use_project_view_matrix = true;
		imr::program* program = {};
		std::vector<std::pair<int, Itexture_info*>> textures = {};
		std::vector<vert_attrib_pointer> vert_attrib_pointers = {};
		inline static std::vector<float> vertices = {};
		inline static std::vector<unsigned short> indices = {};
	};

	struct text_state
	{
		imr::text::font_info* font_info = {};
	};

	struct context
	{
		inline static context* instance()
		{
			static context ctx = {};
			return &ctx;
		}

		std::stack<camera_state> camera_stack = {};
		std::stack<viewport_state> viewport_stack = {};
		std::stack<blend_func_state> blend_func_stack = {};
		std::stack<instancing_state> instancing_stack = {};
		std::stack<std::string> program_stack = {};
		std::stack<mesh_state> mesh_stack = {};
		std::stack<text_state> text_stack = {};
		std::shared_ptr<Itexture_info> white_texture_info = {};
		std::unordered_map<std::string, std::shared_ptr<imr::Iprogram>> programs = {};
		GLuint quad_vao = 0;
		GLuint instancing_attrib_vao = 0;
		GLuint temp_vao = {};
		std::map<std::tuple<int, GLenum, GLenum>, std::stack<std::unique_ptr<array_buffer>>> array_buffer_pool = {};

		std::unique_ptr<array_buffer> get_array_buffer(int size, GLenum target, GLenum usage)
		{
			auto& pool = array_buffer_pool[std::make_tuple(imr::util::min_power_of_2(size), target, usage)];
			if (pool.empty())
			{
				return create_array_buffer(nullptr, size, target, usage);
			}
			else
			{
				auto ret = std::move(pool.top());
				pool.pop();
				return std::move(ret);
			}
		}

		void release_array_buffer(std::unique_ptr<array_buffer> ptr)
		{
			auto& pool = array_buffer_pool[ptr->key()];
			pool.push(std::move(ptr));
		}

		result create();
		void destroy();
	};
}
