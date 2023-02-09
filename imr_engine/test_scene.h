#pragma once

#include "imr_opengl3.h"
#include <irrKlang.h>
#include <box2d/box2d.h>
#include <json/json.h>
#include <iostream>

namespace
{
	void get_json(const char* json_path, Json::Value* root)
	{
		auto data = imr::load_data(json_path);
		std::string json(data.data());
		Json::CharReaderBuilder builder = {};
		auto* reader = builder.newCharReader();
		std::string errs = {};
		auto result = reader->parse(json.c_str(), json.c_str() + json.size(), root, &errs);
		assert(result);
		delete reader;
	}
}

class test_scene : public imr::game::Iscene, b2ContactListener
{
private:
	std::shared_ptr<imr::Itexture_info> _haul_texture_info = {};
	std::shared_ptr<imr::Itexture_info> _pocketmon_texture_info = {};
	std::shared_ptr<imr::Itexture_info> _atlas_texture_info = {};
	std::shared_ptr<imr::atlas_info> _atlas_info = {};
	std::shared_ptr<imr::sprite::animation::animation_state> _animation_state = {};
	std::shared_ptr<imr::frame_buffer> _frame_buffer = {};

public:
	const std::shared_ptr<imr::Iframe_buffer> get_frame_buffer() const override { return _frame_buffer; }

	imr::result load_resource() override
	{
		parse_cards_json();
		auto [res, tex_info] = imr::load_texture("haul.png");
		_haul_texture_info = tex_info;
		auto [resa, _tex_info] = imr::load_texture("pocketmon.png");
		_pocketmon_texture_info = _tex_info;

		auto [r, _tex_info_] = imr::load_texture("atlas.png");
		_atlas_texture_info = _tex_info_;

		_atlas_info = std::make_shared<imr::atlas_info>(_atlas_texture_info);
		_atlas_info->add_sprite_info("run", { 64 * 3, 0 }, { 64, 64 }, { 0.5f, 0.0f });
		_atlas_info->add_sprite_info("run1", { 64 * 1, 64 * 1 }, { 64, 64 }, { 0.5f, 0.0f });

		imr::sprite::animation::animation_state_builder builder = {};
		auto state_data = builder
			.set_texture_atlas(_atlas_texture_info, _atlas_info)
			.add_animation("run", { "run", "run1" }, 1.0f)
			.build();
		_animation_state = std::make_shared<imr::sprite::animation::animation_state>();
		_animation_state->set_animation_state_data(state_data);
		_animation_state->set_animation("run", true);

		_frame_buffer = std::make_shared<imr::frame_buffer>();
		_frame_buffer->create(800, 600);

		return {};
	}

