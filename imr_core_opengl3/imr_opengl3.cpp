#include "imr_opengl3.h"
#include <stack>
#include <optional>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <unordered_map>
#include <functional>
#include <filesystem>

namespace
{
	const int texture_regs[4] = { imr::TEXTURE_REG_0, imr::TEXTURE_REG_1, imr::TEXTURE_REG_2, imr::TEXTURE_REG_3 };

	void bind_multi_textures(const imr::program* program, const imr::Itexture_info* _1, const imr::Itexture_info* _2, const imr::Itexture_info* _3)
	{
		const imr::Itexture_info* textures[3] = { _1, _2, _3 };

		const int active_idx[3] = { GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3 };
		for (int i = 0; i < 3; ++i)
		{
			if (textures[i])
			{
				glUniform1i(program->get_uniform_location(texture_regs[i + 1]), i + 1);
				glActiveTexture(active_idx[i]);
				textures[i]->bind();
			}
			else
			{
				glActiveTexture(active_idx[i]);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
		}
	}
}

namespace imr
{
	void frame_buffer_texture_info::unbind() const
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	result atlas_info::add_sprite_info(const std::string& sprite_name, int2 position, int2 size, float2 offset)
	{
		assert(_texture_info);
		float4 uv_rect = {};
		uv_rect.xy = position / _texture_info->size();
		uv_rect.zw = (position + size) / _texture_info->size();
		uv_rect.y = 1.0f - uv_rect.y;
		uv_rect.w = 1.0f - uv_rect.w;
		_sprites[sprite_name] = { position, size, offset, uv_rect };
		return {};
	}

	result atlas_info::remove_sprite_info(const std::string& sprite_name)
	{
		assert(_sprites.contains(sprite_name));
		_sprites.erase(sprite_name);
		return {};
	}

	const Itexture_info* get_white_texture_info()
	{
		return CTX->white_texture_info.get();
	}
}

namespace imr
{
	void frame_backbuffer::bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}

namespace imr
{
	bool extension_supported(const char* extension)
	{
#ifndef OPENGL_ES       
		GLint nNumExtensions;
		glGetIntegerv(GL_NUM_EXTENSIONS, &nNumExtensions);

		for (GLint i = 0; i < nNumExtensions; i++)
			if (strcmp(extension, (const char*)glGetStringi(GL_EXTENSIONS, i)) == 0)
				return 1;
#else
		GLubyte* extensions = NULL;
		const GLubyte* start;
		GLubyte* where, * terminator;

		where = (GLubyte*)strchr(extension, ' ');
		if (where || *extension == '\0')
			return 0;

		extensions = (GLubyte*)glGetString(GL_EXTENSIONS);

		start = extensions;
		for (;;)
		{
			where = (GLubyte*)strstr((const char*)start, extension);

			if (!where)
				break;

			terminator = where + strlen(extension);

			if (where == start || *(where - 1) == ' ')
			{
				if (*terminator == ' ' || *terminator == '\0')
					return 1;
			}
			start = terminator;
		}
#endif
		return 0;
	}

	result initialize()
	{
		return CTX->create();
	}

	result deinitialize()
	{
		CTX->destroy();
		return {};
	}

	void program::use()
	{
		glUseProgram(_program);
	}

	void program::unuse()
	{
		glUseProgram(0);
	}

	void program::destroy()
	{
		if (_program > 0)
		{
			glDeleteProgram(_program);
			_program = 0;
		}
	}

	std::tuple<result, std::shared_ptr<program>> program_builder::build(const char* vs, const char* fs)
	{
		result ret;
		auto [rst_v, vertex_shader] = load_shader(GL_VERTEX_SHADER, vs);
		if (rst_v.type != imr::result_type::success)
		{
			return { rst_v, { 0 } };
		}
		auto [rst_f, fragment_shader] = load_shader(GL_FRAGMENT_SHADER, fs);
		if (rst_f.type != imr::result_type::success)
		{
			return { rst_f, { 0 } };
		}

		GLuint prog = glCreateProgram();
		glAttachShader(prog, vertex_shader);
		glAttachShader(prog, fragment_shader);
		glLinkProgram(prog);

		glDeleteShader(vertex_shader);
		glDeleteShader(fragment_shader);

		GLint status;
		glGetProgramiv(prog, GL_LINK_STATUS, &status);
		if (status == GL_FALSE)
		{
			ret.type = imr::result_type::fail;
			ret.error_code = 1;

			GLint logLength = 0;
			glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
			GLchar* log = new GLchar[logLength + 1];
			glGetProgramInfoLog(prog, logLength, &logLength, log);
			ret.msg = log;
			delete[] log;
			return { ret, { 0 } };
		}

		return { ret, std::make_shared<program>(prog) };
	}

	std::tuple<result, GLuint> program_builder::load_shader(GLenum type, const char* code)
	{
		result ret = {};
		GLuint shader = glCreateShader(type);
		const char* precision = "";
		if (type == GL_FRAGMENT_SHADER)
		{
			precision = "precision mediump float;";
		};
		const char* codes[] = {
			"#version 300 es\n",
			precision,
			code
		};
		glShaderSource(shader, 3, codes, NULL);
		glCompileShader(shader);
		GLint status;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE)
		{
			ret.type = result_type::fail;
			ret.error_code = 1;
			GLint infoLogLength;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
			GLchar* strInfoLog = new GLchar[infoLogLength + 1];
			glGetShaderInfoLog(shader, infoLogLength, NULL, strInfoLog);
			ret.msg = strInfoLog;
			delete[] strInfoLog;
			return { ret, 0 };
		}
		return { ret, shader };
	}

	result context::create()
	{
		const char* instancing_vs = R"(
			in vec4 aPosition;
			in vec4 aTranslateScale;
			in vec4 aRotationWidthHeightRev;
			in vec4 aUVRect;
			in vec4 aColor;
			in vec4 aOffsetRev;

			uniform mat4 uProjectionMatrix;
			uniform mat4 uViewMatrix;

			out vec2 vScreenPos;
			out vec2 vTexCoord;
			out vec4 vColor;
			out float vScaleX;

			void main()
			{
				vec4 pos = aPosition - vec4(aOffsetRev.xy, 0, 0);

				// size, scale
				pos.xy *= aRotationWidthHeightRev.yz;
				pos.xy *= aTranslateScale.zw;
				// rotation
				float rotation = aRotationWidthHeightRev.x;
				float tmp_x = pos.x;
				pos.x = pos.x * cos(rotation) - pos.y * sin(rotation);
				pos.y = tmp_x * sin(rotation) + pos.y * cos(rotation);
				// transition
				pos.xy += aTranslateScale.xy;

				// coord
				int vertIdx = int(aPosition.z);
				vec2 coord = vec2(0, 0);
				if (vertIdx == 0) {
					coord.x = aUVRect.x;
					coord.y = aUVRect.y;
				}
				else if (vertIdx == 1) {
					coord.x = aUVRect.x;
					coord.y = aUVRect.w;
				}
				else if (vertIdx == 2) {
					coord.x = aUVRect.z;
					coord.y = aUVRect.w;
				}
				else if (vertIdx == 3) {
					coord.x = aUVRect.z;
					coord.y = aUVRect.y;
				}

				// pixel perfect
				pos = floor(pos + 0.5);

				// output
				gl_Position = uProjectionMatrix * uViewMatrix * vec4(pos.xy, 0.1f, 1);
				vScreenPos = (gl_Position.xy + vec2(1.0, 1.0)) * 0.5;
				vTexCoord = coord;
				vColor = aColor;
				vScaleX = aTranslateScale.z;
			}
		)";

