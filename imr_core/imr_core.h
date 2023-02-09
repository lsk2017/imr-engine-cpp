#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <typeindex>
#include <functional>
#include <vector>

#define IMRRESULT(R) \
{\
	auto __ret = R; \
	if (__ret.type == imr::result_type::fail) return __ret; \
}

#define IMRASSERT(R) \
{\
	assert(R.type == imr::result_type::success); \
}

namespace imr
{
	struct int2
	{
		int x = 0, y = 0;
		int2() : x(), y() {};
		int2(int _x, int _y) : x(_x), y(_y) {};
		int2(float _x, float _y) : x(static_cast<int>(_x)), y(static_cast<int>(_y)) {};
		int length_sqrt() { return x * x + y * y; }

		int2& operator+=(const int2& rh) { this->x += rh.x; this->y += rh.y; return *this; }
		int2& operator-=(const int2& rh) { this->x -= rh.x; this->y -= rh.y; return *this; }
	};

	union int4
	{
		int value[4];
		struct {
			int x;
			int y;
			int z;
			int w;
		};
		struct {
			int2 xy;
			int2 zw;
		};
		struct {
			int2 lt;
			int2 rb;
		};
		int operator[](size_t index) const { return value[index]; }
		int4(int _x, int _y, int _z, int _w) : x(_x), y(_y), z(_z), w(_w) {};
		int4(int2 _xy, int2 _zw) : xy(_xy), zw(_zw) {};
		int4() : x(), y(), z(), w() {};
		int2 rt() const { return { z, y }; }
		int2 lb() const { return { x, w }; }
	};

	struct float2
	{
		float x = 0;
		float y = 0;
		float2() {}
		float2(float _x, float _y)
			:x(_x), y(_y)
		{}
		float2(const int2& rh)
		{
			x = static_cast<float>(rh.x);
			y = static_cast<float>(rh.y);
		}
		float2 rotate(float radian)
		{
			return {
				x * std::cos(radian) - y * std::sin(radian),
				x * std::sin(radian) + y * std::cos(radian)
			};
		}
		float length() const
		{
			return std::sqrtf(x * x + y * y);
		}
		float2 normalize() const
		{
			auto len = length();
			return { x / len, y / len };
		}
		inline float2& operator=(const int2& rh)
		{
			x = static_cast<float>(rh.x);
			y = static_cast<float>(rh.y);
			return *this;
		}
		float2& operator+=(const float2& rh) { this->x += rh.x; this->y += rh.y; return *this; }
		float2& operator*=(const float rh) { this->x *= rh; this->y *= rh; return *this; }
		float2& operator/=(const float rh) { this->x /= rh; this->y /= rh; return *this; }
	};

	union float4
	{
		float value[4];
		struct {
			float x;
			float y;
			float z;
			float w;
		};
		struct {
			float2 xy;
			float2 zw;
		};
		float4& operator+=(const float4& rh) {
			for (auto i = 0; i < 4; ++i)
			{
				this->value[i] += rh[i];
			}
			return *this;
		}
		float operator[](size_t index) const { return value[index]; }
		float4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {};
		float4(float2 _xy, float2 _zw) : xy(_xy), zw(_zw) {};
		float4() : x(), y(), z(), w() {};
	};

