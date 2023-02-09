#pragma once

#include "imr_core.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include "MaxRectsBinPack.h"

namespace imr::text
{
	class font_info
	{
	public:
		struct character
		{
			int4 tex_coords = {};
			int2 bearing = {};
			float2 advance = {};
		};
		~font_info();
		result create(const char* path, unsigned int font_width, unsigned int font_height, unsigned int packer_width = 1024, unsigned int packer_height = 1024);
		const character& get_char_rect(unsigned long c);
		const std::string& get_frame_name() { return _frame_name; }
		const frame_buffer_texture_info* get_frame_buffer_texture() { return &_frame_texture_info; }
		inline unsigned int font_width() { return _font_width; }
		inline unsigned int font_height() { return _font_height; }
		void destroy();

	private:
		unsigned int _width = {};
		unsigned int _height = {};
		unsigned int _font_width = {};
		unsigned int _font_height = {};
		std::string _frame_name = {};
		std::shared_ptr<Iframe_buffer> _frame = {};
		frame_buffer_texture_info _frame_texture_info{ nullptr };
		FT_Library _ft = {};
		FT_Face _face = {};
		std::unordered_map<unsigned long, character> _characters = {};
		rbp::MaxRectsBinPack _packer = {};
	};

	result begin(font_info* font);
	result text(const char8_t* txt, const float2& position, const float4& color = { 1, 1, 1, 1 });
	result text(std::string_view txt, const float2& position, const float4& color = { 1, 1, 1, 1 });
	result end();
}
