//
// Created by chakmeshma on 20.11.2017.
//

#ifndef VULKAN_TEST_VULKAN_ENGINE_H
#define VULKAN_TEST_VULKAN_ENGINE_H

#pragma once

#define VK_USE_PLATFORM_WIN32_KHR

#ifdef _DEBUG
#define _ENTBUG
#endif

#ifdef _ENTBUG
#define VKASSERT_SUCCESS(i) assert(i == VK_SUCCESS)
#define ASSERT(i) assert(i)
#else
#define VKASSERT_SUCCESS(i) i
#define ASSERT(i) i
#endif

#define PRINT_ENUMERATIONS

#ifdef PRINT_ENUMERATIONS
#define PRINT_INSTANCE_EXTENSIONS
#define PRINT_DEVICE_EXTENSIONS
#define PRINT_INSTANCE_LAYERS
#define PRINT_DEVICE_LAYERS
#define PRINT_MEMORY_TYPES
#define PRINT_QUEUE_FAMILIES
#endif


#include <vulkan/vulkan.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <math.h>
#include <IL/il.h>
#include <assert.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/cimport.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/quaternion.hpp>
#include "Vulkan Engine Exception.h"
#include "shaderc_online_compiler.h"
#include "initconfig.h"

using namespace glm;

//typedef const C_STRUCT aiScene *FNP_aiImportFile(const char *, unsigned int);

template<class T>
struct Attribute {
	T position[3];
	T normal[3];
	T uv[2];
	T tangent[3];
	T bitangent[3];
};

template<class T>
struct ModelMatrix {
	mat4x4 modelMatrix;
};

template<class T>
struct ViewProjectionMatrices {
	mat4x4 viewMatrix;
	mat4x4 projectionMatrix;
};


class VulkanEngine {
public:

	VulkanEngine(HINSTANCE hInstance, HWND windowHandle, VulkanEngine** ppUnstableInstance, const InitConfiguration* initConfig);

	~VulkanEngine() noexcept(false);

	bool isInited();

	void getInstanceExtensions();

	void getDeviceExtensions();

	static std::string getVersionString(uint32_t versionBitmask);

	void draw();

	bool terminating = false;

	float cameraDistance;
	float fovAngle;
	float zNear;
	float zFar;
	uint16_t textureDim;
	bool verticalSyncEnabled;
	std::string meshFileName;

	float cameraRotationValue = 0.0f;

	ViewProjectionMatrices<float> viewProjection = {};
	ModelMatrix<float> modelMatrix = {};


	static void calculateViewProjection(VulkanEngine* instance) {

		mat4x4 rotationMatrix = glm::mat4(1.0f);

		rotationMatrix = glm::rotate(rotationMatrix, glm::radians(instance->focusYaw), glm::vec3(0.0f, 1.0f, 0.0f));
		rotationMatrix = glm::rotate(rotationMatrix, glm::radians(instance->focusPitch), glm::vec3(1.0f, 0.0f, 0.0f));

		glm::vec4 cameraPosition = /*rotationMatrix **/ glm::vec4(0.0, 0.0, -instance->cameraDistance, 1.0);

		glm::vec3 focusPoint(instance->focusPointX, instance->focusPointY, instance->focusPointZ);

		instance->viewProjection.viewMatrix = glm::lookAt(glm::vec3(cameraPosition), focusPoint,
			glm::vec3(0.0f, 1.0f, 0.0f));

		static const glm::vec3 cameraRotationAxis(.0f, 1.0f, .0f);

		instance->viewProjection.viewMatrix = glm::rotate(instance->viewProjection.viewMatrix, instance->cameraRotationValue, cameraRotationAxis);


		float frameBufferAspectRatio =
			((float)instance->swapchainCreateInfo.imageExtent.width) /
			((float)instance->swapchainCreateInfo.imageExtent.height);


		instance->viewProjection.projectionMatrix = glm::perspective(glm::radians(instance->fovAngle),
			frameBufferAspectRatio, instance->zNear,
			instance->zFar);
	}

	float getElapsedTime();