	inline float2 to_float2(const int2& v) { return { static_cast<float>(v.x), static_cast<float>(v.y) }; }
	inline bool operator==(const int2& lh, const int2& rh) { return lh.x == rh.x && lh.y == rh.y; }
	inline bool operator!=(const int2& lh, const int2& rh) { return !(lh == rh); }
	inline int2 operator+(const int2& lh, const int2& rh) { return { lh.x + rh.x, lh.y + rh.y }; }
	inline int2 operator-(const int2& lh, const int2& rh) { return { lh.x - rh.x, lh.y - rh.y }; }
	inline int2 operator*(const int2& lh, const int2& rh) { return { lh.x * rh.x, lh.y * rh.y }; }
	inline int2 operator*(const int2& lh, int rh) { return { lh.x * rh, lh.y * rh }; }
	inline int2 operator/(const int2& lh, int rh) { return { lh.x / rh, lh.y / rh }; }
	inline float2 operator+(const float2& lh, const int2& rh) { return { lh.x + rh.x, lh.y + rh.y }; }
	inline float2 operator+(const float2& lh, const float2& rh) { return { lh.x + rh.x, lh.y + rh.y }; }
	inline float2 operator+(float lh, const float2& rh) { return { lh + rh.x, lh + rh.y }; }
	inline float2 operator-(const float2& lh, const float2& rh) { return { lh.x - rh.x, lh.y - rh.y }; }
	inline float2 operator-(float lh, const float2& rh) { return { lh - rh.x, lh - rh.y }; }
	inline float2 operator*(const float2& lh, const float2& rh) { return { lh.x * rh.x, lh.y * rh.y }; }
	inline float2 operator*(const float2& lh, const int2& rh) { return { lh.x * rh.x, lh.y * rh.y }; }
	inline float2 operator*(const float2& lh, float rh) { return { lh.x * rh, lh.y * rh }; }
	inline float2 operator*(float lh, const float2& rh) { return { lh * rh.x, lh * rh.y }; }
	inline float2 operator/(const float2& lh, const float2& rh) { return { lh.x / rh.x, lh.y / rh.y }; }
	inline float2 operator/(const float2& lh, const int2& rh) { return { lh.x / rh.x, lh.y / rh.y }; }
	inline float2 operator/(const int2& lh, const int2& rh) { return { lh.x / static_cast<float>(rh.x), lh.y / static_cast<float>(rh.y) }; }
	inline float2 operator/(const float2& lh, float rh) { return { lh.x / rh, lh.y / rh }; }
	inline float2 operator/(const int2& lh, float rh) { return { lh.x / rh, lh.y / rh }; }
	inline float length(const float2& v) { return std::sqrtf(v.x * v.x + v.y * v.y); }
	inline float4 operator*(const float4& lh, float rh) { return { lh.x * rh, lh.y * rh, lh.z * rh, lh.w * rh }; }

	inline static float4 invalid = { 12452134234.0f, 0, 0, 0 };

	enum result_type
	{
		success,
		fail,
	};

	enum blend_mode
	{
		normal,
		additive,
		multiply,
		screen
	};

	struct result
	{
		result_type type = success;
		int error_code = 0;
		std::string msg = {};
	};

	inline bool succeed(const result& res)
	{
		return res.type == result_type::success;
	}

	inline bool failed(const result& res)
	{
		return res.type == result_type::fail;
	}

	result initialize();
	result deinitialize();
	result on_resolution_changed();

	struct Itexture_info
	{
		virtual ~Itexture_info() = default;
		virtual const std::string& name() const = 0;
		virtual void set_name(const std::string& n) = 0;
		virtual int2 size() const = 0;
		virtual int width() const = 0;
		virtual int height() const = 0;
		virtual void bind() const = 0;
		virtual void unbind() const = 0;
		virtual void destroy() = 0;
	};

	struct Iframe_buffer
	{
		virtual ~Iframe_buffer() = default;
		virtual uintptr_t get_color_texture(int idx = 0) = 0;
		virtual imr::result create(int w, int h, int attachment_count = 1) = 0;
		virtual void destory() = 0;
		virtual void bind() = 0;
		virtual void unbind() = 0;
		virtual int width() const = 0;
		virtual int height() const = 0;
		virtual void bind_color_texture(int attachment_idx = 0) = 0;
	};

	struct frame_backbuffer : public Iframe_buffer
	{
		uintptr_t get_color_texture(int idx = 0) override { return 0; };
		imr::result create(int w, int h, int attachment_count = 1) override
		{
			_w = w;
			_h = h;
			return {};
		}
		void destory() override {}
		void bind() override;
		void unbind() override {}
		int width() const override { return _w; }
		int height() const override { return _h; }
		void bind_color_texture(int attachment_idx = 0) override {}

	private:
		int _w = {};
		int _h = {};
	};

	struct frame_buffer_texture_info : public Itexture_info
	{
		frame_buffer_texture_info(const std::shared_ptr<Iframe_buffer>& frame_buffer, int attachment_idx = 0)
			: _frame_buffer(frame_buffer), _attachment_idx(attachment_idx)
		{}
		Iframe_buffer* get_frame_buffer() { return _frame_buffer.get(); }
		const std::string& name() const { return _name; }
		void set_name(const std::string& n) {}
		int2 size() const override { return { width(), height() }; }
		int width() const override { return _frame_buffer->width(); }
		int height() const override { return _frame_buffer->height(); }
		void bind() const override { _frame_buffer->bind_color_texture(_attachment_idx); }
		void unbind() const override;
		void destroy() override {}

	private:
		std::shared_ptr<Iframe_buffer> _frame_buffer = {};
		std::string _name = {};
		int _attachment_idx = {};
	};

