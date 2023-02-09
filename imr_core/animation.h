#pragma once

#include "imr_core.h"

namespace imr::sprite::animation
{
	struct animation_data
	{
		std::string animation_name = {};
		float duration = {};
		std::vector<std::string> sprite_names = {};
		std::vector<const imr::atlas_info::sprite_info*> sprite_infoes = {};
	};

	struct animation_state_data
	{
		std::shared_ptr<imr::Itexture_info> texture = {};
		std::shared_ptr<atlas_info> atlas = {};
		std::unordered_map<std::string, animation_data> animations = {};
	};

	class animation_state_builder
	{
	private:
		class animation_builder
		{
		public:
			void set_texture_atlas(std::shared_ptr<Itexture_info> texture, std::shared_ptr<atlas_info> atlas)
			{
				_data->texture = texture;
				_data->atlas = atlas;
			}

			animation_builder& add_animation(const std::string& anim_name, const std::vector<std::string>& sprites, float duration)
			{
				std::vector<const imr::atlas_info::sprite_info*> sprite_infoes = {};
				for (auto& sprite_name : sprites)
				{
					auto* info = _data->atlas->get_sprite_info(sprite_name);
					sprite_infoes.push_back(info);
				}
				_data->animations[anim_name] = animation_data{ anim_name, duration, sprites, sprite_infoes };
				return *this;
			}

			std::shared_ptr<animation_state_data> build()
			{
				return _data;
			}
		private:
			std::shared_ptr<animation_state_data> _data = std::make_shared<animation_state_data>();
		};
	public:
		animation_builder& set_texture_atlas(std::shared_ptr<Itexture_info> texture, std::shared_ptr<atlas_info> atlas)
		{
			_anim_builder.set_texture_atlas(texture, atlas);
			return _anim_builder;
		}
	private:
		animation_builder _anim_builder = {};
	};

	class animation_state
	{
	public:
		~animation_state() = default;

		void set_animation_state_data(std::shared_ptr<animation_state_data>& state_data)
		{
			_state_data = state_data;
		}

		result set_animation(const std::string& animation_name, bool loop, bool reset = true)
		{
			if (reset == false && _current_animation && _current_animation->animation_name == animation_name)
			{
				return {};
			}
			_current_animation = &_state_data->animations[animation_name];
			_sequence = 0;
			_elapsed = 0;
			_loop = loop;
			return {};
		}

		void update(float dt)
		{
			if (_current_animation == nullptr)
			{
				return;
			}
			_elapsed += dt;
			const auto time_per_slice = _current_animation->duration / _current_animation->sprite_names.size();
			if (_elapsed >= time_per_slice)
			{
				_elapsed -= time_per_slice;
				if (_sequence == _current_animation->sprite_names.size() - 1)
				{
					if (_loop)
						_sequence = 0;
				}
				else
				{
					_sequence++;
				}
			}
		}

		std::tuple<const Itexture_info*, const atlas_info::sprite_info*> current_animation()
		{
			return { _state_data->texture.get(), _current_animation->sprite_infoes[_sequence] };
		}

		const imr::atlas_info::sprite_info* current_sprite_info()
		{
			return _current_animation->sprite_infoes[_sequence];
		}

		const Itexture_info* get_texture_info() { return _state_data->texture.get(); }

	private:
		std::shared_ptr<animation_state_data> _state_data = {};
		animation_data* _current_animation = {};
		int _sequence = {};
		float _elapsed = {};
		bool _loop = {};
	};
}

namespace imr::instancing
{
	result begin(Itexture_info* texture_info);
	result instance(imr::sprite::animation::animation_state* anim_state, const float2& position = { 0, 0 }, const float2& scale = { 1, 1 }, const float rotation = 0, const float4& color = { 1, 1, 1, 1 });
}
