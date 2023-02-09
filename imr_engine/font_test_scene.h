#pragma once

#include "imr_opengl3.h"
#include <iostream>

class font_test_scene : public imr::game::Iscene
{
private:
	std::shared_ptr<imr::frame_buffer> _frame_buffer = {};
	imr::text::font_info _font = {};

public:
	const std::shared_ptr<imr::Iframe_buffer> get_frame_buffer() const override { return _frame_buffer; }

	imr::result begin() override
	{
		return {};
	}
	imr::result end() override
	{
		return {};
	}
	imr::result load_resource() override
	{
		_frame_buffer = std::make_shared<imr::frame_buffer>();
		_frame_buffer->create(640, 360);

		_font.create("resources/font/neodgm.ttf", 0, 24);
		return {};
	}
	imr::result unload_resource() override
	{
		return {};
	}
	imr::result update(float dt) override
	{
		return {};
	}
	imr::result render(const imr::int2& resolution) override
	{
		if (succeed(imr::camera::begin({ .frame_buffer = _frame_buffer.get(), .origin = imr::camera::left_center })))
		{
			imr::camera::clear({ 0, 1, 0, 1 });

			if (succeed(imr::text::begin(&_font)))
			{
				imr::text::text(u8"이것은\rimr 엔진입니다ⓐ♡", {}, { 1, 0, 0, 1 });
				imr::text::text(u8"새로운 라인 텍스트.", { 0, 50 });
				imr::text::end();
			}

			if (succeed(imr::primitive::begin()))
			{
				imr::primitive::line({ .from = {0,0}, .to = {800, 0} });
				imr::primitive::end();
			}

			imr::camera::end();
		}
		return {};
	}
	imr::result ui(const imr::int2& resolution) override
	{
		return {};
	}

private:
};

