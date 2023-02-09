#include "imr_opengl3.h"
#include <sstream>

namespace imr::text
{
	font_info::~font_info()
	{
		destroy();
	}

	result font_info::create(const char* path, unsigned int font_width, unsigned int font_height, unsigned int packer_width, unsigned int packer_height)
	{
		std::stringstream ss = {};
		ss << path;
		ss << '_';
		ss << font_width;
		ss << '_';
		ss << font_height;
		this->_frame_name = ss.str();

		if (FT_Init_FreeType(&this->_ft))
		{
			return {};
		}

		if (FT_New_Face(this->_ft, path, 0, &this->_face))
		{
			return {};
		}
		FT_Set_Pixel_Sizes(this->_face, font_width, font_height);

		this->_font_width = font_width;
		this->_font_height = font_height;
		this->_width = packer_width;
		this->_height = packer_height;
		this->_packer.Init(packer_width, packer_height, false);
		this->_frame = std::make_shared<frame_buffer>();
		this->_frame->create(this->_width, this->_height);
		this->_frame_texture_info = frame_buffer_texture_info(this->_frame);
		if (succeed(imr::camera::begin({ .frame_buffer = this->_frame.get() })))
		{
			imr::camera::clear({});
			imr::camera::end();
		}
		GL_ASSERT();
		return {};
	}

	const font_info::character& font_info::get_char_rect(unsigned long c)
	{
		if (_characters.contains(c))
		{
			return _characters[c];
		}

		if (FT_Load_Char(_face, c, FT_LOAD_RENDER))
		{
			static character invalid = {};
			return invalid;
		}

		const int margin = 1;
		rbp::MaxRectsBinPack::FreeRectChoiceHeuristic heuristic = rbp::MaxRectsBinPack::RectBestShortSideFit; // This can be changed individually even for each rectangle packed.
		auto rect = _packer.Insert(_face->glyph->bitmap.width + margin, _face->glyph->bitmap.rows + margin, heuristic);
		auto& ret = _characters[c];
		ret.tex_coords = { rect.x + margin, rect.y + margin, rect.width, rect.height };
		ret.bearing = { _face->glyph->bitmap_left, _face->glyph->bitmap_top };
		ret.advance = { static_cast<float>(_face->glyph->advance.x >> 6), static_cast<float>(_face->glyph->advance.y >> 6) };

		// draw to frame
		if (_face->glyph->bitmap.width > 0 && _face->glyph->bitmap.rows > 0)
		{
			GLint prev = {};
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &prev);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			unsigned int texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				_face->glyph->bitmap.width,
				_face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				_face->glyph->bitmap.buffer
			);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glPixelStorei(GL_UNPACK_ALIGNMENT, prev);
			GL_ASSERT();

			imr::texture_info tex = {};
			tex.resource = texture;
			tex._width = _face->glyph->bitmap.width;
			tex._height = _face->glyph->bitmap.rows;

			if (succeed(imr::camera::begin({ .frame_buffer = _frame.get(), .origin = imr::camera::left_top })))
			{
				imr::sprite::draw_single(&tex, to_float2(ret.tex_coords.xy), { 1, -1 }, 0, { 1, 1, 1, 1 }, { 0, 1 });
				imr::camera::end();
			}
			tex.destroy();
		}

		return ret;
	}

	void font_info::destroy()
	{
		FT_Done_Face(_face);
		FT_Done_FreeType(_ft);
	}

	result begin(font_info* font)
	{
		auto& state = CTX->text_stack.emplace();
		state.font_info = font;
		return {};
	}

	result text(const char8_t* txt, const float2& position, const float4& color)
	{
		return text(reinterpret_cast<const char*>(txt), position, color);
	}

	result text(std::string_view txt, const float2& position, const float4& color)
	{
		if (CTX->text_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "invalid stack" };
		}
		auto& state = CTX->text_stack.top();

		imr::push_program(TEXT_PROGRAM_NAME);
		if (succeed(imr::instancing::begin(state.font_info->get_frame_buffer_texture())))
		{
			float2 pos = position;
			for (size_t i = 0; i < txt.length();)
			{
				int char_size = 0;
				unsigned long c = {};
				if ((txt[i] & 0b11111000) == 0b11110000) {
					char_size = 4;
					auto t1 = txt[i];
					auto t2 = txt[i + 1];
					auto t3 = txt[i + 2];
					auto t4 = txt[i + 3];
					c = ((t1 & 0b00000111) << 18) + ((t2 & 0b00111111) << 12) + ((t3 & 0b00111111) << 6) + (t4 & 0b00111111);
				}
				else if ((txt[i] & 0b11110000) == 0b11100000) {
					char_size = 3;
					auto t1 = txt[i];
					auto t2 = txt[i + 1];
					auto t3 = txt[i + 2];
					c = ((t1 & 0b00001111) << 12) + ((t2 & 0b00111111) << 6) + (t3 & 0b00111111);
				}
				else if ((txt[i] & 0b11100000) == 0b11000000) {
					char_size = 2;
					auto t1 = txt[i];
					auto t2 = txt[i + 1];
					c = ((t1 & 0b00011111) << 6) + (t2 & 0b00111111);
				}
				else if ((txt[i] & 0b10000000) == 0b00000000) {
					char_size = 1;
					c = txt[i] & 0b01111111;
				}
				else {
					char_size = 1;
					c = txt[i] & 0b01111111;
				}
				i += char_size;
				if (c == u'\r')
				{
					pos.y = state.font_info->font_height();
					pos.x = position.x;
					continue;
				}
				auto& r = state.font_info->get_char_rect(c);
				auto offset = float2{ static_cast<float>(r.bearing.x), static_cast<float>(-r.bearing.y) };
				imr::instancing::instance(to_float2(r.tex_coords.xy), to_float2(r.tex_coords.zw), {}, pos + offset, { 1, 1 }, 0, color);
				pos = pos + r.advance;
			}
			imr::instancing::end();
		}
		imr::pop_program();
		return {};
	}

	result end()
	{
		if (CTX->text_stack.empty())
		{
			return { .type = fail, .error_code = 1, .msg = "invalid stack" };
		}
		CTX->text_stack.pop();
		return {};
	}
}