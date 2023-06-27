//Mit dem Angriff Steiners wird alles in Ordnung sein!
#include <Windows.h>
#include <thread>
#include <iostream>
#include "Vulkan Engine.h"
#include "initconfig.h"
#include <mutex>

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

//#define _MAKE_WINDOW_NAME_GET_RESOLUTION_WIDTH() WINDOW_RESOLUTION_WIDTH
//#define _MAKE_WINDOW_NAME_GET_RESOLUTION_HEIGHT() WINDOW_RESOLUTION_HEIGHT
//#define _MAKE_WINDOW_NAME_GET_NAME() APP_WINDOW_NAME
//#ifdef FULLSCREEN
//#define _MAKE_WINDOW_NAME() _MAKE_WINDOW_NAME_GET_NAME()\x20 Fullscreen
//#else
//#define _MAKE_WINDOW_NAME() _MAKE_WINDOW_NAME_GET_NAME()\x20 _MAKE_WINDOW_NAME_GET_RESOLUTION_WIDTH()\x78 _MAKE_WINDOW_NAME_GET_RESOLUTION_HEIGHT()
//#endif
//#define __MAKE_WINDOW_NAME_STRINGIFY(x) #x
//#define _MAKE_WINDOW_NAME_STRINGIFY(x) __MAKE_WINDOW_NAME_STRINGIFY(x)
//#define MAKE_WINDOW_NAME() _MAKE_WINDOW_NAME_STRINGIFY(_MAKE_WINDOW_NAME())

//static int lastRotationPosX = -1;
//static int lastRotationPosY = -1;
//static int lastTranslationPosX = -1;
//static int lastTranslationPosY = -1;
//static float rotationSpeed = 10.0f;
//static float panSpeed = 1.0f;
//static float wheelZoomSpeed = .0001f;
//static float autoRotationSpeed = 0.000001f;
static bool autoRotationEnabled = true;


//BOOL WINAPI closeHandler(DWORD dwCtrlType) {
//	if (!engine->terminating) {
//		if (dwCtrlType == CTRL_CLOSE_EVENT) {
//			engine->terminating = true;
//
//			deleteEngineOrUnstableEngine();
//
//			quitMessagePosted = true;
//		}
//	}
//
//	return TRUE;
//}

struct SharedData {
	inline HWND get_wndHWND() {
		lock.lock();
		HWND val = this->wndHWND;
		lock.unlock();
		return val;
	}
	inline bool get_terminating() {
		lock.lock();
		bool val = this->terminating;
		lock.unlock();
		return val;
	}

	inline void set_wndHWND(HWND val) {
		lock.lock();
		this->wndHWND = val;
		lock.unlock();
	}
	inline void set_terminating(bool val) {
		lock.lock();
		this->terminating = val;
		lock.unlock();
	}

	InitConfiguration* initConfig = nullptr;
	HINSTANCE hInstance = NULL;
private:
	HWND wndHWND = NULL;
	bool terminating = false;

