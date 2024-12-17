#include "gui.h"
#include <iostream>
#include <Windows.h>
#include <thread>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"
#include "fonts.hpp"
#include "fontawesome.h"
#include "./Util/memory.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter
);

long __stdcall WindowProcess(
	HWND window,
	UINT message,
	WPARAM wideParameter,
	LPARAM longParameter) 
{
	if (ImGui_ImplWin32_WndProcHandler(window, message, wideParameter, longParameter))
		return true;

	switch (message)
	{
	case WM_SIZE: {
		if (gui::device && wideParameter != SIZE_MINIMIZED)
		{
			gui::presentParameters.BackBufferWidth = LOWORD(longParameter);
			gui::presentParameters.BackBufferHeight = LOWORD(longParameter);
			gui::ResetDevice();
		}
	}return 0;

	case WM_SYSCOMMAND: {
		if ((wideParameter & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
	}break;

	case WM_DESTROY: {
		PostQuitMessage(0);
	}return 0;


	case WM_LBUTTONDOWN: {
		gui::position = MAKEPOINTS(longParameter); // set click point
	}return 0;

	case WM_MOUSEMOVE: {
		if (wideParameter == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(longParameter);
			auto rect = ::RECT{ };

			GetWindowRect(gui::window, &rect);

			rect.left += points.x - gui::position.x;
			rect.top += points.y - gui::position.y;

			if (gui::position.x >= 0 &&
				gui::position.x <= gui::WIDTH &&
				gui::position.y >= 0 && gui::position.y <= 19)
				SetWindowPos(
					gui::window,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}
	}return 0;

	}

	return DefWindowProcW(window, message, wideParameter, longParameter);
}

void gui::CreateHWindow(
	const char* windowName,
	const char* className) noexcept
{
	windowClass.cbSize = sizeof(WNDCLASSEXA);
	windowClass.style = CS_CLASSDC;
	windowClass.lpfnWndProc = WindowProcess;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = GetModuleHandleA(0);
	windowClass.hIcon = 0;
	windowClass.hCursor = 0;
	windowClass.hbrBackground = 0;
	windowClass.lpszMenuName = 0;
	windowClass.lpszClassName = className;
	windowClass.hIconSm = 0;

	RegisterClassExA(&windowClass);

	window = CreateWindowA(
		className,
		windowName,
		WS_POPUP,
		100,
		100,
		WIDTH,
		HEIGHT,
		0,
		0,
		windowClass.hInstance,
		0
	);

	ShowWindow(window, SW_SHOWDEFAULT);
	UpdateWindow(window);
}

void gui::DestroyHWindow() noexcept
{
	DestroyWindow(window);
	UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool gui::CreateDevice() noexcept
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!d3d)
		return false;

	ZeroMemory(&presentParameters, sizeof(presentParameters));

	presentParameters.Windowed = TRUE;
	presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
	presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
	presentParameters.EnableAutoDepthStencil = TRUE;
	presentParameters.AutoDepthStencilFormat = D3DFMT_D16;
	presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		window,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&presentParameters,
		&device) < 0)
		return false;

	return true;
}

void gui::ResetDevice() noexcept
{
	ImGui_ImplDX9_InvalidateDeviceObjects();

	const auto result = device->Reset(&presentParameters);

	if (result == D3DERR_INVALIDCALL)
		IM_ASSERT(0);

	ImGui_ImplDX9_CreateDeviceObjects();
}

void gui::DestroyDevice() noexcept
{
	if (device)
	{
		device->Release();
		device = nullptr;
	}

	if (d3d)
	{
		d3d->Release();
		d3d = nullptr;
	}
}

inline ImFont* icons_font = nullptr;