		const char* default_ps = R"(
				//precision highp float;

                uniform sampler2D uSampler;
                uniform sampler2D uNormalSampler;

                in vec2 vTexCoord;
                in vec4 vColor;
				in float vScaleX;

				out vec4 OutColor[4];

                void main()
                {
                    vec4 col = texture(uSampler, vTexCoord) * vColor;
                    vec4 nor = texture(uNormalSampler, vTexCoord);
					if (vScaleX < 0.0)
					{
						float nx = -1.0 * (nor.x * 2.0 - 1.0);
						nx = (nx + 1.0) / 2.0;
						nor.x = nx;
					}
					nor.a = col.a;
                    OutColor[0] = col;
					OutColor[1] = nor;
                }
		)";
		{
			auto [res, instancing_program] = program_builder::build(
				instancing_vs,
				default_ps
			);
			if (res.type == result_type::fail)
			{
				return res;
			}
			instancing_program->bind_attrib_location(0, "aPosition");
			instancing_program->bind_attrib_location(1, "aTranslateScale");
			instancing_program->bind_attrib_location(2, "aRotationWidthHeightRev");
			instancing_program->bind_attrib_location(3, "aUVRect");
			instancing_program->bind_attrib_location(4, "aColor");
			instancing_program->bind_attrib_location(5, "aOffsetRev");
			instancing_program->bind_uniform_location(0, "uProjectionMatrix");
			instancing_program->bind_uniform_location(1, "uViewMatrix");
			instancing_program->bind_uniform_location(TEXTURE_REG_0, "uSampler");
			instancing_program->bind_uniform_location(TEXTURE_REG_1, "uNormalSampler");
			regist_program(INSTANCING_PROGRAM_NAME, instancing_program);
		}

		{
			auto [res, text_program] = program_builder::build(
				instancing_vs,
				R"(
					uniform sampler2D uSampler;

					in vec2 vTexCoord;
					in vec4 vColor;

					out vec4 OutColor[2];

					void main()
					{
						vec4 col = texture(uSampler, vTexCoord);
						float r = col.r > 0.0f ? 1.0f : 0.0f;
						col = vec4(r, r, r, col.r);
						OutColor[0] = col * vColor;
						OutColor[1] = vec4(0.0, 0.0, 1.0, 1.0);
					}	
			)");
			if (res.type == result_type::fail)
			{
				return res;
			}
			text_program->bind_attrib_location(0, "aPosition");
			text_program->bind_attrib_location(1, "aTranslateScale");
			text_program->bind_attrib_location(2, "aRotationWidthHeightRev");
			text_program->bind_attrib_location(3, "aUVRect");
			text_program->bind_attrib_location(4, "aColor");
			text_program->bind_attrib_location(5, "aOffsetRev");
			text_program->bind_uniform_location(0, "uProjectionMatrix");
			text_program->bind_uniform_location(1, "uViewMatrix");
			text_program->bind_uniform_location(TEXTURE_REG_0, "uSampler");
			regist_program(TEXT_PROGRAM_NAME, text_program);
		}

		{
			auto [res, light_program] = program_builder::build(
				instancing_vs
				,
				R"(
					uniform sampler2D uLightSampler;
					uniform sampler2D uLightDirSampler;
					uniform sampler2D uNormalSampler;

					in vec2 vScreenPos;
					in vec2 vTexCoord;
					in vec4 vColor;

					out vec4 OutColor[1];

					void main()
					{
						vec4 lightCol = texture(uLightSampler, vTexCoord);
						vec4 lightDir = texture(uLightDirSampler, vTexCoord);
						vec4 normalCol = texture(uNormalSampler, vScreenPos);
						lightDir.xy = lightDir.xy * 2.0f - vec2(1.0, 1.0);
						normalCol.xy = -(normalCol.xy * 2.0f - vec2(1.0, 1.0));
						float norm = dot(lightDir.xyz, normalCol.xyz);
						vec4 col = lightCol * vColor * norm;
						col.a = lightCol.r * normalCol.a;
						OutColor[0] = col;
					}		
			)");
			if (res.type == result_type::fail)
			{
				return res;
			}
			light_program->bind_attrib_location(0, "aPosition");
			light_program->bind_attrib_location(1, "aTranslateScale");
			light_program->bind_attrib_location(2, "aRotationWidthHeightRev");
			light_program->bind_attrib_location(3, "aUVRect");
			light_program->bind_attrib_location(4, "aColor");
			light_program->bind_attrib_location(5, "aOffsetRev");
			light_program->bind_uniform_location(0, "uProjectionMatrix");
			light_program->bind_uniform_location(1, "uViewMatrix");
			light_program->bind_uniform_location(TEXTURE_REG_0, "uLightSampler");
			light_program->bind_uniform_location(TEXTURE_REG_1, "uLightDirSampler");
			light_program->bind_uniform_location(TEXTURE_REG_2, "uNormalSampler");
			regist_program(LIGHT_PROGRAM_NAME, light_program);
		}

		{
			auto [res, sprite_program] = program_builder::build(
				R"(
				in vec4 aPosition;

				uniform vec4 ub[13];

				out vec2 vTexCoord;
				out vec4 vColor;
				out float vScaleX;

				void main()
				{
					mat4 uProjectionMatrix = mat4(ub[0], ub[1], ub[2], ub[3]);
					mat4 uViewMatrix = mat4(ub[4], ub[5], ub[6], ub[7]);
					vec4 uTranslateScale = ub[8]; 
					vec4 uSizeOffset = ub[9]; 
					vec4 uColor = ub[10]; 
					vec4 uUVRect = ub[11]; 
					float uRotation = ub[12].x; 

					vec4 pos = vec4(aPosition.xy, 0, 1) - vec4(uSizeOffset.zw, 0, 0);

					// size, scale
					pos.xy *= uSizeOffset.xy;
					pos.xy *= uTranslateScale.zw;
					// rotation
					float tmp_x = pos.x;
					pos.x = pos.x * cos(uRotation) - pos.y * sin(uRotation);
					pos.y = tmp_x * sin(uRotation) + pos.y * cos(uRotation);
					// transition
					pos.xy += uTranslateScale.xy;

					// pixel perfect
					pos = floor(pos + 0.5);

					// coord
					int vertIdx = int(aPosition.z);
					vec2 coord = vec2(0, 0);
					if (vertIdx == 0) {
						coord.x = uUVRect.x;
						coord.y = uUVRect.y;
					}
					else if (vertIdx == 1) {
						coord.x = uUVRect.x;
						coord.y = uUVRect.w;
					}
					else if (vertIdx == 2) {
						coord.x = uUVRect.z;
						coord.y = uUVRect.w;
					}
					else if (vertIdx == 3) {
						coord.x = uUVRect.z;
						coord.y = uUVRect.y;
					}

					// output
					gl_Position = uProjectionMatrix * uViewMatrix * vec4(pos.xy, 0.1f, 1);
					vTexCoord = coord;
					vColor = uColor;
					vScaleX = 1.0f;
				}
			)",
				default_ps
			);
			if (res.type == result_type::fail)
			{
				return res;
			}
			sprite_program->bind_attrib_location(0, "aPosition");
			sprite_program->bind_uniform_location(0, "ub[0]");
			sprite_program->bind_uniform_location(TEXTURE_REG_0, "uSampler");
			sprite_program->bind_uniform_location(TEXTURE_REG_1, "uNormalSampler");
			regist_program(SPRITE_PROGRAM_NAME, sprite_program);
		}

		{
			auto [res, mesh_program] = program_builder::build(
				R"(
					in vec4 aPosUV;
					in vec4 aColor;

					uniform mat4 ub[2];

					out vec2 vTexCoord;
					out vec4 vColor;
					out float vScaleX;

					void main()
					{
						mat4 uProjectionMatrix = ub[0];
						mat4 uViewMatrix = ub[1];
						gl_Position = uProjectionMatrix * uViewMatrix * vec4(aPosUV.xy, 0.1f, 1);
						vTexCoord = aPosUV.zw;
						vColor = aColor;
						vScaleX = 1.0f;
					}
			)",
				default_ps
			);
			IMRRESULT(res);
			mesh_program->bind_attrib_location(0, "aPosUV");
			mesh_program->bind_attrib_location(1, "aColor");
			mesh_program->bind_uniform_location(0, "ub[0]");
			mesh_program->bind_uniform_location(TEXTURE_REG_0, "uSampler");
			mesh_program->bind_uniform_location(TEXTURE_REG_1, "uNormalSampler");
			regist_program(MESH_PROGRAM_NAME, mesh_program);
		}

		{
			auto [res, fog_program] = program_builder::build(
				R"(
					in vec4 aPosUV;

					out vec2 vTexCoord;

					void main()
					{
						gl_Position = vec4(aPosUV.xy, 0.0, 1.0);
						vTexCoord = aPosUV.zw;
					}
				)",
				R"(
				precision highp float;
				const vec3 COLOR = vec3(0.25, 0.25, 0.25);
				const vec3 BG = vec3(0.0, 0.0, 0.0);
				const float ZOOM = 3.;
				const int OCTAVES = 4;
				const float INTENSITY = 2.;

				float random (vec2 st) {
					return fract(sin(dot(st.xy, vec2(12.9818,79.279)))*43758.5453123);
				}

				vec2 random2(vec2 st){
					st = vec2( dot(st,vec2(127.1,311.7)), dot(st,vec2(269.5,183.3)) );
					return -1.0 + 2.0 * fract(sin(st) * 7.);
				}

				float noise(vec2 st) {
					vec2 i = floor(st);
					vec2 f = fract(st);

					// smootstep
					vec2 u = f*f*(3.0-2.0*f);

					return mix( mix( dot( random2(i + vec2(0.0,0.0) ), f - vec2(0.0,0.0) ),
									 dot( random2(i + vec2(1.0,0.0) ), f - vec2(1.0,0.0) ), u.x),
								mix( dot( random2(i + vec2(0.0,1.0) ), f - vec2(0.0,1.0) ),
									 dot( random2(i + vec2(1.0,1.0) ), f - vec2(1.0,1.0) ), u.x), u.y);
				}


				float fractal_brownian_motion(vec2 coord) {
					float value = 0.0;
					float scale = 0.2;
					for (int i = 0; i < 4; i++) {
						value += noise(coord) * scale;
						coord *= 2.0;
						scale *= 0.5;
					}
					return value + 0.2;
				}

				uniform mat4 ub[2];
				uniform vec4 uResolutionTimeRev;

                in vec2 vTexCoord;
				out vec4 OutColor;

                void main()
                {
					mat4 uProjectionMatrix = ub[0];
					mat4 uViewMatrix = ub[1];
					float iTime = uResolutionTimeRev.z;
					vec4 offset = uProjectionMatrix * uViewMatrix * vec4(0.0, 0.0, 0.0, 1);
					offset.x *= -1.0;
					vec2 st = vTexCoord + offset.xy * 0.125;
					st *= uResolutionTimeRev.xy  / uResolutionTimeRev.y;    
					vec2 pos = vec2(st * ZOOM);
					vec2 motion = vec2(fractal_brownian_motion(pos + vec2(iTime * -0.5, iTime * -0.3)));
					float final = fractal_brownian_motion(pos + motion) * INTENSITY;
					OutColor = vec4(mix(BG, COLOR, final), 1.0);
                }
			)"
			);
			IMRRESULT(res);
			fog_program->bind_attrib_location(0, "aPosUV");
			fog_program->bind_uniform_location(0, "ub[0]");
			fog_program->bind_uniform_location(1, "uResolutionTimeRev");
			regist_program(FOG_PROGRAM_NAME, fog_program);
		}

		{
			auto [res, deffered_program] = program_builder::build(
				R"(
					in vec4 aPosUV;
					in vec4 aColor;

					uniform mat4 ub[2];

					out vec2 vTexCoord;
					out vec4 vColor;

					void main()
					{
						mat4 uProjectionMatrix = ub[0];
						mat4 uViewMatrix = ub[1];
						gl_Position = uProjectionMatrix * uViewMatrix * vec4(aPosUV.xy, 0.1f, 1);
						vTexCoord = aPosUV.zw;
						vColor = aColor;
					}
			)",
				R"(
				//precision highp float;

                uniform sampler2D uAlbedoSampler;
                uniform sampler2D uLightColorSampler;
                uniform sampler2D uFogSampler;

                in vec2 vTexCoord;
                in vec4 vColor;
				in float vScaleX;

				out vec4 OutColor;

                void main()
                {
                    vec4 alb = texture(uAlbedoSampler, vTexCoord);
                    vec4 lit_col = texture(uLightColorSampler, vTexCoord);
                    vec4 fog_col = texture(uFogSampler, vTexCoord);
					vec3 col = alb.xyz * lit_col.xyz;
					
					OutColor = mix(fog_col, vec4(col, 1.0), lit_col.a) * vColor;
                }
			)"
			);
			IMRRESULT(res);
			deffered_program->bind_attrib_location(0, "aPosUV");
			deffered_program->bind_attrib_location(1, "aColor");
			deffered_program->bind_uniform_location(0, "ub[0]");
			deffered_program->bind_uniform_location(TEXTURE_REG_0, "uAlbedoSampler");
			deffered_program->bind_uniform_location(TEXTURE_REG_1, "uLightColorSampler");
			deffered_program->bind_uniform_location(TEXTURE_REG_2, "uFogSampler");
			regist_program(DEFFERED_PROGRAM_NAME, deffered_program);
		}

		{
			float vertices[] = {
				0, 0, 0,
				0, 1, 1,
				1, 1, 2,
				1, 0, 3
			};
			unsigned short indices[] = {
				0, 1, 2, 2, 3, 0
			};
			auto quad_vbo = create_array_buffer(vertices, sizeof(vertices), GL_ARRAY_BUFFER, GL_STATIC_DRAW);
			auto indices_vbo = create_array_buffer(indices, sizeof(indices), GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW);
			glGenVertexArrays(1, &quad_vao);
			glBindVertexArray(quad_vao);
			{
				quad_vbo->bind();
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
				glEnableVertexAttribArray(0);
				indices_vbo->bind();
			}
			glBindVertexArray(0);

			glGenVertexArrays(1, &instancing_attrib_vao);
			glBindVertexArray(instancing_attrib_vao);
			{
				quad_vbo->bind();
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
				indices_vbo->bind();
			}
			glBindVertexArray(0);

			glGenVertexArrays(1, &temp_vao);
		}

		{
			auto white_texture_info = std::make_shared<texture_info>();
			this->white_texture_info = white_texture_info;
			glGenTextures(1, &white_texture_info->resource);
			glBindTexture(GL_TEXTURE_2D, white_texture_info->resource);
			unsigned int data[1] = { 0xFFFFFFFF };
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			glBindTexture(GL_TEXTURE_2D, 0);
			white_texture_info->_width = 1;
			white_texture_info->_height = 1;
			white_texture_info->texture_name = "__white_texture__";
		}

		return {};
	}

	void context::destroy()
	{
		{
			for (auto& p : programs)
			{
				if (p.second)
				{
					p.second->destroy();
				}
			}
			programs.clear();
		}

		{
			glDeleteVertexArrays(1, &quad_vao);
			glDeleteVertexArrays(1, &instancing_attrib_vao);
			glDeleteVertexArrays(1, &temp_vao);
			quad_vao = 0;
			instancing_attrib_vao = 0;
			temp_vao = 0;
		}

		{
			array_buffer_pool.clear();
		}
	}

	void texture_info::bind() const
	{
		glBindTexture(GL_TEXTURE_2D, resource);
	}

	void texture_info::unbind() const
	{
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void texture_info::destroy()
	{
		glDeleteTextures(1, &resource);
		resource = 0;
		_width = 0;
		_height = 0;
	}

	result regist_program(const std::string& name, std::shared_ptr<Iprogram> program)
	{
		if (CTX->programs.find(name) != CTX->programs.end())
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "already exist" };
		}
		CTX->programs[name] = program;
		return {};
	}

	result unregist_program(const std::string& name)
	{
		if (CTX->programs.find(name) == CTX->programs.end())
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "not registered program" };
		}
		CTX->programs[name]->destroy();
		CTX->programs.erase(name);
		return {};
	}

	void push_program(const char* program_name)
	{
		CTX->program_stack.push(program_name);
	}

	void pop_program()
	{
		CTX->program_stack.pop();
	}

	template<class T>
	void push_state(std::stack<T>& container, const T& state, std::function<bool(const T& old, const T& cur)> same = {}, std::function<void(const T& cur)> set = {})
	{
		if (container.empty() == false)
		{
			auto& old = container.top();
			if (same && !same(old, state) && set)
			{
				set(state);
			}
		}
		else if (set)
		{
			set(state);
		}
		container.push(state);
	}

	template<class T>
	void pop_state(std::stack<T>& container, std::function<bool(const T& old, const T& cur)> same = {}, std::function<void(const T& cur)> set = {})
	{
		auto& old = container.top();
		container.pop();
		if (container.empty() == false)
		{
			auto& cur = container.top();
			if (same && !same(old, cur) && set)
			{
				set(cur);
			}
		}
	}

	void push_viewport(const viewport_state& state)
	{
		push_state<viewport_state>(CTX->viewport_stack, state,
			[](const viewport_state& old, const viewport_state& cur)
			{
				return old.x == cur.x && old.y == cur.y && old.width == cur.width && old.height == cur.height;
			},
			[](const viewport_state& state)
			{
				glViewport(state.x, state.y, state.width, state.height);
			});
	}

	void pop_viewport()
	{
		pop_state<viewport_state>(CTX->viewport_stack,
			[](const viewport_state& old, const viewport_state& cur)
			{
				return old.x == cur.x && old.y == cur.y && old.width == cur.width && old.height == cur.height;
			},
			[](const viewport_state& state)
			{
				glViewport(state.x, state.y, state.width, state.height);
			});
	}

	void push_blend_func(const blend_func_state& state)
	{
		push_state<blend_func_state>(CTX->blend_func_stack, state,
			[](const blend_func_state& old, const blend_func_state& cur)
			{
				return old.src == cur.src && old.dst == cur.dst;
			},
			[](const blend_func_state& state)
			{
				glBlendFunc(state.src, state.dst);
			});
	}

	void pop_blend_func()
	{
		pop_state<blend_func_state>(CTX->blend_func_stack,
			[](const blend_func_state& old, const blend_func_state& cur)
			{
				return old.src == cur.src && old.dst == cur.dst;
			},
			[](const blend_func_state& state)
			{
				glBlendFunc(state.src, state.dst);
			});
	}

	result on_resolution_changed()
	{
		return {};
	}

	std::tuple<result, std::shared_ptr<Itexture_info>> load_texture(const unsigned int* data, int w, int h)
	{
		auto ret = std::make_shared<texture_info>();
		glGenTextures(1, &ret->resource);
		glBindTexture(GL_TEXTURE_2D, ret->resource);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glBindTexture(GL_TEXTURE_2D, 0);
		ret->_width = w;
		ret->_height = h;
		return { {}, ret };
	}

	std::tuple<result, std::shared_ptr<Itexture_info>> load_texture(const std::string& path)
	{
		const auto binary = load_data(path);
		if (binary.empty())
		{
			return { {.type = result_type::fail, .error_code = 1, .msg = "invalid path" }, nullptr };
		}
		int nr_channels = {};
		std::shared_ptr<texture_info> info = std::make_shared<texture_info>();
		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load_from_memory((const unsigned char*)binary.data(), static_cast<int>(binary.size()), &info->_width, &info->_height, &nr_channels, 0);
		if (data)
		{
			glGenTextures(1, &info->resource);
			glBindTexture(GL_TEXTURE_2D, info->resource);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			int channel = {};
			if (nr_channels == 4)
			{
				channel = GL_RGBA;
			}
			else if (nr_channels == 3)
			{
				channel = GL_RGB;
			}
			glTexImage2D(GL_TEXTURE_2D, 0, channel, info->_width, info->_height, 0, channel, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);
			stbi_image_free(data);
		}
		else
		{
			return { {.type = result_type::fail, .error_code = 2, .msg = "fail to load texture" }, nullptr };
		}
		return { {}, info };
	}
}

