#pragma once

#include <functional>
#include <unordered_map>
#include <optional>
#include <cmath>

namespace tweeners
{
	inline static float __PI__ = 3.1415926545f;

	// https://github.com/nicolausYes/easing-functions/blob/master/src/easing.cpp
	using EASING_FUNC = std::function<float(float)>;
	inline std::unordered_map<std::string, EASING_FUNC> easing_handlers = {
		{ "linear", [](float t) { return t; } },
		{ "easeInSine", [](float t) { return std::sin(1.5707963f * t); } },
		{ "easeOutSine", [](float t) { return 1 + std::sin(1.5707963f * (--t)); } },
		{ "easeInOutSine", [](float t) { return 0.5f * (1 + std::sin(3.1415926f * (t - 0.5f))); } },
		{ "easeInQuad", [](float t) { return t * t; } },
		{ "easeOutQuad", [](float t) { return t * (2 - t); } },
		{ "easeInOutQuad", [](float t) { return t < 0.5f ? 2 * t * t : t * (4 - 2 * t) - 1; } },
		{ "easeInCubic", [](float t) { return t * t * t; } },
		{ "easeOutCubic", [](float t) { return 1 + (--t) * t * t; } },
		{ "easeInOutCubic", [](float t) { return t < 0.5f ? 4 * t * t * t : 1 + (--t) * (2 * (--t)) * (2 * t); } },
		{ "easeInQuart", [](float t) { t *= t; return t * t; } },
		{ "easeOutQuart", [](float t) { t = (--t) * t; return 1 - t * t; } },
		{ "easeInOutQuart", [](float t) { if (t < 0.5f) { t *= t; return 8 * t * t; } else { t = (--t) * t; return 1 - 8 * t * t; }} },
		{ "easeInQuint", [](float t) { float t2 = t * t; return t * t2 * t2; } },
		{ "easeOutQuint", [](float t) { float t2 = (--t) * t; return 1 + t * t2 * t2; } },
		{ "easeInOutQuint", [](float t) { float t2; if (t < 0.5f) { t2 = t * t; return 16 * t * t2 * t2; } else { t2 = (--t) * t; return 1 + 16 * t * t2 * t2; }} },
		{ "easeInExpo", [](float t) { return (std::powf(2, 8 * t) - 1) / 255; } },
		{ "easeOutExpo", [](float t) { return 1 - std::powf(2, -8 * t); } },
		{ "easeInOutExpo", [](float t) { if (t < 0.5f) { return (std::powf(2, 16 * t) - 1) / 510; } else { return 1 - 0.5f * std::powf(2, -16 * (t - 0.5f)); }} },
		{ "easeInCirc", [](float t) { return 1 - sqrt(1 - t); } },
		{ "easeOutCirc", [](float t) { return sqrt(t); } },
		{ "easeInOutCirc", [](float t) { if (t < 0.5f) { return (1 - sqrt(1 - 2 * t)) * 0.5f; } else { return (1 + sqrt(2 * t - 1)) * 0.5f; }} },
		{ "easeInBack", [](float t) { return t * t * (2.70158f * t - 1.70158f); } },
		{ "easeOutBack", [](float t) { return 1 + (--t) * t * (2.70158f * t + 1.70158f); } },
		{ "easeInOutBack", [](float t) { if (t < 0.5f) { return t * t * (7 * t - 2.5f) * 2; } else { return 1 + (--t) * t * 2 * (7 * t + 2.5f); }} },
		{ "easeInElastic", [](float t) { float t2 = t * t; return t2 * t2 * std::sin(t * __PI__ * 4.5f); } },
		{ "easeOutElastic", [](float t) { float t2 = (t - 1) * (t - 1); return 1 - t2 * t2 * cos(t * __PI__ * 4.5f); } },
		{ "easeInOutElastic", [](float t) { float t2; if (t < 0.45f) { t2 = t * t; return 8 * t2 * t2 * std::sin(t * __PI__ * 9); } else if (t < 0.55f) { return 0.5f + 0.75f * std::sin(t * __PI__ * 4); } else { t2 = (t - 1) * (t - 1); return 1 - 8 * t2 * t2 * std::sin(t * __PI__ * 9); }} },
		{ "easeInBounce", [](float t) { return std::powf(2, 6 * (t - 1)) * abs(std::sin(t * __PI__ * 3.5f)); } },
		{ "easeOutBounce", [](float t) { return 1 - std::powf(2, -6 * t) * abs(cos(t * __PI__ * 3.5f)); } },
		{ "easeInOutBounce", [](float t) { if (t < 0.5f) { return 8 * std::powf(2, 8 * (t - 1)) * abs(std::sin(t * __PI__ * 7)); } else { return 1 - 8 * std::powf(2, -8 * t) * abs(std::sin(t * __PI__ * 7)); }} },
	};