void gui::CreateImGui() noexcept
{

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ::ImGui::GetIO();

	io.IniFilename = NULL;

	// style

	ImGuiStyle& style = ImGui::GetStyle();
	auto& colors = style.Colors;

	io.Fonts->AddFontFromMemoryTTF(font_rubik, sizeof(font_rubik), 22.0f);

	static const ImWchar icon_ranges[]{ 0xf000, 0xf3ff, 0 };
	ImFontConfig icons_config;
	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	icons_config.OversampleH = 3;
	icons_config.OversampleV = 3;

	icons_font = io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_data, font_awesome_size, 19.5f, &icons_config, icon_ranges);

	style.ScrollbarRounding = 0;
	style.WindowRounding = 4.0f;
	style.FramePadding = ImVec2(20, 8);
	style.ScrollbarRounding = 5.0f;
	style.GrabRounding = 5.0f;
	style.WindowTitleAlign = ImVec2(0.49f, 0.5f);

	colors[ImGuiCol_ResizeGrip] = ImColor(0, 0, 0, 0);
	colors[ImGuiCol_ResizeGripActive] = ImColor(0, 0, 0, 0);
	colors[ImGuiCol_ResizeGripHovered] = ImColor(0, 0, 0, 0);

	colors[ImGuiCol_Button] = ImColor(18, 18, 18, 100);
	colors[ImGuiCol_ButtonActive] = ImColor(21, 21, 21, 100);
	colors[ImGuiCol_ButtonHovered] = ImColor(21, 21, 21, 100);

	colors[ImGuiCol_FrameBg] = ImColor(24, 24, 24);
	colors[ImGuiCol_FrameBgActive] = ImColor(26, 26, 26);
	colors[ImGuiCol_FrameBgHovered] = ImColor(26, 26, 26);

	colors[ImGuiCol_TitleBgActive] = ImColor(100, 0, 148, 100);

	colors[ImGuiCol_Tab] = ImColor(173, 0, 255, 100);
	colors[ImGuiCol_TabActive] = ImColor(173, 0, 255, 100);
	colors[ImGuiCol_TabHovered] = ImColor(138, 0, 203, 100);
	colors[ImGuiCol_TabSelectedOverline] = ImColor(138, 0, 203, 100);

	colors[ImGuiCol_CheckMark] = ImColor(173, 0, 255, 100);

	colors[ImGuiCol_SliderGrab] = ImColor(173, 0, 255, 100);
	colors[ImGuiCol_SliderGrabActive] = ImColor(138, 0, 203, 100);

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::DestroyImGui() noexcept
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void gui::BeginRender() noexcept
{
	MSG message;
	while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void gui::EndRender() noexcept
{
	ImGui::EndFrame();

	device->SetRenderState(D3DRS_ZENABLE, FALSE);
	device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

	device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

	if (device->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		device->EndScene();
	}

	const auto result = device->Present(0, 0, 0, 0);

	if (result == D3DERR_DEVICELOST && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

void HideWindowFromTaskbar(HWND window)
{
	// Get the current extended style of the window
	LONG_PTR exStyle = GetWindowLongPtr(window, GWL_EXSTYLE);

	// Modify the style to hide it from the taskbar
	exStyle &= ~WS_EX_APPWINDOW; // Remove the APPWINDOW style
	exStyle |= WS_EX_TOOLWINDOW; // Add the TOOLWINDOW style

	// Apply the updated style
	SetWindowLongPtr(window, GWL_EXSTYLE, exStyle);

	// Ensure the window style changes take effect
	ShowWindow(window, SW_HIDE);  // Hide the window
	ShowWindow(window, SW_SHOW);  // Show the window again
}


void gui::Render() noexcept
{
	HWND hwnd = FindWindowA(NULL, "Minecraft");

	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize({ WIDTH, HEIGHT });
	ImGui::Begin(
		"PRIME V1.exe",
		&exit,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoMove
	);

	if (!hwnd == NULL) {

		const auto mem = Memory("Minecraft.Windows.exe");

		static bool hide = false;

		// enlever afficher la fenetre
		if (GetAsyncKeyState(VK_F10) & 1) {
			if (window) {
				if (hide == false) {
					LONG style = GetWindowLong(window, GWL_EXSTYLE);
					style |= WS_EX_TOOLWINDOW;
					SetWindowLong(window, GWL_EXSTYLE, style);

					ShowWindow(window, SW_HIDE);
					hide = true;
				}
				else {
					LONG style = GetWindowLong(window, GWL_EXSTYLE);
					style &= ~WS_EX_TOOLWINDOW;
					SetWindowLong(window, GWL_EXSTYLE, style);

					ShowWindow(window, SW_SHOW);
					hide = false;
				}
			}

		}

		if (ImGui::BeginTabBar("tabs")) {
			if (ImGui::BeginTabItem(ICON_FA_CROSSHAIRS" AutoClicker")) {
				ImGui::PushFont(icons_font);
				ImGui::PopFont();

				ImGui::Text("Press F4 to enable, disable and F10 to Hide.");
				ImGui::Columns(1);

				static int cps = 10;
				ImGui::SliderInt("CPS", &cps, 0, 30);

				static bool toggle = false;

				ImGui::Checkbox("Enable", &toggle);

				static auto lastClickTime = std::chrono::high_resolution_clock::now();

				// activer desactiver l'autoclick avec button
				if (toggle) {
					auto now = std::chrono::high_resolution_clock::now();
					auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastClickTime).count();

					if (elapsedTime >= (1000 / cps)) {
						if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
							mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
							mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
						}
						lastClickTime = now;
					}
				}

				// activer desactiver l'autoclick avec la touche
				if (GetAsyncKeyState(VK_F4) & 1) {
					if (toggle == false) {
						toggle = true;
					}
					else {
						toggle = false;
					}
				}

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(ICON_FA_COGS" Player")) {
				static float reach = 3.0;

				ImGui::SliderFloat("Reach", &reach, 3.0f, 7.0f);

				static float velocity = 1.0;
				ImGui::SliderFloat("Velocity Linear", &velocity, 1.0f, 0.0f);

				static float velocity2 = 1.0;
				ImGui::SliderFloat("Velocity Height", &velocity2, 1.0f, 0.0f);

				static float timer = 1.0;
				ImGui::SliderFloat("Timer", &timer, 1.0f, 2.0f);

				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem(ICON_FA_COG" Settings")) {
				ImGui::Button("RGB");

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	else {
		ImGui::Text("Minecraft not found...");
	}
	ImGui::End();
}