namespace imr::camera
{
	result begin(const begin_args& args)
	{
		assert(args.frame_buffer);
		result ret;
		auto& state = CTX->camera_stack.emplace();
		state.begin = true;
		state.frame = args.frame_buffer;
		state.frame->bind();

		switch (args.origin)
		{
		case center:
		{
			float hw = state.frame->width() / 2.0f;
			float hh = state.frame->height() / 2.0f;
			state.projection = glm::orthoLH<float>(-hw, hw, hh, -hh, 0.01f, 1.0f);
			break;
		}
		case left_top:
			state.projection = glm::orthoLH<float>(0, (float)state.frame->width(), (float)state.frame->height(), 0, 0.01f, 1.0f);
			break;
		case left_bottom:
			state.projection = glm::orthoLH<float>(0, (float)state.frame->width(), 0, (float)-state.frame->height(), 0.01f, 1.0f);
			break;
		case left_center:
			float hh = state.frame->height() / 2.0f;
			state.projection = glm::orthoLH<float>(0, (float)state.frame->width(), hh, -hh, 0.01f, 1.0f);
			break;
		}

		push_viewport({ .x = 0, .y = 0, .width = state.frame->width(), .height = state.frame->height() });

		glEnable(GL_BLEND);
		push_blend_func({ .src = GL_SRC_ALPHA, .dst = GL_ONE_MINUS_SRC_ALPHA });
		camera({});
		return ret;
	}