	std::mutex lock;
};


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	//short wheelDelta = 0;//TODO DO THIS
	//float appliedWheelData = 1.0f;
	SharedData* sharedData = nullptr;

	if (msg == WM_CREATE/* || msg == WM_NCCREATE*/)
	{
		CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
		sharedData = reinterpret_cast<SharedData*>(pCreate->lpCreateParams);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)sharedData);
	}
	else
	{
		LONG_PTR ptr = GetWindowLongPtr(hwnd, GWLP_USERDATA);
		sharedData = reinterpret_cast<SharedData*>(ptr);
	}

	switch (msg) {
	case WM_CLOSE:
		sharedData->set_terminating(true);
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
		/*case WM_MOUSEWHEEL:
			wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);

			appliedWheelData = 1.0f - float(wheelDelta) * initConfig->speedZoom;

			engine->cameraDistance *= appliedWheelData;

			VulkanEngine::calculateViewProjection(engine);
			break;*/
			//case WM_MBUTTONDOWN:
			//	lastTranslationPosX = -1;
			//	lastTranslationPosY = -1;
			//	break;
			//case WM_LBUTTONDOWN:
			//	lastRotationPosX = -1;
			//	lastRotationPosY = -1;
			//	break;
			//case WM_MOUSELEAVE:
			//	lastRotationPosX = -1;
			//	lastRotationPosY = -1;
			//	lastTranslationPosX = -1;
			//	lastTranslationPosY = -1;
			//	break;
			//case WM_MOUSEMOVE:
			//	if ((wParam & MK_LBUTTON) != 0) {
			//		int xPos = GET_X_LPARAM(lParam);
			//		int yPos = GET_Y_LPARAM(lParam);
			//		if (lastRotationPosX == -1 || lastRotationPosY == -1) {
			//			lastRotationPosX = xPos;
			//			lastRotationPosY = yPos;
			//		}
			//		int deltaX = xPos - lastRotationPosX;
			//		int deltaY = yPos - lastRotationPosY;

			//		lastRotationPosX = xPos;
			//		lastRotationPosY = yPos;

			//		engine->focusYaw -= float(deltaX) / 400.0f * rotationSpeed;
			//		engine->focusPitch += float(deltaY) / 400.0f * rotationSpeed;

			//		engine->focusPitch = std::max(-89.9f, std::min(engine->focusPitch, 89.9f));

			//		VulkanEngine::calculateViewProjection(engine);
			//	}
			//	else if ((wParam & MK_MBUTTON) != 0) {
			//		int xPos = GET_X_LPARAM(lParam);
			//		int yPos = GET_Y_LPARAM(lParam);
			//		if (lastTranslationPosX == -1 || lastTranslationPosY == -1) {
			//			lastTranslationPosX = xPos;
			//			lastTranslationPosY = yPos;
			//		}
			//		int deltaX = xPos - lastTranslationPosX;
			//		int deltaY = yPos - lastTranslationPosY;

			//		lastTranslationPosX = xPos;
			//		lastTranslationPosY = yPos;


			//		engine->focusPointX -= float(deltaX) / 1000.0f * panSpeed;
			//		engine->focusPointY -= float(deltaY) / 1000.0f * panSpeed;

			//		VulkanEngine::calculateViewProjection(engine);
			//	}
			//	break;
	case WM_KEYDOWN:
		if (wParam == 32) autoRotationEnabled = !autoRotationEnabled;
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return 0;
}

//extern uint8_t data[]   asm("_binary_Resources_0_png_start");
//extern uint8_t size[]   asm("_binary_Resources_0_png_size");
//extern uint8_t end[]    asm("_binary_Resources_0_png_end");

//void extractResources() {
//    size_t realSize = (size_t)((void *)size);
//
//    FILE *file;
//
//    file = fopen("0.png", "wb");
//    fwrite(data, 1, realSize, file); //64 Bit only
//    fclose(file);
//}

HWND initWindow(SharedData* sharedData) {
	HWND windowHandle = NULL;

	static const char windowClassName[] = "Vulkan Test Window Class";
	static const char windowName[] = "Vulkan Test";
	WNDCLASSEX wc;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = sharedData->hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = windowClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
		return NULL;

	if (sharedData->initConfig->fullscreen) {
		int fullscreenResolutionX = GetSystemMetrics(SM_CXSCREEN);
		int fullscreenResolutionY = GetSystemMetrics(SM_CYSCREEN);

		windowHandle = CreateWindowEx(
			0,
			windowClassName,
			windowName,
			WS_POPUP,
			0, 0, fullscreenResolutionX, fullscreenResolutionY,
			NULL, NULL, sharedData->hInstance, sharedData);
	}
	else {
		DWORD dwStyle = WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX);
		DWORD dwExStyle = WS_EX_CLIENTEDGE;

		RECT windowRect{ 0,0,sharedData->initConfig->windowResolutionX, sharedData->initConfig->windowResolutionY };

		AdjustWindowRectEx(&windowRect, dwStyle, FALSE, dwExStyle);

		windowHandle = CreateWindowEx(
			dwExStyle,
			windowClassName,
			windowName,
			dwStyle,
			CW_USEDEFAULT, CW_USEDEFAULT,
			windowRect.right - windowRect.left, windowRect.bottom - windowRect.top,
			NULL, NULL, sharedData->hInstance, sharedData);
	}

	return windowHandle;
}