	float focusPitch = 0.0f;
	float focusYaw = 0.0f;
	//    float initialModelScale = 1.0f;
	//    float initialModelXRotation = 0.0;
	//    float initialModelYRotation = 0.0f;
	float focusPointX = 0.0f;
	float focusPointY = 0.0f;
	float focusPointZ = 0.0f;
private:
	//static //const uint16_t MAX_DEFAULT_ARRAY_SIZE = 10;
	static const uint16_t MAX_MESHES = 30;
	//    static //const uint16_t MAX_BUFFER_ARRAY_SIZE = MAX_DEFAULT_ARRAY_SIZE;
	//    static //const uint16_t MAX_IMAGE_ARRAY_SIZE = MAX_DEFAULT_ARRAY_SIZE;
	//    static //const uint16_t MAX_IMAGE_VIEW_ARRAY_SIZE = MAX_DEFAULT_ARRAY_SIZE;
	//    static //const uint16_t MAX_DEVICE_MEMORY_ALLOCATION_ARRAY_SIZE = MAX_DEFAULT_ARRAY_SIZE;
	//    static //const uint16_t MAX_SPARSE_IMAGE_ARRAY_SIZE = MAX_DEFAULT_ARRAY_SIZE;
	//    static //const uint16_t MAX_SPARSE_IMAGE_MEMORY_REQUIREMENTS_ARRAY_SIZE = MAX_DEFAULT_ARRAY_SIZE;
	//    static //const uint16_t MAX_SPARSE_IMAGE_FORMAT_PROPERTIES_ARRAY_SIZE = MAX_DEFAULT_ARRAY_SIZE;
	static const uint16_t MAX_COLOR_TEXTURE_ARRAY_SIZE = MAX_MESHES;
	static const uint16_t MAX_NORMAL_TEXTURE_ARRAY_SIZE = MAX_COLOR_TEXTURE_ARRAY_SIZE;
	static const uint16_t MAX_SPECULAR_TEXTURE_ARRAY_SIZE = MAX_COLOR_TEXTURE_ARRAY_SIZE;
	static const uint16_t MAX_UNIFORM_BUFFER_ARRAY_SIZE = MAX_MESHES;
	static const uint16_t MAX_VERTEX_BUFFER_ARRAY_SIZE = MAX_UNIFORM_BUFFER_ARRAY_SIZE;
	static const uint16_t MAX_INDEX_BUFFER_ARRAY_SIZE = MAX_UNIFORM_BUFFER_ARRAY_SIZE;