	result end()
	{
		result ret;

		if (CTX->camera_stack.empty() || CTX->camera_stack.top().begin == false)
		{
			ret.type = fail;
			ret.error_code = 1;
			ret.msg = "imr::camera::begin not called";
			return ret;
		}

		auto& state = CTX->camera_stack.top();
		CTX->camera_stack.pop();

		pop_blend_func();
		pop_viewport();
		if (state.frame)
		{
			state.frame->unbind();
		}

		return ret;
	}

	void enable_depth_test()
	{
		glEnable(GL_DEPTH_TEST);
	}

	void disable_depth_test()
	{
		glDisable(GL_DEPTH_TEST);
	}

	void clear(const float4& color)
	{
		glClearColor(color.x, color.y, color.z, color.w);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void screen_to_world_impl(const float2& scr_pos, float2& out)
	{
		auto& state = CTX->camera_stack.top();
		auto iv = glm::inverse(state.projection * state.view);
		auto size = float2(static_cast<float>(state.frame->width()), static_cast<float>(state.frame->height()));
		auto hsize = size / 2.0f;
		auto p = 2.0f * (scr_pos - hsize) / size;
		auto wpos = iv * glm::vec4{ p.x, p.y, 0, 1 };
		out.x = wpos.x;
		out.y = wpos.y;
	}

	result camera(const camera_args& args)
	{
		result ret;

		if (CTX->camera_stack.empty() || CTX->camera_stack.top().begin == false)
		{
			ret.type = fail;
			ret.error_code = 1;
			ret.msg = "begin camera not called";
			return ret;
		}

		auto& state = CTX->camera_stack.top();
		state.view = glm::scale(state.view, { args.scale.x, args.scale.y, 1.0f });
		state.view = glm::rotate(state.view, args.rotation, { 0.0f, 0.0f, 1.0f });
		state.view = glm::translate(state.view, { std::roundf(args.position.x), std::roundf(args.position.y), 0.0f });
		state.view = glm::inverse(state.view);
		screen_to_world_impl({ 0, static_cast<float>(state.frame->height()) }, state.world_rect.xy);
		screen_to_world_impl({ static_cast<float>(state.frame->width()), 0 }, state.world_rect.zw);
		return ret;
	}

	std::tuple<result, float2> screen_to_world(const float2& scr_pos)
	{
		if (CTX->camera_stack.empty() || CTX->camera_stack.top().begin == false)
		{
			return { {.type = fail, .error_code = 1, .msg = "no camera stack"}, {} };
		}

		float2 ret = {};
		screen_to_world_impl(scr_pos, ret);
		return { {}, ret };
	}
}

namespace imr::sprite
{
	result begin_try_batch()
	{
		if (CTX->camera_stack.empty())
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "no camera contex" };
		}
		auto& state = CTX->camera_stack.top();
		if (state.batches.size() > 0)
		{
			return { .type = result_type::fail, .error_code = 2, .msg = "invaid batched data exist" };
		}
		state.try_batch = true;
		return {};
	}

	result flush_batching();

	result draw(const draw_args& args)
	{
		if (CTX->camera_stack.empty())
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "no camera contex" };
		}
		auto& state = CTX->camera_stack.top();

		// try batch
		if (state.try_batch)
		{
			auto is_same_material = [](const draw_args& lh, const draw_args& rh)
			{
				return lh.texture_info == rh.texture_info;
			};

			if (state.batches.size() > 0 && is_same_material(args, state.batches.back()) == false)
			{
				IMRRESULT(flush_batching());
			}
			state.batches.push_back(args);
			return {};
		}

		auto sprite_program = std::dynamic_pointer_cast<program>(CTX->programs[SPRITE_PROGRAM_NAME]);
		sprite_program->use();

		const Itexture_info* texture = args.texture_info ? args.texture_info : CTX->white_texture_info.get();
		float4 uv_rect = { 0, 1, 1, 0 };
		float2 offset = args.offset;
		int2 size = {};

		size = texture->size();
		if (args.sprite_info)
		{
			uv_rect = args.sprite_info->uv_rect;
			offset = offset + args.sprite_info->offset;
			size = args.sprite_info->size;
		}

		{
			glActiveTexture(GL_TEXTURE0);
			texture->bind();
			bind_multi_textures(sprite_program.get(), args.texture_info_1, args.texture_info_2, args.texture_info_3);
		}

		{
			union uniform_buffer
			{
				float4 value[13] = {};
				struct
				{
					glm::mat4 uProjectionMatrix;
					glm::mat4 uViewMatrix;
					float4 uTranslateScale;
					float4 uSizeOffset;
					float4 uColor;
					float4 uUVRect;
					float4 uRotationRev;
				};
			} buf{};
			buf.uProjectionMatrix = state.projection;
			buf.uViewMatrix = state.view;
			buf.uTranslateScale.xy = args.position;
			buf.uTranslateScale.zw = args.scale;
			buf.uSizeOffset.xy = size;
			buf.uSizeOffset.zw = offset;
			buf.uColor = args.color;
			buf.uUVRect = uv_rect;
			buf.uRotationRev.x = args.rotation * glm::pi<float>() / 180.0f;

			glUniform4fv(sprite_program->get_uniform_location(0), sizeof(uniform_buffer) / sizeof(float4), (const GLfloat*)buf.value);
		}
		GL_ASSERT();
		glBindVertexArray(CTX->quad_vao);
		GL_ASSERT();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
		glBindVertexArray(0);

		return {};
	}

	result flush_batching()
	{
		auto& state = CTX->camera_stack.top();
		if (state.batches.size() == 0)
		{
			return {};
		}
		{
			if (state.batches.size() == 1)
			{
				state.try_batch = false;
				imr::sprite::draw(state.batches.front());
				state.try_batch = true;
			}
			else
			{
				IMRRESULT(imr::instancing::begin({ .texture_info = state.batches.front().texture_info }));
				for (auto& args : state.batches)
				{
					imr::instancing::instance({
						.sprite_info = args.sprite_info,
						.position = args.position,
						.scale = args.scale,
						.rotation = args.rotation,
						.color = args.color,
						.offset = {}
					});
				}
				imr::instancing::end();
			}
		}
		state.batches.clear();
		return {};
	}

	result end_try_batch()
	{
		if (CTX->camera_stack.empty() || CTX->camera_stack.top().try_batch == false)
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "no camera contex or didn't try batch" };
		}
		auto& state = CTX->camera_stack.top();
		IMRRESULT(flush_batching());
		state.try_batch = false;
		return {};
	}

	result draw_single(Itexture_info* tex_info, const float2& position, const float2& scale, float rotation, const float4& color, const float2& offset)
	{
		if (CTX->camera_stack.empty())
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "no camera contex" };
		}
		auto& state = CTX->camera_stack.top();

		auto sprite_program = std::dynamic_pointer_cast<program>(CTX->programs[SPRITE_PROGRAM_NAME]);
		sprite_program->use();

		glActiveTexture(GL_TEXTURE0);
		tex_info->bind();

		float4 uv_rect = { 0, 1, 1, 0 };
		{
			union uniform_buffer
			{
				float4 value[13] = {};
				struct
				{
					glm::mat4 uProjectionMatrix;
					glm::mat4 uViewMatrix;
					float4 uTranslateScale;
					float4 uSizeOffset;
					float4 uColor;
					float4 uUVRect;
					float4 uRotationRev;
				};
			} buf{};
			buf.uProjectionMatrix = state.projection;
			buf.uViewMatrix = state.view;
			buf.uTranslateScale.xy = position;
			buf.uTranslateScale.zw = scale;
			buf.uSizeOffset.xy = tex_info->size();
			buf.uSizeOffset.zw = offset;
			buf.uColor = color;
			buf.uUVRect = uv_rect;
			buf.uRotationRev.x = rotation * glm::pi<float>() / 180.0f;

			glUniform4fv(sprite_program->get_uniform_location(0), sizeof(uniform_buffer) / sizeof(float4), (const GLfloat*)buf.value);
		}
		GL_ASSERT();
		glBindVertexArray(CTX->quad_vao);
		GL_ASSERT();
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)0);
		glBindVertexArray(0);

		return {};
	}
}

