#include <Windows.h>
#include <tchar.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "test_scene.h"
#include "font_test_scene.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h> 
#include <string>
#include <assert.h>
#include <iostream>
#include <filesystem>
#include <fstream>

#pragma comment(lib, "Version.lib")

extern "C"
{
	__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}

std::string get_version()
{
	std::stringstream ss{};
	TCHAR buf[MAX_PATH] = { 0, };
	TCHAR szVersionFile[MAX_PATH]; 	GetModuleFileName(NULL, szVersionFile, MAX_PATH);

	if (FALSE != ::GetFileVersionInfo(szVersionFile, 0, MAX_PATH, buf)) {
		VS_FIXEDFILEINFO* pFineInfo = NULL;
		UINT bufLen = 0;
		// VS_FIXEDFILEINFO 정보 가져오기
		if (FALSE != VerQueryValue(buf, _T("\\"), (LPVOID*)&pFineInfo, &bufLen) != 0) {
			WORD majorVer, minorVer, buildNum, revisionNum;
			majorVer = HIWORD(pFineInfo->dwFileVersionMS);
			minorVer = LOWORD(pFineInfo->dwFileVersionMS);
			buildNum = HIWORD(pFineInfo->dwFileVersionLS);
			revisionNum = LOWORD(pFineInfo->dwFileVersionLS);
			ss << majorVer << "." << minorVer << "." << buildNum << "." << revisionNum;
		}
	}
	return ss.str();
}

