#pragma once

#include "imr_core.h"
#include "gameworld.h"
#include <chrono>

namespace imr::game
{
	class scene_manager;

	class Iscene
	{
	public:
		virtual ~Iscene() = default;
		virtual result begin() = 0;
		virtual result end() = 0;
		virtual result load_resource() = 0;
		virtual result unload_resource() = 0;
		virtual result update(float dt) = 0;
		virtual result render(const int2& resolution) = 0;
		virtual result ui(const int2& resolution) = 0;
		virtual const std::shared_ptr<imr::Iframe_buffer> get_frame_buffer() const = 0;
		scene_manager* scene_mgr = {};
	};

	class scene_manager
	{
	public:
		scene_manager(std::shared_ptr<Igame_context> ctx)
			:_game_context(ctx)
		{}
		~scene_manager();
		result on_device_reset();
		result on_device_lost();
		result on_update();
		result on_ui();
		result on_resolution_changed(const int2 resolution);
		const std::shared_ptr<imr::Iframe_buffer> get_frame_buffer() const { return _scene ? _scene->get_frame_buffer() : nullptr; }

	public:
		result change_scene(std::shared_ptr<Iscene> scene);

		template<class T>
		constexpr T* get_game_context() { return dynamic_cast<T*>(_game_context.get()); }

	private:
		bool is_device_valid = false;
		std::shared_ptr<Iscene> _scene = {};
		std::shared_ptr<Iscene> _prepared_scene = {};
		int2 _resolution = { 1024, 720 };
		std::chrono::steady_clock::time_point _prev = std::chrono::steady_clock::now();
		std::shared_ptr<Igame_context> _game_context = {};
	};
}
