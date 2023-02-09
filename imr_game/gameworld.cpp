#include "gameworld.h"
#include "imr_core.h"
#include <iostream>

namespace imr::game
{
	systems::~systems()
	{
		for (auto* sys : _systems)
		{
			_game_context->raw_pool(typeid(*sys))->destroy((void**)&sys);
		}
	}

	void systems::installed(gameworld* world)
	{
		for (auto* s : _systems)
		{
			s->installed(world);
		}
	}

	void systems::fixed_update(gameworld* world, float fdt)
	{
		for (auto* s : _systems)
		{
			s->fixed_update(world, fdt);
		}
	}

	void systems::update(gameworld* world, float dt)
	{
		for (auto* s : _systems)
		{
			s->update(world, dt);
		}
	}

	void systems::render(gameworld* world)
	{
		for (auto* s : _systems)
		{
			s->render(world);
		}
	}

	void systems::ui(gameworld* world)
	{
		for (auto* s : _systems)
		{
			s->ui(world);
		}
	}

	universe::universe()
	{
		_physics_world->SetContactListener(this);
	}

	universe::~universe()
	{
		while (_commands.size())
		{
			auto* cmd = _commands.front();
			_commands.pop();
			_game_context->raw_pool(typeid(*cmd))->destroy((void**)&cmd);
		}
	}

	void universe::add_command(Iuniverse_command* command)
	{
		_commands.push(command);
	}

	void universe::run_command(Iuniverse_command* command)
	{
		command->run(this);
		_game_context->raw_pool(typeid(*command))->destroy((void**)&command);
	}

	gameworld* universe::create_world(world_id id)
	{
		if (_gameworlds.contains(id) == false)
		{
			_gameworlds[id]._universe = this;
			_gameworlds[id]._game_context = this->_game_context;
		}
		return get_world(id);
	}

	void universe::BeginContact(b2Contact* contact)
	{
		auto* fixture_a = contact->GetFixtureA();
		auto* fixture_b = contact->GetFixtureB();
		auto* a = reinterpret_cast<Icontactable*>(fixture_a->GetUserData().pointer);
		auto* b = reinterpret_cast<Icontactable*>(fixture_b->GetUserData().pointer);
		assert(a != nullptr);
		assert(b != nullptr);
		a->contact_list[fixture_a].insert(b);
		b->contact_list[fixture_b].insert(a);
		a->on_begin_contact(b);
		b->on_begin_contact(a);
	}

	void universe::EndContact(b2Contact* contact)
	{
		auto* fixture_a = contact->GetFixtureA();
		auto* fixture_b = contact->GetFixtureB();
		auto* a = reinterpret_cast<Icontactable*>(fixture_a->GetUserData().pointer);
		auto* b = reinterpret_cast<Icontactable*>(fixture_b->GetUserData().pointer);
		assert(a != nullptr);
		assert(b != nullptr);
		a->contact_list[fixture_a].erase(b);
		if (a->contact_list[fixture_a].size() == 0)
			a->contact_list.erase(fixture_a);
		b->contact_list[fixture_b].erase(a);
		if (b->contact_list[fixture_b].size() == 0)
			b->contact_list.erase(fixture_b);
		a->on_end_contact(b);
		b->on_end_contact(a);
	}

	void universe::fixed_update()
	{
		float fdt = fixed_delta();
		_physics_world->Step(fdt, 6, 2);
		for (auto& p : _gameworlds)
		{
			if (p.second.is_enabled())
			{
				p.second.fixed_update();
			}
		}
	}

	void universe::update(float dt)
	{
		while (_commands.size() > 0)
		{
			auto* cmd = _commands.front();
			_commands.pop();
			run_command(cmd);
		}

		_time += dt;
		_fixed_elapsed += dt;

		float fdt = fixed_delta();
		if (_fixed_elapsed >= fdt)
		{
			fixed_update();
			_fixed_elapsed -= fdt;
		}
		for (auto& p : _gameworlds)
			{
			if (p.second.is_enabled())
			{
				p.second.update(dt);
			}
		}
	}

	void universe::render()
	{
		for (auto& p : _gameworlds)
			{
			if (p.second.is_enabled())
			{
				p.second.render();
			}
		}
	}

	void universe::ui()
	{
		for (auto& p : _gameworlds)
			{
			if (p.second.is_enabled())
			{
				p.second.ui();
			}
		}
	}

	gameworld::~gameworld()
	{
		std::unordered_set<gameobject*> removed = {};
		for (auto& p : _gameobjects)
		{
			removed.insert(p.second);
		}
		for (auto* go : removed)
		{
			destroy_gameobject(&go);
		}
		while (_commands.size())
		{
			auto* cmd = _commands.front();
			_commands.pop();
			_game_context->raw_pool(typeid(*cmd))->destroy((void**)&cmd);
		}
	}

	void gameworld::enable(bool flag)
	{
		for (auto& p : _gameobjects)
		{
			p.second->enable(flag);
		}
		_enabled = flag;
	}

	void gameworld::add_command(Icommand* command)
	{
		_commands.push(command);
	}

	void gameworld::run_command(Icommand* command)
	{
		command->run(this);
		_game_context->raw_pool(typeid(*command))->destroy((void**)&command);
	}

	gameobject* gameworld::create_gameobject()
	{
		auto* ret = _game_context->pool<gameobject>()->create();
		ret->_world = this;
		ret->_id = _universe->_gameobject_id_stack++;
		_gameobjects[ret->_id] = ret;
		return ret;
	}

	void gameworld::destroy_gameobject(gameobject** go)
	{
		go_id goid = (*go)->get_gameobject_id();
		destroy_gameobject_by_id(goid);
	}

	void gameworld::destroy_gameobject_by_id(go_id goid)
	{
		const auto& it = _gameobjects.find(goid);
		if (it != _gameobjects.end())
		{
			_game_context->pool<gameobject>()->destroy(&it->second);
			_gameobjects.erase(goid);
		}
	}

	void gameworld::fixed_update()
	{
		float fdt = _universe->fixed_delta();
		_systems->fixed_update(this, fdt);
	}

	void gameworld::update(float dt)
	{
		while (_commands.size() > 0)
		{
			auto* cmd = _commands.front();
			_commands.pop();
			run_command(cmd);
		}

		_systems->update(this, dt);
	}

	void gameworld::render()
	{
		_systems->render(this);
	}

	void gameworld::ui()
	{
		_systems->ui(this);
	}

	void gameworld::translate(gameworld* prev, gameworld* new_world, gameobject* target)
	{
		for (auto& p : target->_components)
		{
			prev->_remove_gameobject_by_component(p.first, target);
			new_world->_add_gameobject_by_component(p.first, target);
		}
		prev->_gameobjects.erase(target->get_gameobject_id());
		new_world->_gameobjects[target->get_gameobject_id()] = target;
		target->_world = new_world;
	}
}