	bool begin_con = false;
	void BeginContact(b2Contact* contact) override
	{
		begin_con = true;
	}
	void EndContact(b2Contact* contact) override
	{
		begin_con = false;
	}
	void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override
	{
		if (contact->GetFixtureB()->GetBody() == _body)
		{
			auto d = contact->GetManifold()->localNormal;
			contact->SetEnabled(d.y > 0);
		}
	}
	imr::result begin() override
	{
		_engine->play2D("getout.ogg", true);

		b2Vec2 gravity = { .0f, 0.0f };
		_world = std::make_shared<b2World>(gravity);

		_world->SetContactListener(this);

		b2BodyDef ground_body_def;
		ground_body_def.position.Set(pixel_to_unit(400), pixel_to_unit(10));
		_ground_body = _world->CreateBody(&ground_body_def);
		b2PolygonShape ground_box;
		ground_box.SetAsBox(pixel_to_unit(40000), pixel_to_unit(10));
		_ground_body->CreateFixture(&ground_box, 0.0f);

		{
			b2BodyDef gr_def;
			gr_def.position.Set(pixel_to_unit(400), pixel_to_unit(160));
			b2Body* gr = _world->CreateBody(&gr_def);
			b2PolygonShape gr_box;
			gr_box.SetAsBox(pixel_to_unit(100), pixel_to_unit(10));
			gr->CreateFixture(&gr_box, 0.0f);
		}

		{
			b2BodyDef gr_def;
			gr_def.position.Set(pixel_to_unit(200), pixel_to_unit(80));
			b2Body* gr = _world->CreateBody(&gr_def);
			b2PolygonShape gr_box;
			gr_box.SetAsBox(pixel_to_unit(100), pixel_to_unit(10));
			gr->CreateFixture(&gr_box, 0.0f);
		}

		b2BodyDef body_def;
		body_def.type = b2_dynamicBody;
		body_def.position.Set(pixel_to_unit(400), pixel_to_unit(100));
		//body_def.angle = 1;
		_body = _world->CreateBody(&body_def);
		b2PolygonShape dynamic_box;
		dynamic_box.SetAsBox(pixel_to_unit(50), pixel_to_unit(50));

		b2CircleShape circle_shape;
		circle_shape.m_radius = pixel_to_unit(10);

		b2FixtureDef fixture_def;
		fixture_def.shape = &circle_shape;
		fixture_def.density = 10.0f;
		fixture_def.friction = 10.0f;
		fixture_def.restitution = 0.0f;

		_body->CreateFixture(&fixture_def);
		_body->SetAngularDamping(50.0f);

		{
			auto [res, spine_data] = imr::spine::load("spine_sample/spineboy.atlas", "spine_sample/spineboy-pro.skel");
			IMRRESULT(res);
			imr::spine::regist_spine_data("spineboy", spine_data);
		}
		{
			auto [res, spine_instance] = imr::spine::instance("spineboy");
			IMRRESULT(res);
			spine_instance->skeleton->setScaleX(0.5f);
			spine_instance->skeleton->setScaleY(0.5f);
			spine_instance->set_animation(0, "portal", true);
			_spine_instance = spine_instance;
			_spine_instance->set_position({ 20, 50 });
		}
		{
			auto [res, spine_instance] = imr::spine::instance("spineboy");
			IMRRESULT(res);
			spine_instance->skeleton->setScaleX(0.5f);
			spine_instance->skeleton->setScaleY(0.5f);
			spine_instance->set_animation(0, "walk", true);
			_spine_instance2 = spine_instance;
			_spine_instance2->set_position({ 0, 80 });
		}

		return {};
	}
	imr::result end() override
	{
		_body = nullptr;
		_ground_body = nullptr;
		_world = nullptr;
		return {};
	}
	imr::result unload_resource() override
	{
		imr::spine::unregist_spine_data("spineboy");
		return {};
	}
	imr::result update(float dt) override
	{
		_gamepad.poll_events();
		_elapsed += dt;
		const auto phys_dt = 1.0f / 60.0f;
		if (_elapsed >= phys_dt)
		{
			_world->Step(phys_dt, 6, 2);
			_elapsed -= phys_dt;
		}
		if (_tweener)
		{
			_tweener->update(dt);
			if (_tweener->finish())
			{
				_tweener = nullptr;
			}
		}
		if (_spine_instance)
		{
			//auto& pos = _body->GetPosition();
			//_spine_instance->set_position({ unit_to_pixel(pos.x), unit_to_pixel(pos.y) });
			//if (_body->GetLinearVelocity().x == 0)
			//{
			//	//_spine_instance->set_animation(0, "idle", true);
			//}
			//else
			//{
			//	auto is_right = _body->GetLinearVelocity().x >= 0;
			//	_spine_instance->skeleton->setScaleX(is_right ? 0.5f : -0.5f);
			//}
			_spine_instance->update(dt);
		}
		if (_spine_instance2)
		{
			_spine_instance2->update(dt);
		}
		static bool pressed_a = false;
		auto is_jumpable = [this]() {
			auto* con = _body->GetContactList();
			while (con != nullptr)
			{
				if (con->contact->IsEnabled() && con->contact->IsTouching())
				{
					return true;
				}
				con = con->next;
			}
			return false;
		};

		if (pressed_a == false && is_jumpable() && _gamepad.button_state(imr::input::gamepad::e_button::A) == imr::input::press)
		{
			pressed_a = true;
			_body->ApplyLinearImpulseToCenter({ 0, 1.5f }, true);
		}
		else
		{
			pressed_a = _gamepad.button_state(imr::input::gamepad::A) == imr::input::press;
		}
		if (_gamepad.button_state(imr::input::gamepad::e_button::LEFT) == imr::input::press)
		{
			if (_body->GetContactList())
			{
				_body->ApplyTorque(5, true);
			}
			else// if (b2Dot(_body->GetLinearVelocity(), b2Vec2(-1, 0)) <= 0)
			{
				_body->ApplyLinearImpulseToCenter({ -0.05f, 0 }, true);
			}
		}
		if (_gamepad.button_state(imr::input::gamepad::e_button::RIGHT) == imr::input::press)
		{
			if (_body->GetContactList())
			{
				_body->ApplyTorque(-5, true);
			}
			else// if (b2Dot(_body->GetLinearVelocity(), b2Vec2(-1, 0)) >= 0)
			{
				_body->ApplyLinearImpulseToCenter({ 0.05f, 0 }, true);
			}
		}
		const float max_speed = 4;
		if (auto& v = _body->GetLinearVelocity(); std::abs(v.x) > max_speed)
		{
			auto vx = v.x / std::abs(v.x);
			vx *= max_speed;
			_body->SetLinearVelocity({ vx, v.y });
		}
		_animation_state->update(dt);

		return {};
	}
	imr::result render(const imr::int2& resolution) override
	{
		if (succeed(imr::camera::begin({ .frame_buffer = _frame_buffer.get(), /*.origin = imr::camera::left_bottom*/ })))
		{
			imr::camera::clear({ .0f, .0f, .0f, 1.0f });

			auto bpos = _body->GetPosition();
			imr::camera::camera({
				.position = {unit_to_pixel(bpos.x), unit_to_pixel(bpos.y)}
			});

			if (imr::input::touch::is_touch())
			{
				const auto& tp = imr::input::touch::touch_position();
				auto [rr, pos] = imr::camera::screen_to_world(tp);
				std::cout << tp.x << ":" << tp.y << "   -   " << pos.x << ":" << pos.y << std::endl;

				_tweener = tweeners::builder<imr::float2>()
					.from_to(_position, pos, 3, 0, "easeInBounce")
					.build(tweeners::ONCE, 0, [&p = _position](float t, const imr::float2& v) {
					p = v;
					return true;
				});
			}

			_spine_instance->draw();
			_spine_instance2->draw();

			{
				imr::sprite::begin_try_batch();
				auto [atlas, sprite] = _animation_state->current_animation();
				imr::sprite::draw({
					.texture_info = _atlas_texture_info.get(),
					.sprite_info = sprite,
					.position = _position,
					});

				imr::sprite::draw({
					.texture_info = _pocketmon_texture_info.get(),
					.position = {0, 0},
					.scale = {0.1f, 0.1f}
					});

				imr::sprite::draw({
					.texture_info = _pocketmon_texture_info.get(),
					.position = {20, 0},
					.scale = {0.05f, 0.05f}
					});

				imr::sprite::draw({
					.texture_info = _haul_texture_info.get(),
					.position = {30, 0},

					});

				imr::sprite::draw({
					.position = {200, 200},
					.scale = {35, 35},
					.color = {1, 1, 0, 1}
				});

				imr::sprite::draw({
					.position = {400, 200},
					.scale = {35, 35},
					.color = {1, 1, 0, 1}
				});

				imr::sprite::end_try_batch();
			}

			if (succeed(imr::instancing::begin({ .texture_info = _atlas_texture_info.get() })))
			{
				imr::instancing::instance(imr::instancing::instance_args{
					.sprite_info = _atlas_info->get_sprite_info("run"),
					.position = {100, 0},
				});
				imr::instancing::instance({
					.sprite_info = _atlas_info->get_sprite_info("run1"),
					.position = {500, 400},

				});
				imr::instancing::end();
			}

			if (succeed(imr::primitive::begin()))
			{
				imr::primitive::line({
					.from = {0, 0},
					.to = {400, 300},
					.thickness = 2,
					.color = {0, 0, 1, 1},
					});

				imr::primitive::line({
					.from = {0, 0},
					.to = imr::float2{ 400, 300 }.rotate(1 * glm::pi<float>() / 180.0f),
					.thickness = 2,
					.color = {0, 1, 0, 1},
				});

				imr::primitive::line({
					.from = {100, 100},
					.to = {100, 50},
					.thickness = 2,
					.color = {0, 0, 1, 1},
					});

				imr::primitive::line({
					.from = {400, 300},
					.to = {500, 300},
					.thickness = 2,
					.color = {0, 0, 1, 1},
					});

				imr::primitive::line({
					.from = {400, 300},
					.to = {300, 300},
					.thickness = 2,
					.color = {1, 0, 0, 1},
					});

				imr::primitive::circle({
					.center = {400, 300},
					.radius = 100,
					.thickness = 2,
					});

				// box2d
				{
					b2Body* body = _world->GetBodyList();
					while (body != nullptr)
					{
						draw_body(body);
						body = body->GetNext();
					}
				}

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
	const float UNIT_PER_PIXEL = 100;
	float _elapsed = 0;
	imr::float2 _position = {};
	std::shared_ptr<tweeners::Itweener> _tweener = {};
	std::shared_ptr<irrklang::ISoundEngine> _engine = std::shared_ptr<irrklang::ISoundEngine>(irrklang::createIrrKlangDevice());
	std::shared_ptr<b2World> _world = {};
	b2Body* _ground_body = {};
	b2Body* _body = {};
	imr::input::gamepad _gamepad = imr::input::gamepad(0);
	std::shared_ptr<imr::spine::spine_instance> _spine_instance = {};
	std::shared_ptr<imr::spine::spine_instance> _spine_instance2 = {};

	float unit_to_pixel(float unit)
	{
		return unit * UNIT_PER_PIXEL;
	}

	float pixel_to_unit(float pixel)
	{
		return pixel / UNIT_PER_PIXEL;
	}

	void parse_cards_json()
	{
		//read card imag
		Json::Value root;
		::get_json("cards.json", &root);
		Json::Value frames = root["frames"];
		for (decltype(frames.size()) i = 0; i < frames.size(); ++i)
		{
			Json::Value item = frames[i];
			std::cout << item["filename"].asString() << std::endl;
		}
	}

	void draw_body(b2Body* body)
	{
		imr::primitive::begin();

		auto& trans = body->GetTransform();
		b2Fixture* fixture = body->GetFixtureList();
		while (fixture != nullptr)
		{
			auto type = fixture->GetType();
			if (type == b2Shape::Type::e_polygon)
			{
				b2PolygonShape* shape = dynamic_cast<b2PolygonShape*>(fixture->GetShape());
				b2Vec2 p0 = b2Mul(trans, shape->m_vertices[0]);
				for (int i = 1; i < shape->m_count; ++i)
				{
					b2Vec2 p1 = b2Mul(trans, shape->m_vertices[i]);
					imr::primitive::line({
						.from = {unit_to_pixel(p0.x), unit_to_pixel(p0.y)},
						.to = {unit_to_pixel(p1.x), unit_to_pixel(p1.y)},
						.thickness = 2,
						.color = {1, 0, 0, 1},
					});
					p0 = p1;
				}
				b2Vec2 first = b2Mul(trans, shape->m_vertices[0]);
				imr::primitive::line({
					.from = {unit_to_pixel(p0.x), unit_to_pixel(p0.y)},
					.to = {unit_to_pixel(first.x), unit_to_pixel(first.y)},
					.thickness = 2,
					.color = {1, 0, 0, 1},
				});
			}
			else if (type == b2Shape::Type::e_circle)
			{
				b2CircleShape* shape = dynamic_cast<b2CircleShape*>(fixture->GetShape());
				b2Vec2 pos = b2Mul(trans, shape->m_p);
				b2Vec2 dir(shape->m_radius, 0);
				dir = b2Mul(trans, dir);
				imr::float2 center = { unit_to_pixel(pos.x), unit_to_pixel(pos.y) };
				imr::primitive::circle({
					.center = center,
					.radius = unit_to_pixel(shape->m_radius),
					.thickness = 2,
					.segment = 20,
				});
				imr::primitive::line({
					.from = center,
					.to = { unit_to_pixel(dir.x), unit_to_pixel(dir.y) },
					.thickness = 2,
				});

			}
			fixture = fixture->GetNext();
		}

		imr::primitive::end();
	}
};

class test_scene2 : public imr::game::Iscene
{
private:
	std::shared_ptr<imr::Itexture_info> _atlas_texture_info = {};
	std::shared_ptr<imr::atlas_info> _atlas_info = {};
	std::shared_ptr<imr::sprite::animation::animation_state> _animation_state = {};
	std::shared_ptr<imr::frame_buffer> _frame_buffer = {};

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
		auto [r, _tex_info_] = imr::load_texture("atlas.png");
		_atlas_texture_info = _tex_info_;

		_atlas_info = std::make_shared<imr::atlas_info>(_atlas_texture_info);
		_atlas_info->add_sprite_info("run", { 64 * 3, 0 }, { 64, 64 }, { 0.5f, 0.0f });
		_atlas_info->add_sprite_info("run1", { 64 * 1, 64 * 1 }, { 64, 64 }, { 0.5f, 0.0f });

		imr::sprite::animation::animation_state_builder builder = {};
		auto state_data = builder
			.set_texture_atlas(_atlas_texture_info, _atlas_info)
			.add_animation("run", { "run", "run1" }, 1.0f)
			.build();
		_animation_state = std::make_shared<imr::sprite::animation::animation_state>();
		_animation_state->set_animation_state_data(state_data);
		_animation_state->set_animation("run", true);

		_frame_buffer = std::make_shared<imr::frame_buffer>();
		_frame_buffer->create(800, 600);
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
#ifdef _DEBUG
		const int test_cnt = 10000;
#else 
		const int test_cnt = 100000;
#endif
		if (succeed(imr::camera::begin({ .frame_buffer = _frame_buffer.get() })))
		{
			imr::camera::clear({ 1, 1, 0, 1 });

			if (succeed(imr::instancing::begin(_animation_state->get_texture_info())))
			{
				for (int i = 0; i < test_cnt; ++i)
				{
					imr::instancing::instance(_animation_state.get(), { (float)(std::rand() % 800 - 400), (float)(std::rand() % 600 - 300) });
				}
				imr::instancing::end();
			}

			imr::camera::end();
		}
		return {};
	}

	imr::result ui(const imr::int2& resolution) override
	{
		return {};
	}
};