namespace imr::instancing
{
	result begin(const begin_args& args)
	{
		auto& state = CTX->instancing_stack.emplace();
		state.begin = true;
		state.texture_info = args.texture_info ? args.texture_info : CTX->white_texture_info.get();
		state.texture_info_1 = args.texture_info_1;
		state.texture_info_2 = args.texture_info_2;
		state.texture_info_3 = args.texture_info_3;

		return {};
	}
	result instance(const instance_args& args)
	{
		if (CTX->instancing_stack.empty() || CTX->instancing_stack.top().begin == false)
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "instancing not begun" };
		}
		auto& state = CTX->instancing_stack.top();
		assert(state.texture_info);

		int2 sprite_size = state.texture_info->size();
		float4 uv_rect = { 0, 1, 1, 0 };
		float2 offset = args.offset;

		if (args.sprite_info)
		{
			auto* sprite = args.sprite_info;
			sprite_size = sprite->size;
			uv_rect = sprite->uv_rect;
			offset = offset + sprite->offset;
		}

		{
			float* instance_data = &instancing_state::SHARE_BUFFER[state.instance_count * instancing_state::INSTANCE_FORMAT_COUNT];
			int idx = 0;
			// translate, scale
			instance_data[idx++] = args.position.x;
			instance_data[idx++] = args.position.y;
			instance_data[idx++] = args.scale.x;
			instance_data[idx++] = args.scale.y;
			// rotation, width, height
			instance_data[idx++] = args.rotation * glm::pi<float>() / 180.0f;
			instance_data[idx++] = static_cast<float>(sprite_size.x);
			instance_data[idx++] = static_cast<float>(sprite_size.y);
			instance_data[idx++] = 0;
			// uv rect
			instance_data[idx++] = uv_rect[0];
			instance_data[idx++] = uv_rect[1];
			instance_data[idx++] = uv_rect[2];
			instance_data[idx++] = uv_rect[3];
			// color
			instance_data[idx++] = args.color.x;
			instance_data[idx++] = args.color.y;
			instance_data[idx++] = args.color.z;
			instance_data[idx++] = args.color.w;
			// offset rev
			instance_data[idx++] = offset.x;
			instance_data[idx++] = offset.y;
			instance_data[idx++] = 0;
			instance_data[idx++] = 0;
		}
		state.instance_count++;
		return {};
	}

	result begin(const Itexture_info* texture, const Itexture_info* texture_1, const Itexture_info* texture_2, Itexture_info* texture_3)
	{
		auto& state = CTX->instancing_stack.emplace();
		state.begin = true;
		state.texture_info = texture ? texture : CTX->white_texture_info.get();
		state.texture_info_1 = texture_1;
		state.texture_info_2 = texture_2;
		state.texture_info_3 = texture_3;
		return {};
	}

	result instance(imr::sprite::animation::animation_state* anim_state, const float2& position, const float2& scale, const float rotation, const float4& color)
	{
		if (CTX->instancing_stack.empty() || CTX->instancing_stack.top().begin == false)
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "instancing not begun" };
		}
		auto& state = CTX->instancing_stack.top();
		auto* sprite_info = anim_state->current_sprite_info();
		if (sprite_info == nullptr)
		{
			return { .type = result_type::fail, .error_code = 2, .msg = "sprite info is null" };
		}

		{
			float* instance_data = &instancing_state::SHARE_BUFFER[state.instance_count * instancing_state::INSTANCE_FORMAT_COUNT];
			int idx = 0;
			// translate, scale
			instance_data[idx++] = position.x;
			instance_data[idx++] = position.y;
			instance_data[idx++] = scale.x;
			instance_data[idx++] = scale.y;
			// rotation, width, height
			instance_data[idx++] = rotation * glm::pi<float>() / 180.0f;
			instance_data[idx++] = static_cast<float>(sprite_info->size.x);
			instance_data[idx++] = static_cast<float>(sprite_info->size.y);
			instance_data[idx++] = 0;
			// uv rect
			instance_data[idx++] = sprite_info->uv_rect[0];
			instance_data[idx++] = sprite_info->uv_rect[1];
			instance_data[idx++] = sprite_info->uv_rect[2];
			instance_data[idx++] = sprite_info->uv_rect[3];
			// color
			instance_data[idx++] = color.x;
			instance_data[idx++] = color.y;
			instance_data[idx++] = color.z;
			instance_data[idx++] = color.w;
			// offset rev
			instance_data[idx++] = sprite_info->offset.x;
			instance_data[idx++] = sprite_info->offset.y;
			instance_data[idx++] = 0;
			instance_data[idx++] = 0;
		}
		state.instance_count++;
		return {};
	}
	result begin(Itexture_info* texture)
	{
		if (texture == nullptr)
		{
			return { .type = fail, .error_code = 1, .msg = "invalid frame buffer" };
		}
		auto& state = CTX->instancing_stack.emplace();
		state.begin = true;
		state.texture_info = texture;
		return {};
	}
	result instance(const float2& sprite_pos, const float2& sprite_size, const float2& offset, const float2& position, const float2& scale, const float rotation, const float4& color)
	{
		if (CTX->instancing_stack.empty() || CTX->instancing_stack.top().begin == false)
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "instancing not begun" };
		}
		auto& state = CTX->instancing_stack.top();

		float4 uv_rect = {};
		float2 frame_size = state.texture_info->size();
		uv_rect.xy = sprite_pos / frame_size;
		uv_rect.zw = (sprite_pos + sprite_size) / frame_size;
		uv_rect.y = 1.0f - uv_rect.y;
		uv_rect.w = 1.0f - uv_rect.w;
		{
			float* instance_data = &instancing_state::SHARE_BUFFER[state.instance_count * instancing_state::INSTANCE_FORMAT_COUNT];
			int idx = 0;
			// translate, scale
			instance_data[idx++] = position.x;
			instance_data[idx++] = position.y;
			instance_data[idx++] = scale.x;
			instance_data[idx++] = scale.y;
			// rotation, width, height
			instance_data[idx++] = rotation * glm::pi<float>() / 180.0f;
			instance_data[idx++] = static_cast<float>(sprite_size.x);
			instance_data[idx++] = static_cast<float>(sprite_size.y);
			instance_data[idx++] = 0;
			// uv rect
			instance_data[idx++] = uv_rect[0];
			instance_data[idx++] = uv_rect[1];
			instance_data[idx++] = uv_rect[2];
			instance_data[idx++] = uv_rect[3];
			// color
			instance_data[idx++] = color.x;
			instance_data[idx++] = color.y;
			instance_data[idx++] = color.z;
			instance_data[idx++] = color.w;
			// offset rev
			instance_data[idx++] = offset.x;
			instance_data[idx++] = offset.y;
			instance_data[idx++] = 0;
			instance_data[idx++] = 0;
		}
		state.instance_count++;
		return {};
	}
	result end()
	{
		if (CTX->instancing_stack.empty() || CTX->instancing_stack.top().begin == false)
		{
			return { .type = result_type::fail, .error_code = 1, .msg = "instancing not begun" };
		}
		auto state = std::move(CTX->instancing_stack.top());
		CTX->instancing_stack.pop();
		if (state.instance_count == 0)
		{
			return {};
		}

		program* current_program = {};
		if (CTX->program_stack.empty())
		{
			current_program = dynamic_cast<program*>(CTX->programs[INSTANCING_PROGRAM_NAME].get());
		}
		else
		{
			current_program = dynamic_cast<program*>(CTX->programs[CTX->program_stack.top()].get());
		}

		if (current_program == nullptr)
		{
			return { .type = result_type::fail, .error_code = 2, .msg = "invalid program" };
		}
		else
		{
			current_program->use();
		}

		// projection view matrix
		{
			auto& cam_state = CTX->camera_stack.top();
			glUniformMatrix4fv(
				current_program->get_uniform_location(0),
				1,
				GL_FALSE,
				glm::value_ptr(cam_state.projection)
			);
			glUniformMatrix4fv(
				current_program->get_uniform_location(1),
				1,
				GL_FALSE,
				glm::value_ptr(cam_state.view)
			);
		}

		// texture
		{
			glUniform1i(current_program->get_uniform_location(TEXTURE_REG_0), 0);
			assert(state.texture_info != nullptr);
			glActiveTexture(GL_TEXTURE0);
			state.texture_info->bind();
			bind_multi_textures(current_program, state.texture_info_1, state.texture_info_2, state.texture_info_3);
		}

		glBindVertexArray(CTX->instancing_attrib_vao);
		GL_ASSERT();

		// bind instance buffer
		auto instance_size = state.instance_count * instancing_state::INSTANCE_FORMAT_SIZE;
		auto instance_buffer = CTX->get_array_buffer(instance_size, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
		instance_buffer->bind();
		instance_buffer->sub_data(0, instance_size, instancing_state::SHARE_BUFFER.data());

#define VERTEX_ATRIB_POINTER(reg, offset) \
{ \
	auto loc = current_program->get_attrib_location(reg); \
	glEnableVertexAttribArray(loc); \
	glVertexAttribPointer(loc, 4, GL_FLOAT, false, instancing_state::INSTANCE_FORMAT_SIZE, (void*)(offset * sizeof(float))); \
	glVertexAttribDivisor(loc, 1); \
}

		VERTEX_ATRIB_POINTER(1, 0);
		VERTEX_ATRIB_POINTER(2, 4);
		VERTEX_ATRIB_POINTER(3, 8);
		VERTEX_ATRIB_POINTER(4, 12);
		VERTEX_ATRIB_POINTER(5, 16);
		GL_ASSERT();

		glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0, state.instance_count);
		GL_ASSERT();

		// restore
		instance_buffer->unbind();
		CTX->release_array_buffer(std::move(instance_buffer));
		glBindVertexArray(0);
		return {};
	}
}