unsigned __stdcall windowThreadProc(void* data) {
	SharedData* sharedData = reinterpret_cast<SharedData*>(data);

	bool fullscreen = sharedData->initConfig->fullscreen;

	HWND hwnd = initWindow(sharedData);

	if (hwnd != NULL) {
		ShowWindow(hwnd, SW_SHOWNORMAL);
		UpdateWindow(hwnd);

		if (fullscreen) {
			int fullscreenResolutionX = GetSystemMetrics(SM_CXSCREEN);
			int fullscreenResolutionY = GetSystemMetrics(SM_CYSCREEN);

			DEVMODE screen;
			memset(&screen, 0, sizeof(screen));
			screen.dmSize = sizeof(screen);
			screen.dmPelsWidth = fullscreenResolutionX;
			screen.dmPelsHeight = fullscreenResolutionY;
			screen.dmBitsPerPel = 32;
			screen.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			ChangeDisplaySettings(&screen, CDS_FULLSCREEN);

			ShowCursor(false);
		}

		sharedData->set_wndHWND(hwnd); //todo: maybe I should supply hwnd to shareddata later (eg. after or inside first WM_CREATE)?

		MSG msg = {};
		while (GetMessage(&msg, NULL, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	else
	{
		std::cerr << "Couldn't create Window." << std::endl;
		sharedData->set_terminating(true);
	}

	return 0;
}

unsigned __stdcall engineThreadProc(void* data) {
	SharedData* sharedData = reinterpret_cast<SharedData*>(data);

	VulkanEngine* engine = nullptr;

	while (!sharedData->get_terminating() && sharedData->get_wndHWND() == NULL) {}

	if (sharedData->get_terminating() || sharedData->get_wndHWND() == NULL)
		return 0;

	try {
		ilInit();
		std::cout << "DevIL library inited." << std::endl;

		engine = new VulkanEngine(sharedData->hInstance, sharedData->get_wndHWND(), sharedData->initConfig);
		engine->init();
		engine->calculateViewProjection();
	}
	catch (VulkanException e) {
		std::cerr << e.what();
		sharedData->set_terminating(true);
	}

	while (engine != nullptr && !sharedData->get_terminating()) {
		try {
			if (autoRotationEnabled) engine->cameraRotate();
			engine->draw();
		}
		catch (VulkanException e) {
			std::cerr << e.what();
			sharedData->set_terminating(true);
			break;
		}
	}

	if (engine != nullptr)
		delete engine;

	return 0;
}

int main() {
	//SetConsoleCtrlHandler(closeHandler, TRUE); //TODO handle this

	SharedData sharedData;

	try {
		sharedData.initConfig = new InitConfiguration("settings.ini");
	}
	catch (...) {
		std::cerr << "Couldn't load settings.ini" << std::endl;

		system("pause");

		return EXIT_FAILURE;
	}

	std::cout << "Settings file successfully loaded." << std::endl;

	sharedData.hInstance = GetModuleHandle(NULL);

	//MAGIC STARTS

	HANDLE hWindowThread, hEngineThread;
	unsigned windowThreadID, engineThreadID;

	hEngineThread = (HANDLE)_beginthreadex(NULL, 0, &engineThreadProc, reinterpret_cast<void*>(&sharedData), 0, &engineThreadID);
	hWindowThread = (HANDLE)_beginthreadex(NULL, 0, &windowThreadProc, reinterpret_cast<void*>(&sharedData), 0, &windowThreadID);

	WaitForSingleObject(hEngineThread, INFINITE);
	WaitForSingleObject(hWindowThread, INFINITE);
	CloseHandle(hEngineThread);
	CloseHandle(hWindowThread);

	//MAGIC ENDS

	delete sharedData.initConfig;

	system("pause");

	return EXIT_SUCCESS;
}