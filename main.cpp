//Mit dem Angriff Steiners wird alles in Ordnung sein!
#include <Windows.h>
#include <thread>
#include <iostream>
#include "Vulkan Engine.h"

#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

//#define FULLSCREEN


static bool vulkanInited = false;
static VulkanEngine *engine = NULL;
static const char g_szClassName[] = "Vulkan Test Window Class";
static HWND windowHandle = NULL;
static VulkanEngine **pUnstableInstance = new VulkanEngine *;
static bool quitMessagePosted = false;
static int lastRotationPosX = -1;
static int lastRotationPosY = -1;
static int lastTranslationPosX = -1;
static int lastTranslationPosY = -1;
static float rotationSpeed = 10.0f;
static float panSpeed = 1.0f;

void deleteEngineOrUnstableEngine() {
	if (*pUnstableInstance != NULL)
		delete *pUnstableInstance;
	else
		delete engine;
}

BOOL WINAPI closeHandler(DWORD dwCtrlType) {
	if (!engine->terminating) {
		if (dwCtrlType == CTRL_CLOSE_EVENT) {
			engine->terminating = true;

			deleteEngineOrUnstableEngine();

			quitMessagePosted = true;
		}
	}

	return TRUE;
}

bool initVulkanReal(HINSTANCE hInstance, HWND windowHandle) {
	try {
		engine = new VulkanEngine(hInstance, windowHandle, pUnstableInstance);
		VulkanEngine::calculateViewProjection(engine);

		if (engine != NULL)
			*pUnstableInstance = NULL;
	}
	catch (VulkanException e) {
		std::cerr << e.what();

		getchar();

		delete *pUnstableInstance;

		return false;
	}

	return true;
}

bool initVulkan(HINSTANCE hInstance, HWND windowHandle) {
	return (vulkanInited = initVulkanReal(hInstance, windowHandle));
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	short wheelDelta = 0;

	switch (msg) {
	case WM_SHOWWINDOW:
		//#ifndef _ENTBUG
		//		//LoadLibrary("DevIL_debug.dll");
		//#else
		//		//LoadLibrary("DevIL.dll");
		//#endif // DEBUG
		ilInit();

		std::cout << "DevIL library inited." << std::endl;

		if (!vulkanInited && !initVulkan(GetModuleHandle(NULL), windowHandle))
			return -1;
		else
			return DefWindowProc(hwnd, msg, wParam, lParam);
	case WM_CLOSE:
		if (!engine->terminating) {
			engine->terminating = true;
			deleteEngineOrUnstableEngine();
			DestroyWindow(hwnd);
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		quitMessagePosted = true;
		break;
	case WM_MOUSEWHEEL:
		wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
		engine->focusDistance += float(wheelDelta) / 100.0f;
		VulkanEngine::calculateViewProjection(engine);
		break;
	case WM_MBUTTONDOWN:
		lastTranslationPosX = -1;
		lastTranslationPosY = -1;
		break;
	case WM_LBUTTONDOWN:
		lastRotationPosX = -1;
		lastRotationPosY = -1;
		break;
	case WM_MOUSELEAVE:
		lastRotationPosX = -1;
		lastRotationPosY = -1;
		lastTranslationPosX = -1;
		lastTranslationPosY = -1;
		break;
	case WM_MOUSEMOVE:
		if ((wParam & MK_LBUTTON) != 0) {
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			if (lastRotationPosX == -1 || lastRotationPosY == -1) {
				lastRotationPosX = xPos;
				lastRotationPosY = yPos;
			}
			int deltaX = xPos - lastRotationPosX;
			int deltaY = yPos - lastRotationPosY;

			lastRotationPosX = xPos;
			lastRotationPosY = yPos;

			engine->focusYaw -= float(deltaX) / 400.0f * rotationSpeed;
			engine->focusPitch += float(deltaY) / 400.0f * rotationSpeed;

			engine->focusPitch = std::max(-89.9f, std::min(engine->focusPitch, 89.9f));

			VulkanEngine::calculateViewProjection(engine);
		}
		else if ((wParam & MK_MBUTTON) != 0) {
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			if (lastTranslationPosX == -1 || lastTranslationPosY == -1) {
				lastTranslationPosX = xPos;
				lastTranslationPosY = yPos;
			}
			int deltaX = xPos - lastTranslationPosX;
			int deltaY = yPos - lastTranslationPosY;

			lastTranslationPosX = xPos;
			lastTranslationPosY = yPos;


			engine->focusPointX -= float(deltaX) / 1000.0f * panSpeed;
			engine->focusPointY -= float(deltaY) / 1000.0f * panSpeed;

			VulkanEngine::calculateViewProjection(engine);
		}
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

void renderLoop() {
	while (!engine->terminating) {
		if (engine->isInited())
			engine->draw();
	}
}

void initWindow(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	//    extractResources();

	WNDCLASSEX wc;
	MSG msg;
#ifdef FULLSCREEN
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
		throw std::exception();

	windowHandle = CreateWindowEx(
		WS_EX_TOPMOST,
		g_szClassName,
		"VulkanEngine Test",
		WS_POPUP,
		0, 0, 1920, 1080,
		NULL, NULL, hInstance, NULL);

	ShowWindow(windowHandle, nCmdShow);
	UpdateWindow(windowHandle);

	DEVMODE screen;
	memset(&screen, 0, sizeof(screen));
	screen.dmSize = sizeof(screen);
	screen.dmPelsWidth = 1920;
	screen.dmPelsHeight = 1080;
	screen.dmBitsPerPel = 32;
	screen.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	ChangeDisplaySettings(&screen, CDS_FULLSCREEN);

	ShowCursor(false);
#else
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
		throw std::exception();

	windowHandle = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		"Vulkan Test",
		WS_OVERLAPPEDWINDOW ^ (WS_THICKFRAME | WS_MAXIMIZEBOX),
		CW_USEDEFAULT, CW_USEDEFAULT, 1100, 1000,
		NULL, NULL, hInstance, NULL);

	ShowWindow(windowHandle, nCmdShow);
	UpdateWindow(windowHandle);
#endif

	if (windowHandle == NULL)
		throw std::exception();


	std::thread renderThread(renderLoop);


	while (!quitMessagePosted) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT) {
				quitMessagePosted = true;
				engine->terminating = true;
				break;
			}
		}
	}

	return renderThread.join();
}



int main() {
	SetConsoleCtrlHandler(closeHandler, TRUE);

	//Consequently, The first WM_SHOWWINDOW message starts the VulkanEngine initialization.
	//initWindow(GetModuleHandle(NULL), NULL, "", 1);
	initWindow(GetModuleHandle(NULL), NULL, (LPSTR) "", 1);

	return EXIT_SUCCESS;
}