static void glfw_error_callback(int error, const char* description)
{
	std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

void imr::input::gamepad::poll_events()
{
	auto poll_joysticks = [this]() {
		if (glfwJoystickPresent(_pad_id) <= 0)
		{
			return;
		}

		if (glfwJoystickIsGamepad(_pad_id) <= 0)
		{
			return;
		}

		GLFWgamepadstate state;
		glfwGetGamepadState(GLFW_JOYSTICK_1, &state);

		_states[e_button::UP] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS ? e_state::press : e_state::release;
		_states[e_button::DOWN] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS ? e_state::press : e_state::release;
		_states[e_button::LEFT] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS ? e_state::press : e_state::release;
		_states[e_button::RIGHT] = state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS ? e_state::press : e_state::release;
		_states[e_button::A] = state.buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS ? e_state::press : e_state::release;
	};
	poll_joysticks();
}

void poll_keyboard_input(GLFWwindow* window)
{
#define GETSET_KEY(G) \
	imr::input::keyboard::set_key_state(imr::input::keyboard::KEY_##G, glfwGetKey(window, GLFW_KEY_##G) == GLFW_PRESS ? imr::input::press : imr::input::release)

	imr::input::keyboard::clear_first_states();
	GETSET_KEY(SPACE);
	GETSET_KEY(UP);
	GETSET_KEY(RIGHT);
	GETSET_KEY(DOWN);
	GETSET_KEY(LEFT);
	GETSET_KEY(Q);
	GETSET_KEY(W);
	GETSET_KEY(E);
	GETSET_KEY(R);
	GETSET_KEY(0);
	GETSET_KEY(1);
	GETSET_KEY(2);
	GETSET_KEY(3);
	GETSET_KEY(4);
	GETSET_KEY(5);
	GETSET_KEY(6);
	GETSET_KEY(7);
	GETSET_KEY(8);
	GETSET_KEY(9);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR szCmdParam, int iCmdShow)
{
	imr::load_data = [](const std::string& path)
	{
		std::ifstream is = {};
		is.open(path, std::ios::binary);
		is.seekg(0, std::ios::end);
		auto length = is.tellg();
		is.seekg(0, std::ios::beg);
		std::vector<char> ret(length);
		is.read(ret.data(), length);
		is.close();
		return ret;
	};

	// create window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	const char* glsl_version = "#version 300 es";
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	std::string title = (char*)std::u8string(u8"imr engine v").c_str();
	title += get_version();
	GLFWwindow* window = glfwCreateWindow(1280, 720, title.c_str(), NULL, NULL);
	if (window == NULL)
		return 1;
	glfwMakeContextCurrent(window);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
	glfwSwapInterval(1);

	{
		auto res = imr::initialize();
		if (failed(res))
		{
			std::cout << res.msg << std::endl;
			assert(false);
		}
		res = imr::spine::initialize();
		if (failed(res))
		{
			std::cout << res.msg << std::endl;
			assert(false);
		}
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();
	{
		ImGuiStyle& style = ImGui::GetStyle();

		style.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.83f);
		style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1f, 0.1f, 0.10f, 0.83f);
		style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1f, 0.1f, 0.1f, 0.20f);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}
	}

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);

	ImFont* font = io.Fonts->AddFontFromFileTTF("resources/font/neodgm.ttf", 16.0f, NULL, io.Fonts->GetGlyphRangesKorean());
	IM_ASSERT(font != NULL);

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	struct context : imr::game::Igame_context
	{
	};
	auto ctx = std::make_shared<context>();
	imr::game::scene_manager* smr = new imr::game::scene_manager(ctx);
	smr->on_device_reset();
	smr->change_scene(std::make_shared<test_scene>());

	static ImVec2 resolution = { 1280, 720 };
	smr->on_resolution_changed({ (int)resolution.x, (int)resolution.y });
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		poll_keyboard_input(window);

		smr->on_update();

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		smr->on_ui();
		ImGui::PopStyleVar();

		{
			if (ImGui::Begin("memory pool"))
			{
				static bool only_using = false;
				ImGui::Checkbox("only using", &only_using);
				auto& pools = ctx->pools();
				for (auto& p : pools)
				{
					auto use = p.second->get_use_count();
					if (only_using && use == 0)
						continue;
					auto free = p.second->get_free_count();
					ImGui::Text("%s - %d/%d", p.first.name(), use, free);
				}
			}
			ImGui::End();

			if (ImGui::Begin((char*)u8"debug"))
			{
				const char* scenes[3] = { "spine & box2d test", "performance test", "text rendering test" };
				static int selected_idx = 0;
				if (ImGui::BeginCombo("scene", scenes[selected_idx]))
				{
					for (int i = 0; i < 3; ++i)
					{
						if (ImGui::Selectable(scenes[i]))
						{
							selected_idx = i;
							if (selected_idx == 0)
							{
								smr->change_scene(std::make_shared<test_scene>());
							}
							else if (selected_idx == 1)
							{
								smr->change_scene(std::make_shared<test_scene2>());
							}
							else if (selected_idx == 2)
							{
								smr->change_scene(std::make_shared<font_test_scene>());
							}
						}
					}
					ImGui::EndCombo();
				}

				if (glfwJoystickPresent(GLFW_JOYSTICK_1))
				{
					if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1))
					{
						int count;
						auto* axises = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &count);
						for (int i = 0; i < count; ++i)
						{
							float axis = *(axises + i);
							ImGui::TextColored(ImVec4(0, 255, 100, 255), "axies %d - %f", i, axis);
						}

						GLFWgamepadstate state;
						if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state))
						{
							int count = sizeof(state.buttons) / sizeof(unsigned char);
							for (int i = 0; i < count; ++i)
							{
								auto btn = state.buttons[i];
								if (btn == GLFW_PRESS)
								{
									ImGui::TextColored(ImVec4(0, 255, 0, 255), "pressed %c", btn);
								}
								else
								{
									ImGui::TextColored(ImVec4(255, 0, 0, 255), "release%c", btn);
								}
							}
							if (state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB])
							{
								ImGui::TextColored(ImVec4(0, 0, 255, 255), "rt");
							}
						}
					}
				}

				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			}
			ImGui::End();
		}

		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		imr::int2 display_size = { display_w, display_h };
		imr::frame_buffer_texture_info game_texture(smr->get_frame_buffer());
		imr::frame_backbuffer backbuffer{};
		backbuffer.create(display_w, display_h);

		// render ingame
		if (imr::succeed(imr::camera::begin({
			.frame_buffer = &backbuffer,
			})))
		{
			imr::camera::clear({});
			imr::camera::camera({});
			imr::sprite::draw_single(&game_texture, {}, display_size / game_texture.size(), {}, { 1, 1, 1, 1 }, { 0.5f, 0.5f });
			imr::camera::end();
		}

		// render ui
		glViewport(0, 0, display_w, display_h);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);
	}

	{
		smr->on_device_lost();
		delete smr;
		imr::spine::deinitialize();
		imr::deinitialize();
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