	template<class T>
	using progress_callback_type = std::function<bool(float t, const T& v)>;
	using complete_callback_type = std::function<void()>;

	template<class T>
	struct tween_item
	{
		std::string key = {};
		T from = {};
		T to = {};
		float duration = 1.0f;
		float delay = 0.0f;
		float elapsed = 0.0f;
		EASING_FUNC easing_func = {};
		progress_callback_type<T> progress_callback = {};
		complete_callback_type complete_callback = {};
	};

	enum PLAY_MODE
	{
		ONCE,
		LOOP,
		PINGPONG
	};

	enum PLAY_DIRECTION
	{
		FORWARD,
		BACKWARD
	};

	class Itweener
	{
	public:
		virtual ~Itweener() = default;
		virtual bool update(float dt) = 0;
		virtual bool finish() = 0;
		virtual void rewind() = 0;
		virtual void restart() = 0;
	};

	template<class T>
	class tweener : public Itweener
	{
	public:
		bool pause = false;

		void initialize(
			std::vector<tween_item<T>>&& items,
			PLAY_MODE mode = ONCE,
			int loop_count = 0,
			progress_callback_type<T> progress_callback = {},
			complete_callback_type complete_callback = {})
		{
			_items = std::move(items);
			_play_mode = mode;
			_loop_total = loop_count;
			_shared_progress_callback = progress_callback;
			_complete_callback = complete_callback;
			_current_idx = 0;
			_loop_count = 0;
		}

		bool update(float dt) override
		{
			if (pause)
			{
				return true;
			}
			if (_current_idx < 0 || _current_idx >= _items.size())
			{
				return true;
			}
			switch (_play_mode)
			{
			case ONCE:
				return once_update(dt);
			case LOOP:
				return loop_update(dt);
			case PINGPONG:
				return pingpong_update(dt);
			default:
				return false;
			}
		}

		void rewind() override
		{
			for (auto& p : _items)
			{
				p.elapsed = 0;
			}
			_current_idx = 0;
			_direction = FORWARD;
		}

		void restart() override
		{
			rewind();
			_loop_count = 0;
		}

		bool finish() override
		{
			return _play_mode == ONCE && has_next() == false || loop_finish();
		}

	private:
		std::vector<tween_item<T>> _items = {};
		PLAY_MODE _play_mode = { ONCE };
		progress_callback_type<T> _shared_progress_callback = {};
		complete_callback_type _complete_callback = {};
		int _current_idx = {};
		int _loop_total = {};
		int _loop_count = {};
		PLAY_DIRECTION _direction = FORWARD;

		bool has_next() { return _current_idx < static_cast<int>(_items.size()); }
		bool has_prev() { return _current_idx > 0; }
		bool loop_finish() { return _loop_total > 0 && _loop_count >= _loop_total; }
		void call_complete_callback() { if (_complete_callback) _complete_callback(); }

		bool once_update(float dt)
		{
			auto& current = _items[_current_idx];
			auto progress_succeed = false;
			current.elapsed += dt;
			auto complete = current.elapsed >= current.duration + current.delay;
			current.elapsed = std::min(current.duration + current.delay, current.elapsed);
			if (current.elapsed >= current.delay)
			{
				float progress = (current.elapsed - current.delay) / current.duration;
				float t = current.easing_func != nullptr ? current.easing_func(progress) : progress;
				if (current.progress_callback)
				{
					progress_succeed |= current.progress_callback(t, current.from * (1 - t) + current.to * t);
				}
				if (_shared_progress_callback)
				{
					progress_succeed |= _shared_progress_callback(t, current.from * (1 - t) + current.to * t);
				}
			}
			if (complete)
			{
				if (current.complete_callback)
				{
					current.complete_callback();
				}
				_current_idx++;
				call_complete_callback();
			}
			return progress_succeed;
		}