	struct atlas_info
	{
		struct sprite_info
		{
			int2 position = {};
			int2 size = {};
			float2 offset = {};
			float4 uv_rect = {};
		};
		atlas_info(std::shared_ptr<Itexture_info> tex)
			: _texture_info(tex)
		{}

		result add_sprite_info(const std::string& sprite_name, int2 position, int2 size, float2 offset);
		result remove_sprite_info(const std::string& sprite_name);

		const atlas_info::sprite_info* get_sprite_info(const std::string& sprite_name) const
		{
			auto* ret = &_sprites.at(sprite_name);
			return ret;
		}

	private:
		std::shared_ptr<Itexture_info> _texture_info = {};
		std::unordered_map<std::string, atlas_info::sprite_info> _sprites = {};
	};

	inline const char* INSTANCING_PROGRAM_NAME = "_IPN_";
	inline const char* TEXT_PROGRAM_NAME = "_TPN_";
	inline const char* LIGHT_PROGRAM_NAME = "_LPN_";
	inline const char* SPRITE_PROGRAM_NAME = "_SPN_";
	inline const char* MESH_PROGRAM_NAME = "_MPN_";
	inline const char* DEFFERED_PROGRAM_NAME = "_DPN_";
	inline const char* FOG_PROGRAM_NAME = "_FPN_";
	class Iprogram
	{
	public:
		virtual ~Iprogram() = default;
		virtual void use() = 0;
		virtual void unuse() = 0;
		virtual void destroy() = 0;
	};

	inline std::function<std::vector<char>(const std::string& path)> load_data = {};
	std::tuple<result, std::shared_ptr<Itexture_info>> load_texture(const std::string& path);
	std::tuple<result, std::shared_ptr<Itexture_info>> load_texture(const unsigned int* data, int w, int h);
	result regist_program(const std::string& name, std::shared_ptr<Iprogram> program);
	result unregist_program(const std::string& name);
	void push_program(const char* program_name);
	void pop_program();
	const Itexture_info* get_white_texture_info();
}

namespace imr::util
{
	inline int min_power_of_2(int n)
	{
		if ((n & -n) == n) return n;
		n |= n >> 1;
		n |= n >> 2;
		n |= n >> 4;
		n |= n >> 8;
		n |= n >> 16;
		n++;
		return n;
	}
}

namespace imr::camera
{
	enum camera_origin
	{
		center,
		left_top,
		left_bottom,
		left_center,
	};

	struct begin_args
	{
		Iframe_buffer* frame_buffer = {};
		camera_origin origin = center;
	};

	struct camera_args
	{
		float2 position = {};
		float2 scale = { 1.0f, 1.0f };
		float rotation = {};
	};

	void enable_depth_test();
	void disable_depth_test();

	result begin(const begin_args& args);
	void clear(const float4& color = {});
	result camera(const camera_args& args);
	std::tuple<result, float2> screen_to_world(const float2& scr_pos);
	result end();
}

namespace imr::sprite
{
	result begin_try_batch();
	struct draw_args
	{
		const Itexture_info* texture_info = {};
		const Itexture_info* texture_info_1 = {};
		const Itexture_info* texture_info_2 = {};
		const Itexture_info* texture_info_3 = {};
		const atlas_info::sprite_info* sprite_info = {};
		float2 position = {};
		float2 scale = { 1.0f, 1.0f };
		float rotation = {};
		float4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		float2 offset = {};
	};
	result draw(const draw_args& args);
	result end_try_batch();
	result draw_single(Itexture_info* tex_info, const float2& position, const float2& scale = { 1, 1 }, float rotation = 0, const float4& color = { 1, 1, 1, 1 }, const float2& offset = {});
}

namespace imr::instancing
{
	struct begin_args
	{
		const Itexture_info* texture_info = {};
		const Itexture_info* texture_info_1 = {};
		const Itexture_info* texture_info_2 = {};
		const Itexture_info* texture_info_3 = {};
	};
	struct instance_args
	{
		const atlas_info::sprite_info* sprite_info = {};
		float2 position = {};
		float2 scale = { 1.0f, 1.0f };
		float rotation = {};
		float4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
		float2 offset = {};
	};
	result begin(const begin_args& args);
	result instance(const instance_args& args);
	result begin(const Itexture_info* texture, const Itexture_info* texture_1 = nullptr, const Itexture_info* texture_2 = nullptr, Itexture_info* texture_3 = nullptr);
	result instance(const float2& sprite_pos, const float2& sprite_size, const float2& offset, const float2& position, const float2& scale = { 1, 1 }, const float rotation = 0, const float4& color = { 1, 1, 1, 1 });
	result end();
}