	uint32_t instanceExtensionsCount = 0;
	VkInstance instance;
	VkDevice logicalDevices[1];
	VkDeviceQueueCreateInfo deviceQueueCreateInfos[2];
	VkInstanceCreateInfo instanceCreateInfo = {};
	VkApplicationInfo appInfo = {};
	uint32_t numberOfSupportedDevices = 1;
	VkPhysicalDevice physicalDevices[1];
	VkPhysicalDeviceProperties deviceProperties = {};
	VkPhysicalDeviceFeatures supportedDeviceFeatures = {};
	VkPhysicalDeviceFeatures desiredDeviceFeatures = {};
	uint64_t totalHeapMemorySize = 0;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties = {};
	uint32_t numQueueFamilies = 0;
	uint32_t queueCount = 0;
	uint32_t graphicsQueueFamilyIndex = -1;
	uint32_t transferQueueFamilyIndex = -1;
	uint32_t graphicsQueueFamilyNumQueue = 0;
	uint32_t transferQueueFamilyNumQueue = 0;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	VkDeviceCreateInfo logicalDeviceCreateInfo;
	uint32_t layerPropertiesCount = -1;
	std::vector<VkLayerProperties> layerProperties;
	uint32_t deviceExtensionsCount = 0;
	std::vector<VkExtensionProperties> deviceExtensions;
	std::vector<VkExtensionProperties> instanceExtensions;
	VkImageFormatProperties imageFormatProperties = {};
	VkFormatProperties formatProperties = {};
	VkImageCreateInfo imageCreateInfo = {};
	VkImage depthImage;
	VkImageViewCreateInfo imageViewCreateInfo = {};
	VkImageView depthImageView;
	uint32_t memoryTypeCount = 0;
	uint32_t hostVisibleMemoryTypeIndex = -1;
	uint32_t deviceLocalMemoryTypeIndex = -1;
	VkDeviceSize memoryCommittedBytesCount = 0;
	VkMappedMemoryRange memoryFlushRange = {};
	VkMemoryRequirements imageMemoryRequirements = {};
	VkImage* sparseImages;
	VkSparseImageMemoryRequirements* sparseImageMemoryRequirements;
	uint32_t sparseMemoryRequirementsCount = 0;
	VkImageCreateInfo sparseImageCreateInfo = {};
	VkSparseImageFormatProperties* physicalDeviceSparseImageFormatProperties;
	uint32_t physicalDeviceSparseImageFormatPropertiesCount = 0;
	VkQueue graphicsQueue;
	VkQueue transferQueue;
	HINSTANCE hInstance;
	HWND windowHandle;
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	VkBool32 physicalDeviceSurfaceSupported = VK_FALSE;
	uint32_t surfaceSupportedFormatsCount = 0;
	VkSurfaceFormatKHR* surfaceSupportedFormats;
	uint32_t surfaceSupportedPresentModesCount = 0;
	VkPresentModeKHR* surfaceSupportedPresentModes;
	VkPresentModeKHR surfacePresentMode;
	uint32_t swapchainImagesCount = 0;
	VkImage* swapchainImages;
	VkImageView* swapchainImageViews;
	VkPresentInfoKHR presentInfo = {};
	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	VkShaderModule computeShaderModule = {};
	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	VkPipeline computePipeline = {};
	VkPipeline graphicsPipeline = {};
	VkDescriptorSetLayoutCreateInfo computeDescriptorSetLayoutCreateInfo = {};
	VkDescriptorSetLayoutCreateInfo graphicsDescriptorSetLayoutCreateInfo = {};
	VkDescriptorSetLayout computeDescriptorSetLayout = {};
	VkDescriptorSetLayout graphicsDescriptorSetLayout = {};
	VkPipelineLayoutCreateInfo computePipelineLayoutCreateInfo = {};
	VkPipelineLayoutCreateInfo graphicsPipelineLayoutCreateInfo = {};
	VkPipelineLayout computePipelineLayout = {};
	VkPipelineLayout graphicsPipelineLayout = {};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	VkDescriptorPool descriptorPool = {};
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	VkRenderPassCreateInfo renderPassCreateInfo = {};
	VkRenderPass renderPass = {};
	VkFramebuffer* framebuffers;
	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	VkShaderModule graphicsVertexShaderModule;
	//VkShaderModule graphicsGeometryShaderModule;
	VkShaderModule graphicsFragmentShaderModule;
	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	VkVertexInputBindingDescription vertexBindingDescription = {};
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	VkViewport viewport = {};
	VkRect2D scissor = {};
	VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {};
	VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {};
	VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {};
	VkSemaphore waitToPresentSemaphore;
	VkSemaphore indexAcquiredSemaphore;

	VkCommandPool renderCommandPool;
	VkCommandPool transferCommandPool;
	VkCommandBuffer renderCommandBuffer;
	VkFormat surfaceImageFormat;
	VkFormat depthFormat;
	std::vector<Attribute<float>> sortedAttributes[MAX_VERTEX_BUFFER_ARRAY_SIZE];
	std::vector<uint32_t> sortedIndices[MAX_INDEX_BUFFER_ARRAY_SIZE];
	uint32_t totalUniformBufferSize = sizeof(ModelMatrix<float>);
	uint32_t indexBuffersSizes[MAX_INDEX_BUFFER_ARRAY_SIZE];
	uint32_t vertexBuffersSizes[MAX_VERTEX_BUFFER_ARRAY_SIZE];
	VkImage colorTextureImagesDevice[MAX_COLOR_TEXTURE_ARRAY_SIZE];
	VkImage colorTextureImages[MAX_COLOR_TEXTURE_ARRAY_SIZE];
	VkImageView colorTextureViews[MAX_COLOR_TEXTURE_ARRAY_SIZE];
	VkImage normalTextureImagesDevice[MAX_NORMAL_TEXTURE_ARRAY_SIZE];
	VkImage normalTextureImages[MAX_NORMAL_TEXTURE_ARRAY_SIZE];
	VkImageView normalTextureViews[MAX_NORMAL_TEXTURE_ARRAY_SIZE];
	VkImage specTextureImagesDevice[MAX_SPECULAR_TEXTURE_ARRAY_SIZE];