namespace imr::blend
{
	result push_addictive()
	{
		imr::push_blend_func({ .src = GL_ONE, .dst = GL_ONE });
		return {};
	}

	result pop()
	{
		imr::pop_blend_func();
		return {};
	}
}

namespace imr::mesh
{
	result draw(const draw_args& args)
	{
		imr::mesh::begin();
		imr::mesh::use_program(imr::MESH_PROGRAM_NAME);
		int stride = sizeof(imr::mesh::vertex) / sizeof(float);
		imr::mesh::push_meshes((const float*)args.vertices.data(), stride, static_cast<int>(args.vertices.size()) * stride, args.indices.data(), static_cast<int>(args.indices.size()));
		imr::mesh::set_texture(0, args.texture_info);
		imr::mesh::vertex_attrib_pointer(0, 4, 8, 0);
		imr::mesh::vertex_attrib_pointer(1, 4, 8, 4);
		return imr::mesh::end();
	}

	result begin()
	{
		auto& state = CTX->mesh_stack.emplace();
		state.begin = true;
		return {};
	}
	result use_program(const std::string& name)
	{
		if (CTX->mesh_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "not mesh begun" };
		}
		auto& state = CTX->mesh_stack.top();
		if (CTX->programs.find(name) == CTX->programs.end())
		{
			return { .type = fail, .error_code = 2, .msg = "no such program" };
		}
		state.program = dynamic_cast<imr::program*>(CTX->programs[name].get());
		state.program->use();

