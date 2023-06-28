#pragma once

#include <string>
#include <vector>

void* iniReaderInstantiate(const char* fileName);
void iniReaderDestroy(void* iniReader);
bool iniReaderGetBool(void* iniReader, const char* section, const char* name);
int iniReaderGetInt(void* iniReader, const char* section, const char* name);
float iniReaderGetFloat(void* iniReader, const char* section, const char* name);
std::string iniReaderGetString(void* iniReader, const char* section, const char* name);
std::string iniReaderGetStringDefault(void* iniReader, const char* section, const char* name, const char* defaultValue);
float iniReaderGetFloatDefault(void* iniReader, const char* section, const char* name, float defaultValue);
std::vector<float> iniReaderGetFloatVector(void* iniReader, const char* section, const char* name);

struct InitConfiguration {
	bool fullscreen;
	int windowResolutionX;
	int windowResolutionY;
	bool vsync;
	float verticalFOV;
	float zNear;
	float zFar;
	float distanceCamera;
	float speedAutoRotation;
	float speedZoom;
	int texDimension;
	std::string meshFileName;
	std::string resourceDirName;
	std::vector<float> clearColor;
	bool keepConsoleOpen;
	bool recompileShaders;

	InitConfiguration(const char* configFileName) {
		void* iniReader = iniReaderInstantiate(configFileName);

		fullscreen = iniReaderGetBool(iniReader, "Presentaion", "Fullscreen");
		windowResolutionX = iniReaderGetInt(iniReader, "Presentaion", "WindowResolutionX");
		windowResolutionY = iniReaderGetInt(iniReader, "Presentaion", "WindowResolutionY");
		vsync = iniReaderGetBool(iniReader, "Presentaion", "VSync");
		verticalFOV = iniReaderGetFloat(iniReader, "Camera", "VFOV");
		zNear = iniReaderGetFloat(iniReader, "Camera", "ZNear");
		zFar = iniReaderGetFloat(iniReader, "Camera", "ZFar");
		distanceCamera = iniReaderGetFloat(iniReader, "Camera", "CameraDistance");
		speedAutoRotation = iniReaderGetFloat(iniReader, "Interaction", "AutoRotationSpeed");
		speedZoom = iniReaderGetFloat(iniReader, "Interaction", "ZoomSpeed");
		texDimension = iniReaderGetInt(iniReader, "Resource", "TexturesDimension");
		meshFileName = iniReaderGetStringDefault(iniReader, "Resource", "MeshFileNameOverride", "mesh");
		resourceDirName = iniReaderGetStringDefault(iniReader, "Resource", "ResourceDirNameOverride", "Resources");
		clearColor = iniReaderGetFloatVector(iniReader, "Misc", "ClearColor");
		keepConsoleOpen = iniReaderGetBool(iniReader, "Misc", "KeepConsoleOpen");
		recompileShaders = iniReaderGetBool(iniReader, "Misc", "InvalidateShadersCache");

		iniReaderDestroy(iniReader);
	}
};