namespace imr::blend
{
	result push_addictive();
	result pop();
}

namespace imr::mesh
{
	struct vertex
	{
		float2 position = {};
		float2 uv = {};
		float4 color = { 1, 1, 1, 1 };
	};
	struct draw_args
	{
		std::vector<vertex> vertices = {};
		std::vector<unsigned short> indices = {};
		Itexture_info* texture_info = {};
		blend_mode blend = {};
	};
	result draw(const draw_args& args);

	result begin();
	result use_program(const std::string& name);
	result set_use_projection_view_matrix(bool val);
	result push_meshes(const float* vertices, int v_stride, size_t v_cnt, const unsigned short* indices, size_t i_cnt);
	result set_uniform_mat4(int location, const float* data, int length);
	result set_uniform_vec4(int location, const float* data, int length);
	result set_texture(int location, Itexture_info* texture);
	result vertex_attrib_pointer(int location, int count, int stride, int offset);
	result end();
};

namespace imr::primitive
{
	struct line_args
	{
		float2 from = {};
		float2 to = {};
		float thickness = 1.0f;
		float4 color = { 1, 1, 1, 1 };
	};
	struct circle_args
	{
		float2 center = {};
		float radius = 50.0f;
		float thickness = 1.0f;
		float4 color = { 1, 1, 1, 1 };
		int segment = 10;
	};
	result begin();
	result line(const line_args& args);
	result circle(const circle_args& args);
	result end();
}

namespace imr::input
{
	enum e_state
	{
		release = 0,
		press
	};

	namespace touch
	{
		enum e_touch_id
		{
			touch_0 = 0,
			touch_1,
			touch_2,
		};

		enum e_touch_state
		{
			none,
			press,
			release,
			touch
		};

		inline std::unordered_map<e_touch_id, float2> _touch_position = {};
		inline void _set_position(e_touch_id id, const float2& pos)
		{
			_touch_position[id] = pos;
		}
		inline std::unordered_map<e_touch_id, e_touch_state> _touch_state = {};
		inline void _state_reset()
		{
			_touch_state.clear();
		}
		inline void _set_touch(e_touch_id id)
		{
			_touch_state[id] = touch;
		}
		inline bool is_touch(e_touch_id id = touch_0)
		{
			return _touch_state[id] == touch;
		}
		inline const float2& touch_position(e_touch_id id = touch_0)
		{
			return _touch_position[id];
		}
	}

	namespace keyboard
	{
		enum e_key
		{
			KEY_UP,
			KEY_RIGHT,
			KEY_DOWN,
			KEY_LEFT,
			KEY_SPACE,
			KEY_Q,
			KEY_W,
			KEY_E,
			KEY_R,
			KEY_0,
			KEY_1,
			KEY_2,
			KEY_3,
			KEY_4,
			KEY_5,
			KEY_6,
			KEY_7,
			KEY_8,
			KEY_9,
		};

		inline std::unordered_map<e_key, e_state> _states = {};
		inline std::unordered_set<e_key> _first_press = {};
		inline e_state key_state(e_key key) { return _states[key]; }
		inline void set_key_state(e_key key, e_state state) 
		{ 
			if (_states[key] == e_state::release && state == e_state::press)
			{
				_first_press.insert(key);
			}
			_states[key] = state; 
		}
		inline void clear_first_states()
		{
			_first_press.clear();
		}
		inline bool is_first_press(e_key key) { return _first_press.contains(key); }
	}

	class gamepad
	{
	public:
		enum e_button
		{
			A = 0, // CROSS
			B, // CIRCLE
			X, // SQUARE
			Y, // TRIANGLE
			LEFT_BUMPER,
			RIGHT_BUMPER,
			BACK,
			START,
			GUIDE,
			LEFT_THUMB,
			RIGHT_THUMB,
			UP,
			RIGHT,
			DOWN,
			LEFT,
			LEFT_TRIGGER,
			RIGHT_TRIGGER,
		};

		gamepad(int pad_id) : _pad_id(pad_id) {}
		~gamepad() = default;
		e_state button_state(e_button btn) {
			return _states[btn];
		}
		const float2& axis() { return _axis; }
		void poll_events();
	private:
		int _pad_id = {};
		std::unordered_map<e_button, e_state> _states = {};
		float2 _axis = {};
	};
}