		return {};
	}
	result set_use_projection_view_matrix(bool val)
	{
		if (CTX->mesh_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "not mesh begun" };
		}
		auto& state = CTX->mesh_stack.top();
		state.use_project_view_matrix = val;
		return {};
	}
	result push_meshes(const float* vertices, int v_stride, size_t v_cnt, const unsigned short* indices, size_t i_cnt)
	{
		if (CTX->mesh_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "not mesh begun" };
		}
		auto& state = CTX->mesh_stack.top();

		auto prev_verts_cnt = static_cast<unsigned short>(state.vertices.size() / v_stride);
		auto prev_indices_cnt = state.indices.size();
		state.vertices.insert(state.vertices.end(), vertices, vertices + v_cnt);
		state.indices.insert(state.indices.end(), indices, indices + i_cnt);
		if (prev_verts_cnt > 0)
		{
			for (auto i = prev_indices_cnt; i < state.indices.size(); ++i)
			{
				state.indices[i] += prev_verts_cnt;
			}
		}
		return {};
	}
	result set_uniform_mat4(int location, const float* data, int length)
	{
		if (CTX->mesh_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "not mesh begun" };
		}
		auto& state = CTX->mesh_stack.top();
		glUniformMatrix4fv(state.program->get_uniform_location(location), length, GL_FALSE, data);
		GL_ASSERT();
		return {};
	}
	result set_uniform_vec4(int location, const float* data, int length)
	{
		if (CTX->mesh_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "not mesh begun" };
		}
		auto& state = CTX->mesh_stack.top();
		glUniform4fv(state.program->get_uniform_location(location), length, data);
		GL_ASSERT();
		return {};
	}
	result set_texture(int location, Itexture_info* texture)
	{
		if (CTX->mesh_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "not mesh begun" };
		}
		auto& state = CTX->mesh_stack.top();
		auto& item = state.textures.emplace_back();
		item.first = location;
		item.second = texture;
		return {};
	}
	result vertex_attrib_pointer(int location, int count, int stride, int offset)
	{
		if (CTX->mesh_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "not mesh begun" };
		}
		auto& state = CTX->mesh_stack.top();
		auto& pt = state.vert_attrib_pointers.emplace_back();
		pt.location = location;
		pt.count = count;
		pt.stride = stride;
		pt.offset = offset;
		return {};
	}
	result end()
	{
		if (CTX->mesh_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "not mesh begun" };
		}
		auto& state = CTX->mesh_stack.top();
		auto& cam_state = CTX->camera_stack.top();

		GL_ASSERT();

		static const int tex_idx[6] = {
			GL_TEXTURE0,
			GL_TEXTURE1,
			GL_TEXTURE2,
			GL_TEXTURE3,
			GL_TEXTURE4,
			GL_TEXTURE5
		};

		for (auto& p : state.textures)
		{
			if (p.second)
			{
				glUniform1i(state.program->get_uniform_location(texture_regs[p.first]), p.first);
				glActiveTexture(tex_idx[p.first]);
				p.second->bind();
			}
			else
			{
				return { .type = fail, .error_code = 1, .msg = "no such texture or frame" };
			}
		}

		if (state.use_project_view_matrix)
		{
			union uniform_buffer
			{
				float4 value[8] = {};
				struct
				{
					glm::mat4 uProjectionMatrix;
					glm::mat4 uViewMatrix;
				};
			} buf{};
			buf.uProjectionMatrix = cam_state.projection;
			buf.uViewMatrix = cam_state.view;
			glUniformMatrix4fv(state.program->get_uniform_location(0), sizeof(uniform_buffer) / sizeof(glm::mat4), GL_FALSE, (const GLfloat*)buf.value);
		}

		GL_ASSERT();

		// vao 가 없으면 glVertexAttribPointer에서 에러 발생
		glBindVertexArray(CTX->temp_vao);

		const int vertices_size = (int)state.vertices.size() * sizeof(float);
		auto quad_vbo = CTX->get_array_buffer(vertices_size, GL_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
		quad_vbo->bind();
		quad_vbo->sub_data(0, vertices_size, state.vertices.data());
		for (auto& p : state.vert_attrib_pointers)
		{
			glVertexAttribPointer(state.program->get_attrib_location(p.location), p.count, GL_FLOAT, GL_FALSE, p.stride * sizeof(float), (void*)(p.offset * sizeof(float)));
			glEnableVertexAttribArray(state.program->get_attrib_location(p.location));
		}

		const int indices_size = (int)state.indices.size() * sizeof(unsigned short);
		auto indices_vbo = CTX->get_array_buffer(indices_size, GL_ELEMENT_ARRAY_BUFFER, GL_DYNAMIC_DRAW);
		indices_vbo->bind();
		indices_vbo->sub_data(0, indices_size, state.indices.data());

		glDrawElements(GL_TRIANGLES, (GLsizei)state.indices.size(), GL_UNSIGNED_SHORT, (void*)0);
		GL_ASSERT();

		CTX->release_array_buffer(std::move(quad_vbo));
		CTX->release_array_buffer(std::move(indices_vbo));

		state.vertices.clear();
		state.indices.clear();
		CTX->mesh_stack.pop();
		return {};
	}
}

namespace imr::primitive
{
	result begin()
	{
		return imr::instancing::begin(CTX->white_texture_info.get());
	}

	result line(const line_args& args)
	{
		float2 dir = args.to - args.from;
		float length = imr::length(dir);
		float rotation = std::atan2f(dir.y, dir.x) * 180.0f / glm::pi<float>();
		return imr::instancing::instance({
			.position = args.from,
			.scale = {length, args.thickness},
			.rotation = rotation,
			.color = args.color,
			.offset = {0, 0.5},
			});
	}

	result circle(const circle_args& args)
	{
		auto from = float2{ args.radius, 0 };
		const auto angle_to_rad = glm::pi<float>() / 180.0f;
		const int step_angle = 360 / args.segment;
		for (int i = 0; i < args.segment; ++i)
		{
			auto to = from.rotate(step_angle * angle_to_rad);
			line({
				.from = from + args.center,
				.to = to + args.center,
				.thickness = args.thickness,
				.color = args.color
			});
			from = to;
		}
		return {};
	}

	result end()
	{
		return imr::instancing::end();
	}
}