	VkImage specTextureImages[MAX_SPECULAR_TEXTURE_ARRAY_SIZE];
	VkImageView specTextureViews[MAX_SPECULAR_TEXTURE_ARRAY_SIZE];
	VkDeviceMemory depthImageMemory;
	VkDeviceMemory uniTexturesMemory;
	VkDeviceMemory uniBuffersMemory;
	VkDeviceMemory uniTexturesMemoryDevice;
	VkDeviceMemory uniBuffersMemoryDevice;
	VkBuffer uniformBuffersDevice[MAX_UNIFORM_BUFFER_ARRAY_SIZE];
	VkBuffer vertexBuffersDevice[MAX_VERTEX_BUFFER_ARRAY_SIZE];
	VkBuffer indexBuffersDevice[MAX_INDEX_BUFFER_ARRAY_SIZE];
	VkBuffer uniformBuffers[MAX_UNIFORM_BUFFER_ARRAY_SIZE];
	VkBuffer vertexBuffers[MAX_VERTEX_BUFFER_ARRAY_SIZE];
	VkBuffer indexBuffers[MAX_INDEX_BUFFER_ARRAY_SIZE];
	VkDeviceSize* colorTexturesBindOffsetsDevice;
	VkDeviceSize* normalTexturesBindOffsetsDevice;
	VkDeviceSize* specTexturesBindOffsetsDevice;
	VkDeviceSize* colorTexturesBindOffsets;
	VkDeviceSize* normalTexturesBindOffsets;
	VkDeviceSize* specTexturesBindOffsets;
	VkDeviceSize* uniformBuffersBindOffsetsDevice;
	VkDeviceSize* vertexBuffersBindOffsetsDevice;
	VkDeviceSize* indexBuffersBindOffsetsDevice;
	VkDeviceSize* uniformBuffersBindOffsets;
	VkDeviceSize* vertexBuffersBindOffsets;
	VkDeviceSize* indexBuffersBindOffsets;
	aiScene* cachedScene = NULL;
	VkDescriptorSet meshDescriptorSets[MAX_MESHES];
	VkSampler textureSampler;
	VkFence queueDoneFence;
	LARGE_INTEGER frequency;        // ticks per second
	LARGE_INTEGER t1, t2;           // ticks
	double elapsedTime;
	bool inited = false;


	void init();

	void createInstance();

	void getInstanceLayers();

	void getDeviceLayers();

	void enumeratePhysicalDevices();

	void getPhysicalDevicePropertiesAndFeatures();

	void terminate();

	void createLogicalDevice();

	//    void getPhysicalDeviceImageFormatProperties(VkFormat imageFormat);

	void getPhysicalDeviceSparseImageFormatProperties();

	void createAllBuffers();

	void createDepthImageAndImageview();

	void createSparseImage();

	//void allocateDeviceMemories();
	//void bindBufferMemories();
	void getQueues();

	void getQueueFamilyPresentationSupport();

	void createSurface();

	void createSwapchain();

	uint32_t acquireNextFramebufferImageIndex();

	void present(uint32_t swapchainPresentImageIndex);


	void createQueueDoneFence();

	void createRenderpass();

	void createSwapchainImageViews();

	void createFramebuffers();

	void createGraphicsShaderModule(const char* shaderFileName, VkShaderModule* shaderModule,
		shaderc_shader_kind shaderType);

	//    void createFragmentGraphicsShaderModule();
	//
	//    void createGeometryGraphicsShaderModule();

	void createGraphicsPipeline();

	void createPipelineAndDescriptorSetsLayout();

	void render(uint32_t drawableImageIndex);

	void createRenderCommandPool();

	void createTransferCommandPool();

	void createWaitToPresentSemaphore();

	void createWaitToDrawSemaphore();

	void destroySyncMeans();

	void createDescriptorSets();

	void loadMesh(const char* fileName);

	/*void writeBuffers();*/
	VkMemoryRequirements createBuffer(VkBuffer* buffer, VkDeviceSize size, VkBufferUsageFlags usageFlags);

	void getSupportedDepthFormat();

	VkMemoryRequirements createTexture(VkImage* textureImage, VkImageUsageFlags usageFlags);

	void createTextureView(VkImageView* textureImageView, VkImage textureImage);

	void createAllTextures();

	void createDescriptorPool();

	void createSyncMeans();

	void setupTimer();

	void commitBuffers();

	void commitTextures();

	void destroyStagingMeans();

	std::string loadShaderCode(const char* fileName);

	//void createGraphicsNormalViewerPipeline();

	//VkShaderModule graphicsNormalViewerVertexShaderModule;
	//VkShaderModule graphicsNormalViewerGeometryShaderModule;
	//VkShaderModule graphicsNormalViewerFragmentShaderModule;
	//VkPipeline graphicsDebugPipeline;

	std::string resourcesPath = "Resources\\";
};


#endif //VULKAN_TEST_VULKAN_ENGINE_H