		bool loop_update(float dt)
		{
			auto current = _items[_current_idx];
			auto progress_succeed = false;
			current.elapsed += dt;
			auto complete = current.elapsed >= current.duration + current.delay;
			current.elapsed = std::min(current.duration + current.delay, current.elapsed);

			if (current.elapsed >= current.delay)
			{
				float progress = (current.elapsed - current.delay) / current.duration;
				float t = current.easing_func != nullptr ? current.easing_func(progress) : progress;
				if (current.progress_callback)
				{
					progress_succeed |= current.progress_callback(t, current.from * (1 - t) + current.to * t);
				}
				if (_shared_progress_callback)
				{
					progress_succeed |= _shared_progress_callback(t, current.from * (1 - t) + current.to * t);
				}
			}
			if (complete)
			{
				if (current.complete_callback)
				{
					current.complete_callback();
				}
				_current_idx++;
				if (has_next() == false)
				{
					_loop_count++;
					if (loop_finish())
					{
						call_complete_callback();
					}
					else
					{
						rewind();
					}
				}
			}
			return progress_succeed;
		}

		bool pingpong_update(float dt)
		{
			auto& current = _items[_current_idx];
			auto progress_succeed = false;
			auto complete = false;
			if (_direction == PLAY_DIRECTION::FORWARD)
			{
				current.elapsed += dt;
				complete = current.elapsed >= current.duration + current.delay;
				current.elapsed = std::min(current.duration + current.delay, current.elapsed);
			}
			else
			{
				current.elapsed -= dt;
				complete = current.elapsed <= 0;
				current.elapsed = std::max(0.0f, current.elapsed);
			}
			if (current.elapsed >= current.delay)
			{
				float progress = (current.elapsed - current.delay) / current.duration;
				float t = current.easing_func != nullptr ? current.easing_func(progress) : progress;
				if (current.progress_callback)
				{
					progress_succeed |= current.progress_callback(t, current.from * (1 - t) + current.to * t);
				}
				if (_shared_progress_callback)
				{
					progress_succeed |= _shared_progress_callback(t, current.from * (1 - t) + current.to * t);
				}
			}
			if (complete)
			{
				if (current.complete_callback)
				{
					current.complete_callback();
				}

				if (_direction == PLAY_DIRECTION::FORWARD)
				{
					_current_idx++;
					if (has_next() == false)
					{
						_current_idx = static_cast<int>(_items.size()) - 1;
						_direction = PLAY_DIRECTION::BACKWARD;
					}
				}
				else
				{
					_current_idx--;
					if (has_prev() == false)
					{
						_current_idx = 0;
						_direction = PLAY_DIRECTION::FORWARD;
						_loop_count++;
						if (loop_finish())
						{
							call_complete_callback();
						}
					}
				}
			}
			return progress_succeed;
		}
	};

	template<class T>
	class tween_builder
	{
	public:
		tween_builder<T>& from_to(const T& from, const T& to, float duration, float delay, const std::string& ease_type, progress_callback_type<T> progress_callback = {}, complete_callback_type complete_callback = {})
		{
			EASING_FUNC handler = easing_handlers.find(ease_type) != easing_handlers.end() ? easing_handlers[ease_type] : nullptr;
			return from_to(from, to, duration, delay, handler, progress_callback, complete_callback);
		}

		tween_builder<T>& from_to(const T& from, const T& to, float duration, float delay, EASING_FUNC easing_func = {}, progress_callback_type<T> progress_callback = {}, complete_callback_type complete_callback = {})
		{
			auto& item = _items.emplace_back();
			item.from = from;
			item.to = to;
			item.duration = duration;
			item.delay = delay;
			item.easing_func = easing_func;
			item.progress_callback = progress_callback;
			item.complete_callback = complete_callback;
			return *this;
		}

		std::shared_ptr<Itweener> build(PLAY_MODE mode = ONCE, int loop_count = 0, progress_callback_type<T> shared_progress = {}, complete_callback_type complete_callback = {})
		{
			auto ret = std::make_shared<tweener<T>>();
			ret->initialize(std::move(_items), mode, loop_count, shared_progress, complete_callback);
			return std::move(ret);
		}

	private:
		std::vector<tween_item<T>> _items = {};
	};

	template<class T>
	inline tween_builder<T> builder()
	{
		tween_builder<T> ret = {};
		return ret;
	}
}
