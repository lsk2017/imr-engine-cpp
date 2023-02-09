#include "imr_core.h"
#include "imr_scene.h"

namespace imr::game
{
	scene_manager::~scene_manager()
	{
		if (_scene)
		{
			_scene->end();
			_scene->unload_resource();
		}
	}

	result scene_manager::on_device_reset()
	{
		is_device_valid = true;
		if (_prepared_scene)
		{
			change_scene(_prepared_scene);
			_prepared_scene = nullptr;
		}
		else if (_scene)
		{
			_scene->load_resource();
		}
		return {};
	}

	result scene_manager::on_device_lost()
	{
		is_device_valid = false;
		if (_scene)
		{
			_scene->unload_resource();
		}
		return {};
	}

	result scene_manager::on_update()
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - _prev).count();
		_prev = now;
		if (is_device_valid == false)
		{
			return { .type = fail, .error_code = 1, .msg = "invalid device" };
		}
		if (_scene)
		{
			auto* prev_scene = _scene.get();
			_scene->update(ms / 1000.0f);
			if (prev_scene == _scene.get())
			{
				_scene->render(_resolution);
			}
		}
		return {};
	}

	result scene_manager::on_ui()
	{
		if (_scene)
		{
			_scene->ui(_resolution);
		}
		return {};
	}

	result scene_manager::on_resolution_changed(const int2 resolution)
	{
		_resolution = resolution;
		return {};
	}

	result scene_manager::change_scene(std::shared_ptr<Iscene> scene)
	{
		if (is_device_valid == false)
		{
			_prepared_scene = scene;
			return { .type = success, .error_code = 0, .msg = "scene cached." };
		}
		if (_scene)
		{
			_scene->end();
			_scene->unload_resource();
			_scene->scene_mgr = {};
		}
		_scene = scene;
		_scene->scene_mgr = this;
		_scene->load_resource();
		_scene->begin();
		_prev = std::chrono::steady_clock::now();
		on_update();
		return {};
	}
}