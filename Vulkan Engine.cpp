// Created by chakmeshma on 20.11.2017.

#include "Vulkan Engine.h"

std::string VulkanEngine::getCurrentPath() {
	TCHAR buffer[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, buffer, MAX_PATH);
	std::wstring::size_type pos = std::string(buffer).find_last_of("\\/");
	return (std::string(buffer).substr(0, pos) + "\\");
}

std::string VulkanEngine::makeDevilErrorText(std::string filePath) {
	std::string errorText = "DevIL: Couldn't load texture file: ";
	errorText += getCurrentPath();
	errorText += filePath;

	return errorText;
}


VulkanEngine::VulkanEngine(HINSTANCE hInstance, HWND windowHandle, const InitConfiguration* initConfig) {
	initVkObjectsNull();

	this->cameraDistance = -initConfig->distanceCamera;
	this->fovAngle = initConfig->verticalFOV;
	this->zNear = initConfig->zNear;
	this->zFar = initConfig->zFar;
	this->textureDim = initConfig->texDimension;
	this->verticalSyncEnabled = initConfig->vsync;
	this->meshFileName = initConfig->meshFileName;
	this->resourcesPath = initConfig->resourceDirName;
	this->resourcesPath.append("\\");
	this->clearColor = initConfig->clearColor;
	this->autoRotationSpeed = initConfig->speedAutoRotation;

	this->hInstance = hInstance;
	this->windowHandle = windowHandle;
}

void VulkanEngine::initVkObjectsNull() {

	NULL_HANDLE_INIT_ARRAY(colorTextureImages, MAX_COLOR_TEXTURE_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(colorTextureImagesDevice, MAX_COLOR_TEXTURE_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(colorTextureViews, MAX_COLOR_TEXTURE_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(meshDescriptorSets, MAX_MESHES)

		NULL_HANDLE_INIT_ARRAY(uniformBuffers, MAX_UNIFORM_BUFFER_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(uniformBuffersDevice, MAX_UNIFORM_BUFFER_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(vertexBuffersDevice, MAX_VERTEX_BUFFER_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(vertexBuffers, MAX_VERTEX_BUFFER_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(indexBuffersDevice, MAX_INDEX_BUFFER_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(indexBuffers, MAX_INDEX_BUFFER_ARRAY_SIZE)

		NULL_HANDLE_INIT_ARRAY(specTextureViews, MAX_SPECULAR_TEXTURE_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(specTextureImages, MAX_SPECULAR_TEXTURE_ARRAY_SIZE)

		NULL_HANDLE_INIT_ARRAY(normalTextureImagesDevice, MAX_NORMAL_TEXTURE_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(normalTextureImages, MAX_NORMAL_TEXTURE_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(normalTextureViews, MAX_NORMAL_TEXTURE_ARRAY_SIZE)
		NULL_HANDLE_INIT_ARRAY(specTextureImagesDevice, MAX_SPECULAR_TEXTURE_ARRAY_SIZE)
}

VulkanEngine::~VulkanEngine() noexcept(false) {
	terminate();
}

void VulkanEngine::getInstanceExtensions() {
	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, nullptr);

	instanceExtensions.resize(instanceExtensionsCount);

	vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionsCount, instanceExtensions.data());

#ifdef PRINT_INSTANCE_EXTENSIONS
	std::cout << "Instance Extensions:\n";

	for (uint32_t i = 0; i < instanceExtensionsCount; i++) {
		std::cout << "\t" << instanceExtensions[i].extensionName << "("
			<< std::to_string(instanceExtensions[i].specVersion) << ")\n";
	}
#endif
}

void VulkanEngine::getDeviceExtensions() {
	vkEnumerateDeviceExtensionProperties(physicalDevices[0], nullptr, &deviceExtensionsCount, nullptr);

	deviceExtensions.resize(deviceExtensionsCount);

	vkEnumerateDeviceExtensionProperties(physicalDevices[0], nullptr, &deviceExtensionsCount, deviceExtensions.data());
#ifdef PRINT_DEVICE_EXTENSIONS
	std::cout << "Device Extensions:\n";

	for (uint32_t i = 0; i < deviceExtensionsCount; i++) {
		std::cout << "\t" << deviceExtensions[i].extensionName << "(" << std::to_string(deviceExtensions[0].specVersion)
			<< ")\n";
	}
#endif
}

std::string VulkanEngine::getVersionString(uint32_t versionBitmask) {
	char versionString[128];

	uint32_t uMajorAPIVersion = versionBitmask >> 22;
	uint32_t uMinorAPIVersion = ((versionBitmask << 10) >> 10) >> 12;
	uint32_t uPatchAPIVersion = (versionBitmask << 20) >> 20;

	int majorAPIVersion = uMajorAPIVersion;
	int minorAPIVersion = uMinorAPIVersion;
	int patchAPIVersion = uPatchAPIVersion;

	sprintf_s(versionString, "%d.%d.%d", majorAPIVersion, minorAPIVersion, patchAPIVersion);

	return versionString;
}

void VulkanEngine::initDevIL() {
	ilInit();
	std::cout << "DevIL library inited." << std::endl;
}

void VulkanEngine::init() {
	initDevIL();
	getInstanceExtensions();
	getInstanceLayers();
	createInstance();
	enumeratePhysicalDevices();
	getPhysicalDevicePropertiesAndFeatures();
	createLogicalDevice();
	createSyncMeans();
	getDeviceExtensions();
	getDeviceLayers();
	loadMesh(meshFileName.c_str());
	createAllTextures();
	createAllBuffers();
	getQueues();
	getQueueFamilyPresentationSupport();
	createSurface(); // create surface
	createSwapchain();  // create swapchain
	getSupportedDepthFormat();
	createDepthImageAndImageview(); // create depth image and depth imageview (depth-stencil attachment)
	createSwapchainImageViews(); // create swapchain imageviews (framebuffer color attachment)
	createRenderpass(); // create renderpass
	createFramebuffers(); // create framebuffer
	createGraphicsShaderModule("vert.glsl", &graphicsVertexShaderModule,
		shaderc_glsl_default_vertex_shader); // create vertex shader
	createGraphicsShaderModule("frag.glsl", &graphicsFragmentShaderModule,
		shaderc_glsl_default_fragment_shader); // create fragment shader
	//createGraphicsShaderModule("debug_vert.glsl", &graphicsNormalViewerVertexShaderModule,
	//	shaderc_glsl_default_vertex_shader); // create debug vertex shader
	//createGraphicsShaderModule("debug_geom.glsl", &graphicsNormalViewerGeometryShaderModule,
	//	shaderc_glsl_default_geometry_shader); // create debug geometry shader
	//createGraphicsShaderModule("debug_frag.glsl", &graphicsNormalViewerFragmentShaderModule,
	//	shaderc_glsl_default_fragment_shader); // create debug fragment shader
	createPipelineAndDescriptorSetsLayout(); // create pipeline layout
	createGraphicsPipeline(); //create pipeline
	//createGraphicsNormalViewerPipeline();
	createRenderCommandPool(); // create render commandpool
	createTransferCommandPool(); // create render commandpool
	commitBuffers();
	commitTextures();
	destroyStagingMeans();
	createDescriptorPool(); // create descriptorpool
	createDescriptorSets();
	setupTimer();
}

void VulkanEngine::setupTimer() {
	// get ticks per second
	QueryPerformanceFrequency(&frequency);

	// start timer
	QueryPerformanceCounter(&t1);
}

void VulkanEngine::calculateViewProjection() {

	mat4x4 rotationMatrix = glm::mat4(1.0f);

	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(this->focusYaw), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationMatrix = glm::rotate(rotationMatrix, glm::radians(this->focusPitch), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::vec4 cameraPosition = /*rotationMatrix **/ glm::vec4(0.0, 0.0, -this->cameraDistance, 1.0);

	glm::vec3 focusPoint(this->focusPointX, this->focusPointY, this->focusPointZ);

	this->viewProjection.viewMatrix = glm::lookAt(glm::vec3(cameraPosition), focusPoint,
		glm::vec3(0.0f, 1.0f, 0.0f));

	static const glm::vec3 cameraRotationAxis(.0f, 1.0f, .0f);

	this->viewProjection.viewMatrix = glm::rotate(this->viewProjection.viewMatrix, this->cameraRotationValue, cameraRotationAxis);


	float frameBufferAspectRatio =
		((float)this->swapchainCreateInfo.imageExtent.width) /
		((float)this->swapchainCreateInfo.imageExtent.height);


	this->viewProjection.projectionMatrix = glm::perspective(glm::radians(this->fovAngle),
		frameBufferAspectRatio, this->zNear,
		this->zFar);
}

void VulkanEngine::cameraRotate() {
	cameraRotationValue += autoRotationSpeed * getElapsedTime();
	calculateViewProjection();
}

void VulkanEngine::getSupportedDepthFormat() {

	// Since all depth formats may be optional, we need to find a suitable depth format to use
	// Start with the highest precision packed format
	std::vector<VkFormat> depthFormats = {
			VK_FORMAT_D32_SFLOAT_S8_UINT,
			VK_FORMAT_D32_SFLOAT,
			VK_FORMAT_D24_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM_S8_UINT,
			VK_FORMAT_D16_UNORM
	};

	for (auto& format : depthFormats) {
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physicalDevices[0], format, &formatProps);
		// Format must support depth stencil attachment for optimal tiling
		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			depthFormat = format;
			return;
		}
	}

	throw VulkanException("Couldn't find depth stencil supported image format to create with.");
}

void VulkanEngine::draw() {
	uint32_t drawableImageIndex = acquireNextFramebufferImageIndex();
	render(drawableImageIndex);
	present(drawableImageIndex);
}

void VulkanEngine::createInstance() {
	appInfo.apiVersion = VK_API_VERSION_1_0;
	appInfo.applicationVersion = 0;
	appInfo.pApplicationName = "VulkanEngine Test";
	appInfo.engineVersion = 0;
	appInfo.pEngineName = "VulkanEngine Engine";
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;

	std::vector<const char*> layerNames = { "VK_LAYER_LUNARG_monitor", "VK_LAYER_LUNARG_standard_validation" };
	std::vector<const char*> extensionNames = { "VK_KHR_surface", "VK_KHR_win32_surface" };

	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.enabledExtensionCount = 2;
#ifndef _ENTBUG
	instanceCreateInfo.enabledLayerCount = 2;
#else
	instanceCreateInfo.enabledLayerCount = 0;
#endif
	instanceCreateInfo.ppEnabledExtensionNames = extensionNames.data();
	instanceCreateInfo.ppEnabledLayerNames = layerNames.data();
	instanceCreateInfo.flags = 0;

	instanceCreateInfo.pApplicationInfo = &appInfo;

	if (vkCreateInstance(&instanceCreateInfo, nullptr, &instance) == VK_SUCCESS) {
		std::cout << "Instance created successfully.\n";
	}
	else {
		throw VulkanException("Instance creation failed.");
	}
}

void VulkanEngine::getInstanceLayers() {
	vkEnumerateInstanceLayerProperties(&layerPropertiesCount, nullptr);

	std::cout << "Number of Instance Layers available to physical device:\t" << std::to_string(layerPropertiesCount)
		<< std::endl;

	layerProperties.resize(layerPropertiesCount);

	vkEnumerateInstanceLayerProperties(&layerPropertiesCount, layerProperties.data());

#ifdef PRINT_INSTANCE_LAYERS
	std::cout << "Instance Layers:\n";

	for (uint32_t i = 0; i < layerPropertiesCount; i++) {
		std::cout << "\tLayer #" << std::to_string(i) << std::endl;
		std::cout << "\t\tName:\t" << layerProperties[i].layerName << std::endl;
		std::cout << "\t\tSpecification Version:\t" << getVersionString(layerProperties[i].specVersion) << std::endl;
		std::cout << "\t\tImplementation Version:\t" << layerProperties[i].implementationVersion << std::endl;
		std::cout << "\t\tDescription:\t" << layerProperties[i].description << std::endl;
	}
#endif
}

void VulkanEngine::createAllTextures() {
	uint64_t lastCoveredSizeDevice = 0;
	uint64_t lastCoveredSize = 0;

	colorTexturesBindOffsetsDevice = new VkDeviceSize[cachedScene->mNumMeshes];
	normalTexturesBindOffsetsDevice = new VkDeviceSize[cachedScene->mNumMeshes];
	specTexturesBindOffsetsDevice = new VkDeviceSize[cachedScene->mNumMeshes];

	colorTexturesBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];
	normalTexturesBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];
	specTexturesBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		uint64_t requiredPaddingDevice = 0;
		uint64_t requiredPadding = 0;

		VkDeviceSize colorTextureMemoryOffsetDevice;
		VkDeviceSize colorTextureMemorySizeDevice;
		VkDeviceSize normalTextureMemoryOffsetDevice;
		VkDeviceSize normalTextureMemorySizeDevice;
		VkDeviceSize specTextureMemoryOffsetDevice;
		VkDeviceSize specTextureMemorySizeDevice;

		VkDeviceSize colorTextureMemoryOffset;
		VkDeviceSize colorTextureMemorySize;
		VkDeviceSize normalTextureMemoryOffset;
		VkDeviceSize normalTextureMemorySize;
		VkDeviceSize specTextureMemoryOffset;
		VkDeviceSize specTextureMemorySize;

		VkMemoryRequirements colorTextureDeviceMemoryRequirement = createTexture(colorTextureImagesDevice + meshIndex,
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		VkMemoryRequirements normalTextureDeviceMemoryRequirement = createTexture(normalTextureImagesDevice + meshIndex,
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT);
		VkMemoryRequirements specTextureDeviceMemoryRequirement = createTexture(specTextureImagesDevice + meshIndex,
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_DST_BIT);

		VkMemoryRequirements colorTextureMemoryRequirement = createTexture(colorTextureImages + meshIndex,
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		VkMemoryRequirements normalTextureMemoryRequirement = createTexture(normalTextureImages + meshIndex,
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
		VkMemoryRequirements specTextureMemoryRequirement = createTexture(specTextureImages + meshIndex,
			VK_IMAGE_USAGE_SAMPLED_BIT |
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		if (meshIndex == 0) {
			colorTextureMemoryOffsetDevice = 0;
			colorTexturesBindOffsetsDevice[meshIndex] = colorTextureMemoryOffsetDevice;
			colorTextureMemorySizeDevice = colorTextureDeviceMemoryRequirement.size;

			lastCoveredSizeDevice += colorTextureMemoryOffsetDevice + colorTextureMemorySizeDevice;

			colorTextureMemoryOffset = 0;
			colorTexturesBindOffsets[meshIndex] = colorTextureMemoryOffset;
			colorTextureMemorySize = colorTextureMemoryRequirement.size;

			lastCoveredSize += colorTextureMemoryOffset + colorTextureMemorySize;
		}
		else {
			requiredPaddingDevice = colorTextureDeviceMemoryRequirement.alignment -
				(lastCoveredSizeDevice % colorTextureDeviceMemoryRequirement.alignment);

			if (requiredPaddingDevice == colorTextureDeviceMemoryRequirement.alignment)
				requiredPaddingDevice = 0;

			colorTextureMemoryOffsetDevice = lastCoveredSizeDevice + requiredPaddingDevice;
			colorTexturesBindOffsetsDevice[meshIndex] = colorTextureMemoryOffsetDevice;
			colorTextureMemorySizeDevice = colorTextureDeviceMemoryRequirement.size;

			lastCoveredSizeDevice += requiredPaddingDevice + colorTextureMemorySizeDevice;


			requiredPadding = colorTextureMemoryRequirement.alignment -
				(lastCoveredSize % colorTextureMemoryRequirement.alignment);

			if (requiredPadding == colorTextureMemoryRequirement.alignment)
				requiredPadding = 0;

			colorTextureMemoryOffset = lastCoveredSize + requiredPadding;
			colorTexturesBindOffsets[meshIndex] = colorTextureMemoryOffset;
			colorTextureMemorySize = colorTextureMemoryRequirement.size;

			lastCoveredSize += requiredPadding + colorTextureMemorySize;
		}

		requiredPaddingDevice = normalTextureDeviceMemoryRequirement.alignment -
			(lastCoveredSizeDevice % normalTextureDeviceMemoryRequirement.alignment);

		if (requiredPaddingDevice == normalTextureDeviceMemoryRequirement.alignment)
			requiredPaddingDevice = 0;

		normalTextureMemoryOffsetDevice = lastCoveredSizeDevice + requiredPaddingDevice;
		normalTexturesBindOffsetsDevice[meshIndex] = normalTextureMemoryOffsetDevice;
		normalTextureMemorySizeDevice = normalTextureDeviceMemoryRequirement.size;

		lastCoveredSizeDevice += requiredPaddingDevice + normalTextureMemorySizeDevice;

		requiredPaddingDevice = specTextureDeviceMemoryRequirement.alignment -
			(lastCoveredSizeDevice % specTextureDeviceMemoryRequirement.alignment);

		if (requiredPaddingDevice == specTextureDeviceMemoryRequirement.alignment)
			requiredPaddingDevice = 0;

		specTextureMemoryOffsetDevice = lastCoveredSizeDevice + requiredPaddingDevice;
		specTexturesBindOffsetsDevice[meshIndex] = specTextureMemoryOffsetDevice;
		specTextureMemorySizeDevice = specTextureDeviceMemoryRequirement.size;

		lastCoveredSizeDevice += requiredPaddingDevice + specTextureMemorySizeDevice;


		requiredPadding =
			normalTextureMemoryRequirement.alignment - (lastCoveredSize % normalTextureMemoryRequirement.alignment);

		if (requiredPadding == normalTextureMemoryRequirement.alignment)
			requiredPadding = 0;

		normalTextureMemoryOffset = lastCoveredSize + requiredPadding;
		normalTexturesBindOffsets[meshIndex] = normalTextureMemoryOffset;
		normalTextureMemorySize = normalTextureMemoryRequirement.size;

		lastCoveredSize += requiredPadding + normalTextureMemorySize;

		requiredPadding =
			specTextureMemoryRequirement.alignment - (lastCoveredSize % specTextureMemoryRequirement.alignment);

		if (requiredPadding == specTextureMemoryRequirement.alignment)
			requiredPadding = 0;

		specTextureMemoryOffset = lastCoveredSize + requiredPadding;
		specTexturesBindOffsets[meshIndex] = specTextureMemoryOffset;
		specTextureMemorySize = specTextureMemoryRequirement.size;

		lastCoveredSize += requiredPadding + specTextureMemorySize;

	}

	VkDeviceSize totalRequiredMemorySizeDevice = lastCoveredSizeDevice;
	VkDeviceSize totalRequiredMemorySize = lastCoveredSize;

	VkMemoryAllocateInfo uniMemoryAllocateInfo = {};
	uniMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	uniMemoryAllocateInfo.pNext = nullptr;
	uniMemoryAllocateInfo.allocationSize = totalRequiredMemorySize;
	uniMemoryAllocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;

	VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &uniMemoryAllocateInfo, nullptr, &uniTexturesMemoryDevice));

	uniMemoryAllocateInfo.allocationSize = totalRequiredMemorySizeDevice;
	uniMemoryAllocateInfo.memoryTypeIndex = hostVisibleMemoryTypeIndex;

	VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &uniMemoryAllocateInfo, nullptr, &uniTexturesMemory));

	void* mappedMemory = NULL;

	VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], uniTexturesMemory, 0, VK_WHOLE_SIZE, 0, &mappedMemory));

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		uint16_t fileNumber = meshIndex;

		VkImageSubresource textureImageSubresource = {};
		VkSubresourceLayout subresourceLayout;

		textureImageSubresource.arrayLayer = 0;
		textureImageSubresource.mipLevel = 0;
		textureImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		vkGetImageSubresourceLayout(logicalDevices[0], colorTextureImagesDevice[meshIndex], &textureImageSubresource,
			&subresourceLayout);

		ILuint imgName = ilGenImage();

		ilBindImage(imgName);

		std::string colorFileName(resourcesPath);

		colorFileName.append("t");
		colorFileName.append(std::to_string(fileNumber));
		colorFileName.append("c.png");

		if (ilLoadImage(colorFileName.c_str()) != IL_TRUE)
			throw VulkanException(makeDevilErrorText(colorFileName).c_str());

		void* pTextureData = ilGetData();

		for (int i = 0; i < textureDim; i++) {
			uint64_t rowPadding = -1;

			rowPadding = subresourceLayout.rowPitch - ((4 * textureDim) % subresourceLayout.rowPitch);

			if (rowPadding == subresourceLayout.rowPitch) rowPadding = 0;

			memcpy((byte*)mappedMemory + (i * (textureDim * 4 + rowPadding)) + colorTexturesBindOffsetsDevice[meshIndex],
				(byte*)pTextureData + (i * textureDim * 4), textureDim * 4);
		}

		ilDeleteImage(imgName);

		textureImageSubresource.arrayLayer = 0;
		textureImageSubresource.mipLevel = 0;
		textureImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		vkGetImageSubresourceLayout(logicalDevices[0], normalTextureImagesDevice[meshIndex], &textureImageSubresource,
			&subresourceLayout);

		imgName = ilGenImage();

		ilBindImage(imgName);

		std::string normalFileName(resourcesPath);

		normalFileName.append("t");
		normalFileName.append(std::to_string(fileNumber));
		normalFileName.append("n.png");

		if (ilLoadImage(normalFileName.c_str()) != IL_TRUE)
			throw VulkanException(makeDevilErrorText(normalFileName).c_str());

		pTextureData = ilGetData();

		for (int i = 0; i < textureDim; i++) {
			uint64_t rowPadding = -1;

			rowPadding = subresourceLayout.rowPitch - ((4 * textureDim) % subresourceLayout.rowPitch);

			if (rowPadding == subresourceLayout.rowPitch) rowPadding = 0;

			memcpy((byte*)mappedMemory + (i * (textureDim * 4 + rowPadding)) + normalTexturesBindOffsetsDevice[meshIndex],
				(byte*)pTextureData + (i * textureDim * 4), textureDim * 4);
		}

		ilDeleteImage(imgName);

		textureImageSubresource.arrayLayer = 0;
		textureImageSubresource.mipLevel = 0;
		textureImageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		vkGetImageSubresourceLayout(logicalDevices[0], specTextureImagesDevice[meshIndex], &textureImageSubresource,
			&subresourceLayout);

		imgName = ilGenImage();

		ilBindImage(imgName);

		std::string specFileName(resourcesPath);

		specFileName.append("t");
		specFileName.append(std::to_string(fileNumber));
		specFileName.append("s.png");

		if (ilLoadImage(specFileName.c_str()) != IL_TRUE)
			throw VulkanException(makeDevilErrorText(specFileName).c_str());

		pTextureData = ilGetData();

		for (int i = 0; i < textureDim; i++) {
			uint64_t rowPadding = -1;

			rowPadding = subresourceLayout.rowPitch - ((4 * textureDim) % subresourceLayout.rowPitch);

			if (rowPadding == subresourceLayout.rowPitch) rowPadding = 0;

			memcpy((byte*)mappedMemory + (i * (textureDim * 4 + rowPadding)) + specTexturesBindOffsetsDevice[meshIndex],
				(byte*)pTextureData + (i * textureDim * 4), textureDim * 4);
		}

		ilDeleteImage(imgName);
	}

	vkUnmapMemory(logicalDevices[0], uniTexturesMemory);

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		VKASSERT_SUCCESS(vkBindImageMemory(logicalDevices[0], colorTextureImages[meshIndex], uniTexturesMemory,
			colorTexturesBindOffsets[meshIndex]));
		VKASSERT_SUCCESS(vkBindImageMemory(logicalDevices[0], normalTextureImages[meshIndex], uniTexturesMemory,
			normalTexturesBindOffsets[meshIndex]));
		VKASSERT_SUCCESS(vkBindImageMemory(logicalDevices[0], specTextureImages[meshIndex], uniTexturesMemory,
			specTexturesBindOffsets[meshIndex]));

		VKASSERT_SUCCESS(
			vkBindImageMemory(logicalDevices[0], colorTextureImagesDevice[meshIndex], uniTexturesMemoryDevice,
				colorTexturesBindOffsetsDevice[meshIndex]));
		VKASSERT_SUCCESS(
			vkBindImageMemory(logicalDevices[0], normalTextureImagesDevice[meshIndex], uniTexturesMemoryDevice,
				normalTexturesBindOffsetsDevice[meshIndex]));
		VKASSERT_SUCCESS(
			vkBindImageMemory(logicalDevices[0], specTextureImagesDevice[meshIndex], uniTexturesMemoryDevice,
				specTexturesBindOffsetsDevice[meshIndex]));
	}

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		createTextureView(colorTextureViews + meshIndex, colorTextureImagesDevice[meshIndex]);
		createTextureView(normalTextureViews + meshIndex, normalTextureImagesDevice[meshIndex]);
		createTextureView(specTextureViews + meshIndex, specTextureImagesDevice[meshIndex]);
	}
}

void VulkanEngine::getDeviceLayers() {
	vkEnumerateDeviceLayerProperties(physicalDevices[0], &layerPropertiesCount, nullptr);

	std::cout << "Number of Device Layers available to physical device:\t" << std::to_string(layerPropertiesCount)
		<< std::endl;

	layerProperties.resize(layerPropertiesCount);

	vkEnumerateDeviceLayerProperties(physicalDevices[0], &layerPropertiesCount, layerProperties.data());

#ifdef PRINT_DEVICE_LAYERS
	std::cout << "Device Layers:\n";

	for (uint32_t i = 0; i < layerPropertiesCount; i++) {
		std::cout << "\tLayer #" << std::to_string(i) << std::endl;
		std::cout << "\t\tName:\t" << layerProperties[i].layerName << std::endl;
		std::cout << "\t\tSpecification Version:\t" << getVersionString(layerProperties[i].specVersion) << std::endl;
		std::cout << "\t\tImplementation Version:\t" << layerProperties[i].implementationVersion << std::endl;
		std::cout << "\t\tDescription:\t" << layerProperties[i].description << std::endl;
	}
#endif
}

void VulkanEngine::enumeratePhysicalDevices() {
	if (vkEnumeratePhysicalDevices(instance, &numberOfSupportedDevices, physicalDevices) == VK_SUCCESS) {
		std::cout
			<< "Physical device enumeration succeeded, first available device selected for logical device creation.\n";
	}
	else {
		throw VulkanException("Physical device enumeration failed.");
	}
}

void VulkanEngine::commitBuffers() {
	VkCommandBuffer commandBuffer;
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.commandPool = transferCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VKASSERT_SUCCESS(vkAllocateCommandBuffers(logicalDevices[0], &commandBufferAllocateInfo, &commandBuffer));

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	VKASSERT_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		VkBufferCopy region = {};
		region.srcOffset = 0;
		region.dstOffset = 0;
		region.size = totalUniformBufferSize;

		vkCmdCopyBuffer(commandBuffer, uniformBuffers[meshIndex], uniformBuffersDevice[meshIndex], 1, &region);

		region.size = vertexBuffersSizes[meshIndex];

		vkCmdCopyBuffer(commandBuffer, vertexBuffers[meshIndex], vertexBuffersDevice[meshIndex], 1, &region);

		region.size = indexBuffersSizes[meshIndex];

		vkCmdCopyBuffer(commandBuffer, indexBuffers[meshIndex], indexBuffersDevice[meshIndex], 1, &region);
	}

	VKASSERT_SUCCESS(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo queueSubmit = {};
	queueSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	queueSubmit.pNext = nullptr;
	queueSubmit.waitSemaphoreCount = 0;
	queueSubmit.pWaitSemaphores = nullptr;
	queueSubmit.pWaitDstStageMask = nullptr;
	queueSubmit.commandBufferCount = 1;
	queueSubmit.pCommandBuffers = &commandBuffer;
	queueSubmit.signalSemaphoreCount = 0;
	queueSubmit.pSignalSemaphores = nullptr;

	VKASSERT_SUCCESS(vkQueueSubmit(transferQueue, 1, &queueSubmit, VK_NULL_HANDLE));

	VKASSERT_SUCCESS(vkDeviceWaitIdle(logicalDevices[0]));

	vkFreeCommandBuffers(logicalDevices[0], transferCommandPool, 1, &commandBuffer);
}

int VulkanEngine::getMaxUsableSampleCount(VkPhysicalDeviceProperties physicalDeviceProperties) {
	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return 64; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return 32; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return 16; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return 8; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return 4; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return 2; }

	return 1;
}

void VulkanEngine::getPhysicalDevicePropertiesAndFeatures() {
	vkGetPhysicalDeviceProperties(physicalDevices[0], &deviceProperties);

	std::cout << "Highest Supported Vulkan Version:\t" << getVersionString(deviceProperties.apiVersion) << std::endl;

	vkGetPhysicalDeviceFeatures(physicalDevices[0], &supportedDeviceFeatures);

	std::cout << "Supports Tesselation Shader Feature:\t"
		<< ((supportedDeviceFeatures.tessellationShader) ? ("Yes") : ("No")) << std::endl;

	std::cout << "Maximum Supported Image Size:\t" << deviceProperties.limits.maxImageDimension1D << "x"
		<< deviceProperties.limits.maxImageDimension2D << "x" << deviceProperties.limits.maxImageDimension3D
		<< std::endl;

	std::cout << "Minimum Memory Map Alignment:\t" << deviceProperties.limits.minMemoryMapAlignment << std::endl;

	std::cout << "Max Local Working Group Size: " << deviceProperties.limits.maxComputeWorkGroupSize[0] << "x"
		<< deviceProperties.limits.maxComputeWorkGroupSize[1] << "x"
		<< deviceProperties.limits.maxComputeWorkGroupSize[2] << std::endl;

	std::cout << "Max Total Working Group Invocations: " << deviceProperties.limits.maxComputeWorkGroupInvocations
		<< std::endl;

	std::cout << "Max bound descriptor sets in a pipeline layout: " << deviceProperties.limits.maxBoundDescriptorSets
		<< std::endl;

	std::cout << "Minimum Uniform Buffer offest alignment: " << deviceProperties.limits.minUniformBufferOffsetAlignment
		<< std::endl;

	std::cout << "Maximum Texel Buffer elements: " << deviceProperties.limits.maxTexelBufferElements << std::endl;

	std::cout << "Maximum Subpass Color Attachments: " << deviceProperties.limits.maxColorAttachments << std::endl;


	int maxUsableSampleCount = getMaxUsableSampleCount(deviceProperties);

	std::cout << "Maximum Framebuffer Sample Count: " << maxUsableSampleCount << std::endl;

	std::cout << "Maximum Framebuffer Width: " << deviceProperties.limits.maxFramebufferWidth << std::endl;

	std::cout << "Maximum Framebuffer Height: " << deviceProperties.limits.maxFramebufferHeight << std::endl;

	std::cout << "Maximum Framebuffer Layers: " << deviceProperties.limits.maxFramebufferLayers << std::endl;

	std::cout << "Maximum Vertex Input bindings: " << deviceProperties.limits.maxVertexInputBindings << std::endl;

	std::cout << "Maximum Vertex Input Attributes: " << deviceProperties.limits.maxVertexInputAttributes << std::endl;

	std::cout << "Maximum Vertex Input Binding Stride: " << deviceProperties.limits.maxVertexInputBindingStride
		<< std::endl;

	std::cout << "Maximum Vertex Input Attribute Offset: " << deviceProperties.limits.maxVertexInputAttributeOffset
		<< std::endl;

	std::cout << "Maximum Viewports: " << deviceProperties.limits.maxViewports << std::endl;

	std::cout << "Minimum Line Width: " << deviceProperties.limits.lineWidthRange[0] << std::endl;

	std::cout << "Maximum Line Width: " << deviceProperties.limits.lineWidthRange[1] << std::endl;

	std::cout << "Line Width Granularity: " << deviceProperties.limits.lineWidthGranularity << std::endl;

	std::cout << "Max Index Value: " << deviceProperties.limits.maxDrawIndexedIndexValue << std::endl;

	std::cout << "Max Geometry Produced Vertices: " << deviceProperties.limits.maxGeometryOutputVertices << std::endl;

	std::cout << "Max Geometry Produced Components: " << deviceProperties.limits.maxGeometryOutputComponents
		<< std::endl;

	std::cout << "Max Geometry Total Produced Components: " << deviceProperties.limits.maxGeometryTotalOutputComponents
		<< std::endl;

	std::cout << "Max Geometry Consumed Components: " << deviceProperties.limits.maxGeometryInputComponents
		<< std::endl;

	vkGetPhysicalDeviceMemoryProperties(physicalDevices[0], &deviceMemoryProperties);

	memoryTypeCount = deviceMemoryProperties.memoryTypeCount;

	std::cout << "Memory Type count: " << memoryTypeCount << std::endl;

	std::string flagsString;

	size_t lastSelectedHostVisibleMemoryTypeHeapSize = 0;
	size_t lastSelectedDeviceLocalMemoryTypeHeapSize = 0;

	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
		flagsString = "";

		int heapIndex = deviceMemoryProperties.memoryTypes[i].heapIndex;

		if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && \
			deviceMemoryProperties.memoryHeaps[heapIndex].size > lastSelectedHostVisibleMemoryTypeHeapSize) {
			hostVisibleMemoryTypeIndex = i;
			lastSelectedHostVisibleMemoryTypeHeapSize = deviceMemoryProperties.memoryHeaps[heapIndex].size;
		}

		if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) && \
			deviceMemoryProperties.memoryHeaps[heapIndex].size > lastSelectedDeviceLocalMemoryTypeHeapSize) {
			deviceLocalMemoryTypeIndex = i;
			lastSelectedDeviceLocalMemoryTypeHeapSize = deviceMemoryProperties.memoryHeaps[heapIndex].size;
		}

		if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
			flagsString += "DEVICE_LOCAL ";
		}

		if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
			flagsString += "HOST_VISIBLE ";
		}

		if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) {
			flagsString += "HOST_COHERENT ";
		}

		if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0) {
			flagsString += "HOST_CACHED ";
		}

		if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) != 0) {
			flagsString += "LAZILY_ALLOCATED ";
		}


#ifdef PRINT_MEMORY_TYPES
		std::cout << "Memory type " << i << ":\n";
		std::cout << "\tFlags: " << flagsString << std::endl;
		std::cout << "\tHeap Index: " << heapIndex << std::endl;
		std::cout << "\tHeap Size: "
			<< ((float)deviceMemoryProperties.memoryHeaps[heapIndex].size) / (1024 * 1024 * 1024) << "GB"
			<< std::endl;
#endif

		//if ((((deviceMemoryProperties.memoryTypes[i].propertyFlags << (31 - VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) >> (31 - VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) >> VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
		totalHeapMemorySize += deviceMemoryProperties.memoryHeaps[heapIndex].size;
	}

	if (hostVisibleMemoryTypeIndex == -1)
		throw VulkanException("No host visible memory type found.");

	if (deviceLocalMemoryTypeIndex == -1)
		throw VulkanException("No device local memory type found.");

	std::cout << "\nTotal Heaps Size:\t" << std::to_string((float)totalHeapMemorySize / (1024 * 1024 * 1024)) << "GB"
		<< std::endl;
}

void VulkanEngine::terminate() {
	//if (*ppUnstableInstance_img != NULL)
	//	instanceToTerminate = *ppUnstableInstance_img;
	//else
	//Shutting Down
	//releasing memory

	//Terminte other threads
	//Working current thread possible command buffer generation through flags
	if (instance == VK_NULL_HANDLE)
		return;

	if (logicalDevices[0] == VK_NULL_HANDLE)
	{
		vkDestroyInstance(instance, nullptr);
		std::cout << "Instance destroyed." << std::endl;

		return;
	}


	std::cout << "Waiting for logical device to return (get idle)..." << std::endl;;
	vkDeviceWaitIdle(logicalDevices[0]);
	std::cout << "Logical device returned (got idle)." << std::endl;;


	if (textureSampler != VK_NULL_HANDLE) {
		vkDestroySampler(logicalDevices[0], textureSampler, nullptr);
		std::cout << "Texture Sampler destroyed." << std::endl;
	}

	if (cachedScene != nullptr) {
		uint32_t numUniformBuffersDeviceDestroyed = 0;
		uint32_t numVertexBuffersDeviceDestroyed = 0;
		uint32_t numIndexBuffersDeviceDestroyed = 0;
		for (uint16_t i = 0; i < cachedScene->mNumMeshes; i++) {
			if (uniformBuffersDevice[i] != VK_NULL_HANDLE) {
				vkDestroyBuffer(logicalDevices[0], uniformBuffersDevice[i], nullptr);
				numUniformBuffersDeviceDestroyed++;
			}

			if (vertexBuffersDevice[i] != VK_NULL_HANDLE) {
				vkDestroyBuffer(logicalDevices[0], vertexBuffersDevice[i], nullptr);
				numVertexBuffersDeviceDestroyed++;
			}

			if (indexBuffersDevice[i] != VK_NULL_HANDLE) {
				vkDestroyBuffer(logicalDevices[0], indexBuffersDevice[i], nullptr);
				numIndexBuffersDeviceDestroyed++;
			}
		}
		if (numUniformBuffersDeviceDestroyed)
			std::cout << numUniformBuffersDeviceDestroyed << " Uniform Buffer(s) destroyed." << std::endl;
		if (numVertexBuffersDeviceDestroyed)
			std::cout << numVertexBuffersDeviceDestroyed << " Vertex Buffer(s) destroyed." << std::endl;
		if (numIndexBuffersDeviceDestroyed)
			std::cout << numIndexBuffersDeviceDestroyed << " Index Buffer(s) destroyed." << std::endl;
	}

	if (uniBuffersMemoryDevice != VK_NULL_HANDLE) {
		vkFreeMemory(logicalDevices[0], uniBuffersMemoryDevice, nullptr);
		std::cout << "Buffers Memory freed." << std::endl;
	}

	if (cachedScene != nullptr) {
		uint32_t numImageViewsDestroyed = 0;
		uint32_t numImagesDestroyed = 0;
		for (uint16_t i = 0; i < cachedScene->mNumMeshes; i++) {
			if (specTextureViews[i] != VK_NULL_HANDLE) {
				vkDestroyImageView(logicalDevices[0], specTextureViews[i], nullptr);
				numImageViewsDestroyed++;
			}

			if (specTextureImagesDevice[i] != VK_NULL_HANDLE) {
				vkDestroyImage(logicalDevices[0], specTextureImagesDevice[i], nullptr);
				numImagesDestroyed++;
			}

			if (normalTextureViews[i] != VK_NULL_HANDLE) {
				vkDestroyImageView(logicalDevices[0], normalTextureViews[i], nullptr);
				numImageViewsDestroyed++;
			}

			if (normalTextureImagesDevice[i] != VK_NULL_HANDLE) {
				vkDestroyImage(logicalDevices[0], normalTextureImagesDevice[i], nullptr);
				numImagesDestroyed++;
			}

			if (colorTextureViews[i] != VK_NULL_HANDLE) {
				vkDestroyImageView(logicalDevices[0], colorTextureViews[i], nullptr);
				numImageViewsDestroyed++;
			}

			if (colorTextureImagesDevice[i] != VK_NULL_HANDLE) {
				vkDestroyImage(logicalDevices[0], colorTextureImagesDevice[i], nullptr);
				numImagesDestroyed++;
			}
		}
		if (numImageViewsDestroyed)
			std::cout << numImageViewsDestroyed << " ImageView(s) Destroyed." << std::endl;
		if (numImagesDestroyed)
			std::cout << numImagesDestroyed << " Image(s) Destroyed." << std::endl;
	}

	if (uniTexturesMemoryDevice != VK_NULL_HANDLE) {
		vkFreeMemory(logicalDevices[0], uniTexturesMemoryDevice, nullptr);
		std::cout << "Textures Memory freed." << std::endl;
	}

	if (graphicsDescriptorSetLayout != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(logicalDevices[0], graphicsDescriptorSetLayout, nullptr);
		std::cout << "DescriptorSet Layout destroyed successfully." << std::endl;
	}

	if (descriptorPool != VK_NULL_HANDLE) {
		vkDestroyDescriptorPool(logicalDevices[0], descriptorPool, nullptr);
		std::cout << "Descriptor Set Pool destroyed successfully." << std::endl;
	}

	if (depthImageView != VK_NULL_HANDLE) {
		vkDestroyImageView(logicalDevices[0], depthImageView, nullptr);
		std::cout << "Depth-Stencil ImageView destroyed." << std::endl;
	}

	if (depthImage != VK_NULL_HANDLE) {
		vkDestroyImage(logicalDevices[0], depthImage, nullptr);
		std::cout << "Depth-Stencil Image destroyed." << std::endl;
	}

	if (depthImageMemory != VK_NULL_HANDLE) {
		vkFreeMemory(logicalDevices[0], depthImageMemory, nullptr);
		std::cout << "Depth-Stencil Image Memory freed." << std::endl;
	}

	uint32_t numSwapchainImageViewsDestroyed = 0;
	for (uint32_t i = 0; i < swapchainImagesCount; i++)
	{
		if (swapchainImageViews[i] != VK_NULL_HANDLE) {
			vkDestroyImageView(logicalDevices[0], swapchainImageViews[i], nullptr);
			numSwapchainImageViewsDestroyed++;
		}
	}
	if (numSwapchainImageViewsDestroyed)
		std::cout << numSwapchainImageViewsDestroyed << " Swapchain ImageViews destroyed successfully." << std::endl;

	if (renderCommandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(logicalDevices[0], renderCommandPool, nullptr);
		std::cout << "Render Command Pool destroyed successfully." << std::endl;
	}

	if (transferCommandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(logicalDevices[0], transferCommandPool, nullptr);
		std::cout << "Transfer Command Pool destroyed successfully." << std::endl;
	}

	if (graphicsVertexShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(logicalDevices[0], graphicsVertexShaderModule, nullptr);
		std::cout << "Graphics Vertex Shader module destroyed successfully." << std::endl;
	}

	if (graphicsFragmentShaderModule != VK_NULL_HANDLE) {
		vkDestroyShaderModule(logicalDevices[0], graphicsFragmentShaderModule, nullptr);
		std::cout << "Graphics Fragment Shader module destroyed successfully." << std::endl;
	}

	if (graphicsPipelineLayout != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(logicalDevices[0], graphicsPipelineLayout, nullptr);
		std::cout << "Graphics Pipeline layout destroyed successfully." << std::endl;
	}

	if (renderPass != VK_NULL_HANDLE) {
		vkDestroyRenderPass(logicalDevices[0], renderPass, nullptr);
		std::cout << "Renderpass destroyed successfully." << std::endl;
	}

	if (graphicsPipeline != VK_NULL_HANDLE) {
		vkDestroyPipeline(logicalDevices[0], graphicsPipeline, nullptr);
		std::cout << "Pipeline destroyed successfully." << std::endl;
	}

	uint32_t numFramebuffersDestroyed = 0;
	for (uint32_t i = 0; i < swapchainImagesCount; i++)
	{
		if (framebuffers[i] != VK_NULL_HANDLE) {
			vkDestroyFramebuffer(logicalDevices[0], framebuffers[i], nullptr);
			numFramebuffersDestroyed++;
		}
	}
	if (numFramebuffersDestroyed)
		std::cout << numFramebuffersDestroyed << " Framebuffer(s) destroyed successfully." << std::endl;

	if (swapchain != VK_NULL_HANDLE) {
		vkDestroySwapchainKHR(logicalDevices[0], swapchain, nullptr);
		std::cout << "Swapchain destroyed successfully." << std::endl;
	}

	destroySyncMeans();

	if (logicalDevices[0] != VK_NULL_HANDLE) {
		vkDestroyDevice(logicalDevices[0], nullptr);
		std::cout << "Logical Device destroyed." << std::endl;
	}

	if (instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
		std::cout << "Instance destroyed." << std::endl;
	}
}

void VulkanEngine::createLogicalDevice() {
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[0], &numQueueFamilies, nullptr);

	std::cout << "Number of Queue Families:\t" << std::to_string(numQueueFamilies) << std::endl;


	queueFamilyProperties.resize(numQueueFamilies);

	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[0], &numQueueFamilies, queueFamilyProperties.data());

	for (uint32_t i = 0; i < numQueueFamilies; i++) {
		std::string queueFlagsString = "";

		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
			queueFlagsString += "GRAPHICS ";
		}

		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
			queueFlagsString += "COMPUTE ";
		}

		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0) {
			queueFlagsString += "TRANSFER ";
		}

		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) != 0) {
			queueFlagsString += "SPARSE-BINDING ";
		}

#ifdef PRINT_QUEUE_FAMILIES
		std::cout << std::endl;
		std::cout << "Queue Family " << i << ":\n";
		std::cout << "\t Queue Family Queue Amount: " << queueFamilyProperties[i].queueCount << std::endl;
		std::cout << "\t Queue Family Min Image Transfer Granularity Width: "
			<< queueFamilyProperties[i].minImageTransferGranularity.width << std::endl;
		std::cout << "\t Queue Family Min Image Transfer Granularity Height: "
			<< queueFamilyProperties[i].minImageTransferGranularity.height << std::endl;
		std::cout << "\t Queue Family Min Image Transfer Granularity Depth: "
			<< queueFamilyProperties[i].minImageTransferGranularity.depth << std::endl;
		std::cout << "\t Queue Family Flags: " << queueFlagsString << std::endl;
		std::cout << "\t Queue Family Timestamp Valid Bits: " << queueFamilyProperties[i].timestampValidBits << "Bits"
			<< std::endl;
#endif


		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0 && graphicsQueueFamilyIndex == -1) {
			graphicsQueueFamilyIndex = i;
			graphicsQueueFamilyNumQueue = queueFamilyProperties[i].queueCount;
		}

		if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) != 0 && transferQueueFamilyIndex == -1) {
			transferQueueFamilyIndex = i;
			transferQueueFamilyNumQueue = queueFamilyProperties[i].queueCount;
		}

		queueCount += queueFamilyProperties[i].queueCount;
	}

	if (graphicsQueueFamilyIndex == -1 || graphicsQueueFamilyNumQueue == 0) {
		throw VulkanException("Cannot find queue family that supports graphics.");
	}

	if (transferQueueFamilyIndex == -1 || transferQueueFamilyNumQueue == 0) {
		throw VulkanException("Cannot find queue family that supports transferring.");
	}

	std::cout << "Total Device Queue Count:\t" << std::to_string(queueCount) << std::endl;


	deviceQueueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfos[0].flags = 0;
	deviceQueueCreateInfos[0].pNext = nullptr;
	deviceQueueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
	deviceQueueCreateInfos[0].queueCount = graphicsQueueFamilyNumQueue;
	float* graphicsQueuePriorities = new float[graphicsQueueFamilyNumQueue];

	for (uint32_t i = 0; i < graphicsQueueFamilyNumQueue; i++)
		graphicsQueuePriorities[i] = (i == 1) ? (1.0f) : (0.0f);

	deviceQueueCreateInfos[0].pQueuePriorities = graphicsQueuePriorities;


	deviceQueueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	deviceQueueCreateInfos[1].flags = 0;
	deviceQueueCreateInfos[1].pNext = nullptr;
	deviceQueueCreateInfos[1].queueFamilyIndex = transferQueueFamilyIndex;
	deviceQueueCreateInfos[1].queueCount = transferQueueFamilyNumQueue;
	float* transferQueuePriorities = new float[transferQueueFamilyNumQueue];

	for (uint32_t i = 0; i < transferQueueFamilyNumQueue; i++)
		transferQueuePriorities[i] = (i == 1) ? (1.0f) : (0.0f);

	deviceQueueCreateInfos[1].pQueuePriorities = transferQueuePriorities;


	std::vector<const char*> extensionNames = { "VK_KHR_swapchain" };

	desiredDeviceFeatures.geometryShader = VK_FALSE;

	logicalDeviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logicalDeviceCreateInfo.flags = 0;
	logicalDeviceCreateInfo.pNext = nullptr;
	logicalDeviceCreateInfo.queueCreateInfoCount = (deviceQueueCreateInfos[1].queueFamilyIndex ==
		deviceQueueCreateInfos[0].queueFamilyIndex) ? (1) : (2);
	logicalDeviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos;
	logicalDeviceCreateInfo.enabledExtensionCount = 1;
	logicalDeviceCreateInfo.enabledLayerCount = 0;
	logicalDeviceCreateInfo.ppEnabledExtensionNames = extensionNames.data();
	logicalDeviceCreateInfo.ppEnabledLayerNames = nullptr;
	logicalDeviceCreateInfo.pEnabledFeatures = &desiredDeviceFeatures;


	if (vkCreateDevice(physicalDevices[0], &logicalDeviceCreateInfo, nullptr, logicalDevices) == VK_SUCCESS) {
		std::cout << "Logical device creation succeeded.\n";
	}
	else {
		throw VulkanException("Logical Device creation failed.\n");
	}

	delete transferQueuePriorities;
	delete graphicsQueuePriorities;
}

void VulkanEngine::createAllBuffers() {
	uint64_t lastCoveredSizeDevice = 0;
	uint64_t lastCoveredSize = 0;

	uniformBuffersBindOffsetsDevice = new VkDeviceSize[cachedScene->mNumMeshes];
	vertexBuffersBindOffsetsDevice = new VkDeviceSize[cachedScene->mNumMeshes];
	indexBuffersBindOffsetsDevice = new VkDeviceSize[cachedScene->mNumMeshes];

	uniformBuffersBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];
	vertexBuffersBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];
	indexBuffersBindOffsets = new VkDeviceSize[cachedScene->mNumMeshes];

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		uint64_t requiredPaddingDevice = 0;
		uint64_t requiredPadding = 0;


		VkDeviceSize uniformBufferMemoryOffset;
		VkDeviceSize uniformBufferMemorySize;
		VkDeviceSize vertexBufferMemoryOffset;
		VkDeviceSize vertexBufferMemorySize;
		VkDeviceSize indexBufferMemoryOffset;
		VkDeviceSize indexBufferMemorySize;

		VkDeviceSize uniformBufferMemoryOffsetDevice;
		VkDeviceSize uniformBufferMemorySizeDevice;
		VkDeviceSize vertexBufferMemoryOffsetDevice;
		VkDeviceSize vertexBufferMemorySizeDevice;
		VkDeviceSize indexBufferMemoryOffsetDevice;
		VkDeviceSize indexBufferMemorySizeDevice;

		VkMemoryRequirements uniformBufferDeviceMemoryRequirements = createBuffer(uniformBuffersDevice + meshIndex,
			totalUniformBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		VkMemoryRequirements vertexBufferDeviceMemoryRequirements = createBuffer(vertexBuffersDevice + meshIndex,
			vertexBuffersSizes[meshIndex],
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		VkMemoryRequirements indexBufferDeviceMemoryRequirements = createBuffer(indexBuffersDevice + meshIndex,
			indexBuffersSizes[meshIndex],
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		VkMemoryRequirements uniformBufferMemoryRequirements = createBuffer(uniformBuffers + meshIndex,
			totalUniformBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		VkMemoryRequirements vertexBufferMemoryRequirements = createBuffer(vertexBuffers + meshIndex,
			vertexBuffersSizes[meshIndex],
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
		VkMemoryRequirements indexBufferMemoryRequirements = createBuffer(indexBuffers + meshIndex,
			indexBuffersSizes[meshIndex],
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

		if (meshIndex == 0) {
			uniformBufferMemoryOffsetDevice = 0;
			uniformBuffersBindOffsetsDevice[meshIndex] = uniformBufferMemoryOffsetDevice;
			uniformBufferMemorySizeDevice = totalUniformBufferSize;

			lastCoveredSizeDevice += uniformBufferMemoryOffsetDevice + uniformBufferMemorySizeDevice;

			uniformBufferMemoryOffset = 0;
			uniformBuffersBindOffsets[meshIndex] = uniformBufferMemoryOffset;
			uniformBufferMemorySize = totalUniformBufferSize;

			lastCoveredSize += uniformBufferMemoryOffset + uniformBufferMemorySize;
		}
		else {
			requiredPaddingDevice = uniformBufferDeviceMemoryRequirements.alignment -
				(lastCoveredSizeDevice % uniformBufferDeviceMemoryRequirements.alignment);

			if (requiredPaddingDevice == uniformBufferDeviceMemoryRequirements.alignment)
				requiredPaddingDevice = 0;

			uniformBufferMemoryOffsetDevice = lastCoveredSizeDevice + requiredPaddingDevice;
			uniformBuffersBindOffsetsDevice[meshIndex] = uniformBufferMemoryOffsetDevice;
			uniformBufferMemorySizeDevice = totalUniformBufferSize;

			lastCoveredSizeDevice += requiredPaddingDevice + uniformBufferMemorySizeDevice;


			requiredPadding = uniformBufferMemoryRequirements.alignment -
				(lastCoveredSize % uniformBufferMemoryRequirements.alignment);

			if (requiredPadding == uniformBufferMemoryRequirements.alignment)
				requiredPadding = 0;

			uniformBufferMemoryOffset = lastCoveredSize + requiredPadding;
			uniformBuffersBindOffsets[meshIndex] = uniformBufferMemoryOffset;
			uniformBufferMemorySize = totalUniformBufferSize;

			lastCoveredSize += requiredPadding + uniformBufferMemorySize;
		}

		requiredPaddingDevice = vertexBufferDeviceMemoryRequirements.alignment -
			(lastCoveredSizeDevice % vertexBufferDeviceMemoryRequirements.alignment);

		if (requiredPaddingDevice == vertexBufferDeviceMemoryRequirements.alignment)
			requiredPaddingDevice = 0;

		vertexBufferMemoryOffsetDevice = lastCoveredSizeDevice + requiredPaddingDevice;
		vertexBuffersBindOffsetsDevice[meshIndex] = vertexBufferMemoryOffsetDevice;
		vertexBufferMemorySizeDevice = vertexBuffersSizes[meshIndex];

		lastCoveredSizeDevice += requiredPaddingDevice + vertexBufferMemorySizeDevice;

		requiredPaddingDevice = indexBufferDeviceMemoryRequirements.alignment -
			(lastCoveredSizeDevice % indexBufferDeviceMemoryRequirements.alignment);

		if (requiredPaddingDevice == indexBufferDeviceMemoryRequirements.alignment)
			requiredPaddingDevice = 0;

		indexBufferMemoryOffsetDevice = lastCoveredSizeDevice + requiredPaddingDevice;
		indexBuffersBindOffsetsDevice[meshIndex] = indexBufferMemoryOffsetDevice;
		indexBufferMemorySizeDevice = indexBuffersSizes[meshIndex];

		lastCoveredSizeDevice += requiredPaddingDevice + indexBufferMemorySizeDevice;


		requiredPadding =
			vertexBufferMemoryRequirements.alignment - (lastCoveredSize % vertexBufferMemoryRequirements.alignment);

		if (requiredPadding == vertexBufferMemoryRequirements.alignment)
			requiredPadding = 0;

		vertexBufferMemoryOffset = lastCoveredSize + requiredPadding;
		vertexBuffersBindOffsets[meshIndex] = vertexBufferMemoryOffset;
		vertexBufferMemorySize = vertexBuffersSizes[meshIndex];

		lastCoveredSize += requiredPadding + vertexBufferMemorySize;

		requiredPadding =
			indexBufferMemoryRequirements.alignment - (lastCoveredSize % indexBufferMemoryRequirements.alignment);

		if (requiredPadding == indexBufferMemoryRequirements.alignment)
			requiredPadding = 0;

		indexBufferMemoryOffset = lastCoveredSize + requiredPadding;
		indexBuffersBindOffsets[meshIndex] = indexBufferMemoryOffset;
		indexBufferMemorySize = indexBuffersSizes[meshIndex];

		lastCoveredSize += requiredPadding + indexBufferMemorySize;

	}

	VkDeviceSize totalRequiredMemorySizeDevice = lastCoveredSizeDevice;
	VkDeviceSize totalRequiredMemorySize = lastCoveredSize;

	VkMemoryAllocateInfo uniMemoryAllocateInfo = {};
	uniMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	uniMemoryAllocateInfo.pNext = nullptr;
	uniMemoryAllocateInfo.allocationSize = totalRequiredMemorySize;
	uniMemoryAllocateInfo.memoryTypeIndex = hostVisibleMemoryTypeIndex;

	VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &uniMemoryAllocateInfo, nullptr, &uniBuffersMemory));

	uniMemoryAllocateInfo.allocationSize = totalRequiredMemorySizeDevice;
	uniMemoryAllocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;

	VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &uniMemoryAllocateInfo, nullptr, &uniBuffersMemoryDevice));

	void* mappedMemory = NULL;

	VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], uniBuffersMemory, 0, VK_WHOLE_SIZE, 0, &mappedMemory));

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		modelMatrix.modelMatrix = glm::mat4x4(1.0f);

		memcpy((byte*)mappedMemory + uniformBuffersBindOffsetsDevice[meshIndex], &modelMatrix,
			totalUniformBufferSize);

		memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryFlushRange.pNext = nullptr;
		memoryFlushRange.size = totalUniformBufferSize;
		memoryFlushRange.offset = uniformBuffersBindOffsetsDevice[meshIndex];
		memoryFlushRange.memory = uniBuffersMemory;

		VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));

		memcpy((byte*)mappedMemory + vertexBuffersBindOffsetsDevice[meshIndex], sortedAttributes[meshIndex].data(),
			vertexBuffersSizes[meshIndex]);

		memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryFlushRange.pNext = nullptr;
		memoryFlushRange.size = vertexBuffersSizes[meshIndex];
		memoryFlushRange.offset = vertexBuffersBindOffsetsDevice[meshIndex];
		memoryFlushRange.memory = uniBuffersMemory;

		VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));

		memcpy((byte*)mappedMemory + indexBuffersBindOffsetsDevice[meshIndex], sortedIndices[meshIndex].data(),
			indexBuffersSizes[meshIndex]);

		memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		memoryFlushRange.pNext = nullptr;
		memoryFlushRange.size = indexBuffersSizes[meshIndex];
		memoryFlushRange.offset = indexBuffersBindOffsetsDevice[meshIndex];
		memoryFlushRange.memory = uniBuffersMemory;

		VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));
	}

	vkUnmapMemory(logicalDevices[0], uniBuffersMemory);

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], uniformBuffers[meshIndex], uniBuffersMemory,
			uniformBuffersBindOffsets[meshIndex]));
		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], vertexBuffers[meshIndex], uniBuffersMemory,
			vertexBuffersBindOffsets[meshIndex]));
		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], indexBuffers[meshIndex], uniBuffersMemory,
			indexBuffersBindOffsets[meshIndex]));

		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], uniformBuffersDevice[meshIndex], uniBuffersMemoryDevice,
			uniformBuffersBindOffsetsDevice[meshIndex]));
		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], vertexBuffersDevice[meshIndex], uniBuffersMemoryDevice,
			vertexBuffersBindOffsetsDevice[meshIndex]));
		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], indexBuffersDevice[meshIndex], uniBuffersMemoryDevice,
			indexBuffersBindOffsetsDevice[meshIndex]));
	}
}

void VulkanEngine::createDepthImageAndImageview() {

	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = nullptr;
	imageCreateInfo.flags = 0;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = depthFormat;
	imageCreateInfo.extent.width = swapchainCreateInfo.imageExtent.width;
	imageCreateInfo.extent.height = swapchainCreateInfo.imageExtent.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = nullptr;

	VKASSERT_SUCCESS(vkCreateImage(logicalDevices[0], &imageCreateInfo, nullptr, &depthImage));

	VkMemoryRequirements depthImageMemoryRequirements;

	vkGetImageMemoryRequirements(logicalDevices[0], depthImage, &depthImageMemoryRequirements);

	VkMemoryAllocateInfo depthImageMemoryAllocateInfo = {};

	depthImageMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	depthImageMemoryAllocateInfo.pNext = nullptr;
	depthImageMemoryAllocateInfo.allocationSize = depthImageMemoryRequirements.size;
	depthImageMemoryAllocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;

	VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &depthImageMemoryAllocateInfo, nullptr, &depthImageMemory));

	VKASSERT_SUCCESS(vkBindImageMemory(logicalDevices[0], depthImage, depthImageMemory, 0));

	VkImageViewCreateInfo depthImageViewCreateInfo = {};

	depthImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthImageViewCreateInfo.pNext = NULL;
	depthImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthImageViewCreateInfo.format = depthFormat;
	depthImageViewCreateInfo.flags = 0;
	depthImageViewCreateInfo.image = depthImage;
	depthImageViewCreateInfo.subresourceRange = {};
	depthImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
	depthImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	depthImageViewCreateInfo.subresourceRange.levelCount = 1;
	depthImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	depthImageViewCreateInfo.subresourceRange.layerCount = 1;

	VKASSERT_SUCCESS(vkCreateImageView(logicalDevices[0], &depthImageViewCreateInfo, nullptr, &depthImageView));
}

//void VulkanEngine::getPhysicalDeviceImageFormatProperties(VkFormat imageFormat) {
//    vkGetPhysicalDeviceFormatProperties(physicalDevices[0], imageFormat, &formatProperties);
//
//    std::string bufferFormatFeatureFlagsString = "";
//    std::string linearTilingFormatFeatureFlagsString = "";
//    std::string optimalTilingFormatFeatureFlagsString = "";
//
//#pragma region format features bits
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT != 0) {
//        bufferFormatFeatureFlagsString += "SAMPLED_IMAGED ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT != 0) {
//        bufferFormatFeatureFlagsString += "SAMPLED_IMAGE_FILTER_LINEAR ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT != 0) {
//        bufferFormatFeatureFlagsString += "STORAGE_IMAGE ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT != 0) {
//        bufferFormatFeatureFlagsString += "STORAGE_IMAGE_ATOMIC ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT != 0) {
//        bufferFormatFeatureFlagsString += "UNIFORM_TEXEL_BUFFER ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT != 0) {
//        bufferFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT != 0) {
//        bufferFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER_ATOMIC ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT != 0) {
//        bufferFormatFeatureFlagsString += "VERTEX_BUFFER ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT != 0) {
//        bufferFormatFeatureFlagsString += "COLOR_ATTACHMENT ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT != 0) {
//        bufferFormatFeatureFlagsString += "COLOR_ATTACHMENT_BLEND ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT != 0) {
//        bufferFormatFeatureFlagsString += "DEPTH_STENCIL_ATTACHMENT ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT != 0) {
//        bufferFormatFeatureFlagsString += "BLIT_SRC ";
//    }
//
//    if (formatProperties.bufferFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT != 0) {
//        bufferFormatFeatureFlagsString += "BLIT_DST ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "SAMPLED_IMAGED ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "SAMPLED_IMAGE_FILTER_LINEAR ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "STORAGE_IMAGE ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "STORAGE_IMAGE_ATOMIC ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "UNIFORM_TEXEL_BUFFER ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER_ATOMIC ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "VERTEX_BUFFER ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "COLOR_ATTACHMENT ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "COLOR_ATTACHMENT_BLEND ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "DEPTH_STENCIL_ATTACHMENT ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "BLIT_SRC ";
//    }
//
//    if (formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT != 0) {
//        linearTilingFormatFeatureFlagsString += "BLIT_DST ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "SAMPLED_IMAGED ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "SAMPLED_IMAGE_FILTER_LINEAR ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "STORAGE_IMAGE ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "STORAGE_IMAGE_ATOMIC ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "UNIFORM_TEXEL_BUFFER ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_ATOMIC_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "STORAGE_TEXEL_BUFFER_ATOMIC ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "VERTEX_BUFFER ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "COLOR_ATTACHMENT ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "COLOR_ATTACHMENT_BLEND ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "DEPTH_STENCIL_ATTACHMENT ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "BLIT_SRC ";
//    }
//
//    if (formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT != 0) {
//        optimalTilingFormatFeatureFlagsString += "BLIT_DST ";
//    }
//#pragma endregion
//
//    std::string imageFormatString = ""; // INCOMPLETE
//    std::cout << "Physical Device format support features pertaining to format " << imageFormatString << std::endl;
//    std::cout << "\tBuffer format features flags: " << bufferFormatFeatureFlagsString << std::endl;
//    std::cout << "\tLinear tiling format feature flags: " << linearTilingFormatFeatureFlagsString << std::endl;
//    std::cout << "\tOptimal tiling format feature flags: " << optimalTilingFormatFeatureFlagsString << std::endl;
//
//    VkResult result = vkGetPhysicalDeviceImageFormatProperties(physicalDevices[0], VK_FORMAT_R8G8B8A8_UNORM,
//                                                               VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_LINEAR,
//                                                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
//                                                               VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT,
//                                                               &imageFormatProperties);
//    float maxResourceSizeGB;
//    VkSampleCountFlags sampleCountFlags;
//    uint32_t sampleCount;
//
//    switch (result) {
//        case VK_SUCCESS:
//            std::cout
//                    << "Physical Device support extent pertaining to image format R8G8B8A8_UNORM (2D) with optimal tiling usable as source and destination of transfer commands, allowing image view creation off itself:\n";
//
//            maxResourceSizeGB = ((float) imageFormatProperties.maxResourceSize) / (1024 * 1024 * 1024);
//
//            std::cout << "\tMax Array Layers: " << imageFormatProperties.maxArrayLayers << std::endl;
//            std::cout << "\tMax MimMap Levels: " << imageFormatProperties.maxMipLevels << std::endl;
//            std::cout << "\tMax Resource Size: " << maxResourceSizeGB << "GB" << std::endl;
//            std::cout << "\tMax Sample Count: " << imageFormatProperties.sampleCounts << std::endl;
//            break;
//        case VK_ERROR_OUT_OF_HOST_MEMORY:
//            throw VulkanException("Couldn't fetch image format properties, out of host memory.");
//            break;
//        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
//            throw VulkanException("Couldn't fetch image format properties, out of device memory.");
//            break;
//        case VK_ERROR_FORMAT_NOT_SUPPORTED:
//            throw VulkanException("Format not supported.");
//            break;
//    }
//}

//void VulkanEngine Engine::createImageView() {
//	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//	imageViewCreateInfo.pNext = nullptr;
//	imageViewCreateInfo.flags = 0;
//	imageViewCreateInfo.image = depthImage[0];
//	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//	imageViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
//	imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
//	imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
//	imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
//	imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
//	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
//	imageViewCreateInfo.subresourceRange.levelCount = 1;
//	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
//	imageViewCreateInfo.subresourceRange.layerCount = 1;
//
//	imageViews = new VkImageView[1];
//
//	VkResult result = vkCreateImageView(logicalDevices[0], &imageViewCreateInfo, &imageViewCreationCallbacks, imageViews);
//
//	switch (result) {
//	case VK_SUCCESS:
//		std::cout << "Image View creation succeeded.\n";
//		break;
//	default:
//		throw VulkanException("Image View creation failed.");
//	}
//}

//void VulkanEngine Engine::allocateDeviceMemories() {
//	VkMemoryAllocateInfo deviceMemoryAllocateInfo;
//
//	for (uint16_t uniformBufferIndex = 0; uniformBufferIndex < cachedScene->mNumMeshes * 3; uniformBufferIndex += 3) {
//		deviceMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//		deviceMemoryAllocateInfo.pNext = nullptr;
//		deviceMemoryAllocateInfo.allocationSize = totalUniformBufferSize;
//		deviceMemoryAllocateInfo.memoryTypeIndex = hostVisibleMemoryTypeIndex;
//
//		VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &deviceMemoryAllocateInfo, nullptr, bufferMemories + uniformBufferIndex + 0));
//
//
//		deviceMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//		deviceMemoryAllocateInfo.pNext = nullptr;
//		deviceMemoryAllocateInfo.allocationSize = vertexBufferSizes[uniformBufferIndex / 3];
//		deviceMemoryAllocateInfo.memoryTypeIndex = hostVisibleMemoryTypeIndex;
//
//		VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &deviceMemoryAllocateInfo, nullptr, bufferMemories + uniformBufferIndex + 1) );
//
//		deviceMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//		deviceMemoryAllocateInfo.pNext = nullptr;
//		deviceMemoryAllocateInfo.allocationSize = indexBufferSizes[uniformBufferIndex / 3];
//		deviceMemoryAllocateInfo.memoryTypeIndex = hostVisibleMemoryTypeIndex;
//
//		VKASSERT_SUCCESS(vkAllocateMemory(logicalDevices[0], &deviceMemoryAllocateInfo, nullptr, bufferMemories + uniformBufferIndex + 2));
//	}
//}

//void VulkanEngine Engine::writeBuffers() {
//	void *mappedMemory;
//
//
//	for (uint16_t uniformBufferIndex = 0; uniformBufferIndex < cachedScene->mNumMeshes * 3; uniformBufferIndex += 3) {
//		VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex], 0, totalUniformBufferSize, 0, &mappedMemory));
//
//		float xRotation = (3.1415926536f / 180.0f) * 180;
//		float yRotation = (3.1415926536f / 180.0f) * 90;
//
//		ModelMatrix modelMatrix;
//		float rotationMatrices[2][16];
//		float translationMatrices[2][16];
//
//		rotationMatrices[0][0] = 1.0f;		rotationMatrices[0][4] = 0.0f;			rotationMatrices[0][8] = 0.0f;			rotationMatrices[0][12] = 0.0f;
//		rotationMatrices[0][1] = 0.0f;		rotationMatrices[0][5] = cosf(xRotation);	rotationMatrices[0][9] = -sinf(xRotation);	rotationMatrices[0][13] = 0.0f;
//		rotationMatrices[0][2] = 0.0f;		rotationMatrices[0][6] = sinf(xRotation);	rotationMatrices[0][10] = cosf(xRotation);	rotationMatrices[0][14] = 0.0f;
//		rotationMatrices[0][3] = 0.0f;		rotationMatrices[0][7] = 0.0f;			rotationMatrices[0][11] = 0.0f;			rotationMatrices[0][15] = 1.0f;
//
//		rotationMatrices[1][0] = cosf(yRotation);	rotationMatrices[1][4] = 0.0f;	rotationMatrices[1][8] = sinf(yRotation);	rotationMatrices[1][12] = 0.0f;
//		rotationMatrices[1][1] = 0.0f;				rotationMatrices[1][5] = 1.0f;	rotationMatrices[1][9] = 0.0f;				rotationMatrices[1][13] = 0.0f;
//		rotationMatrices[1][2] = -sinf(yRotation);	rotationMatrices[1][6] = 0.0f;	rotationMatrices[1][10] = cosf(yRotation);	rotationMatrices[1][14] = 0.0f;
//		rotationMatrices[1][3] = 0.0f;				rotationMatrices[1][7] = 0.0f;	rotationMatrices[1][11] = 0.0f;				rotationMatrices[1][15] = 1.0f;
//
//		multiplyMatrix<float>(modelMatrix.modelMatrix, rotationMatrices[0], rotationMatrices[1]);
//
//		memcpy(mappedMemory, &modelMatrix, totalUniformBufferSize);
//
//		memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
//		memoryFlushRange.pNext = nullptr;
//		memoryFlushRange.size = VK_WHOLE_SIZE;
//		memoryFlushRange.offset = 0;
//		memoryFlushRange.memory = bufferMemories[uniformBufferIndex];
//
//		VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));
//
//		vkUnmapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex]);
//
//
//		VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex + 1], 0, vertexBufferSizes[uniformBufferIndex / 3], 0, &mappedMemory));
//
//		memcpy(mappedMemory, sortedAttributes[uniformBufferIndex / 3].data(), vertexBufferSizes[uniformBufferIndex / 3]);
//
//		memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
//		memoryFlushRange.pNext = nullptr;
//		memoryFlushRange.size = VK_WHOLE_SIZE;
//		memoryFlushRange.offset = 0;
//		memoryFlushRange.memory = bufferMemories[uniformBufferIndex + 1];
//
//		VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));
//
//		vkUnmapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex + 1]);
//
//
//
//
//		VKASSERT_SUCCESS(vkMapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex + 2], 0, indexBufferSizes[uniformBufferIndex / 3], 0, &mappedMemory));
//
//		memcpy(mappedMemory, sortedIndices[uniformBufferIndex / 3].data(), indexBufferSizes[uniformBufferIndex / 3]);
//
//		memoryFlushRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
//		memoryFlushRange.pNext = nullptr;
//		memoryFlushRange.size = VK_WHOLE_SIZE;
//		memoryFlushRange.offset = 0;
//		memoryFlushRange.memory = bufferMemories[uniformBufferIndex + 2];
//
//		VKASSERT_SUCCESS(vkFlushMappedMemoryRanges(logicalDevices[0], 1, &memoryFlushRange));
//
//		vkUnmapMemory(logicalDevices[0], bufferMemories[uniformBufferIndex + 2]);
//
//
//	}
//}
//
//void VulkanEngine Engine::bindBufferMemories() {
//
//	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
//		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], buffers[(meshIndex * 3) + 0], bufferMemories[(meshIndex * 3) + 0], 0));
//
//		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], buffers[(meshIndex * 3) + 1], bufferMemories[(meshIndex * 3) + 1], 0));
//
//		VKASSERT_SUCCESS(vkBindBufferMemory(logicalDevices[0], buffers[(meshIndex * 3) + 2], bufferMemories[(meshIndex * 3) + 2], 0));
//	}
//}

void VulkanEngine::createSparseImage() {
	sparseImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	sparseImageCreateInfo.pNext = nullptr;
	sparseImageCreateInfo.flags = VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT | VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
	sparseImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	sparseImageCreateInfo.format = VK_FORMAT_R32G32B32A32_UINT;
	sparseImageCreateInfo.extent.width = 4096;
	sparseImageCreateInfo.extent.height = 4096;
	sparseImageCreateInfo.extent.depth = 1;
	sparseImageCreateInfo.arrayLayers = 1;
	sparseImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	sparseImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	sparseImageCreateInfo.usage =
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	sparseImageCreateInfo.mipLevels = imageFormatProperties.maxMipLevels;
	sparseImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	sparseImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	sparseImageCreateInfo.queueFamilyIndexCount = 0;
	sparseImageCreateInfo.pQueueFamilyIndices = nullptr;

	sparseImages = new VkImage[1];

	NULL_HANDLE_INIT_ARRAY(sparseImages, 1)

		VkResult result = vkCreateImage(logicalDevices[0], &sparseImageCreateInfo, nullptr, sparseImages);

	switch (result) {
	case VK_SUCCESS:
		std::cout << "Sparse Image (" << 2048 << "x" << 2048
			<< ", with different other parameters) created successfully.\n";
		break;
	default:
		throw VulkanException("Sparse Image creation failed.");
	}

	vkGetImageSparseMemoryRequirements(logicalDevices[0], sparseImages[0], &sparseMemoryRequirementsCount, nullptr);

	sparseImageMemoryRequirements = new VkSparseImageMemoryRequirements[sparseMemoryRequirementsCount];

	vkGetImageSparseMemoryRequirements(logicalDevices[0], sparseImages[0], &sparseMemoryRequirementsCount,
		sparseImageMemoryRequirements);

	std::cout << "Sparse Image Memory Requirements:\n";

	for (uint32_t i = 0; i < sparseMemoryRequirementsCount; i++) {
		VkSparseImageMemoryRequirements sparseImageMemoryRequirementsElement = sparseImageMemoryRequirements[i];

		std::string aspectMaskString = "";

		if ((sparseImageMemoryRequirementsElement.formatProperties.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != 0) {
			aspectMaskString += "COLOR_BIT ";
		}

		if ((sparseImageMemoryRequirementsElement.formatProperties.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != 0) {
			aspectMaskString += "DEPTH_BIT ";
		}

		if ((sparseImageMemoryRequirementsElement.formatProperties.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0) {
			aspectMaskString += "STENCIL_BIT ";
		}

		if ((sparseImageMemoryRequirementsElement.formatProperties.aspectMask & VK_IMAGE_ASPECT_METADATA_BIT) != 0) {
			aspectMaskString += "METADATA_BIT ";
		}

		std::cout << "\tApplied to following aspects: " << aspectMaskString << std::endl;

		std::cout << "\tImage Granularity Width: "
			<< sparseImageMemoryRequirementsElement.formatProperties.imageGranularity.width << std::endl;
		std::cout << "\tImage Granularity Height: "
			<< sparseImageMemoryRequirementsElement.formatProperties.imageGranularity.height << std::endl;
		std::cout << "\tImage Granularity Depth: "
			<< sparseImageMemoryRequirementsElement.formatProperties.imageGranularity.depth << std::endl;

		std::string flagsString = "";

		if ((sparseImageMemoryRequirementsElement.formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) !=
			0) {
			flagsString += "SINGLE_MIPTAIL ";
		}

		if ((sparseImageMemoryRequirementsElement.formatProperties.flags &
			VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT) != 0) {
			flagsString += "ALIGNED_MIP_SIZE ";
		}

		if ((sparseImageMemoryRequirementsElement.formatProperties.flags &
			VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT) != 0) {
			flagsString += "NONSTANDARD_BLOCK_SIZE ";
		}

		std::cout << "\tFlags: " << flagsString << std::endl;

		std::cout << "\tFirst Mip-Tail Level: " << sparseImageMemoryRequirementsElement.imageMipTailFirstLod
			<< std::endl;
		std::cout << "\tFirst Mip-Tail Size: " << sparseImageMemoryRequirementsElement.imageMipTailSize << std::endl;
		std::cout << "\tFirst Mip-Tail Offset (in memory binding region): "
			<< sparseImageMemoryRequirementsElement.imageMipTailOffset << std::endl;
		std::cout << "\tFirst Mip-Tail Stride (between deviant miptails of array): "
			<< sparseImageMemoryRequirementsElement.imageMipTailStride << std::endl;

	}
}

void VulkanEngine::getPhysicalDeviceSparseImageFormatProperties() {
	VkImageUsageFlags usageFlags =
		VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	VkFormat format = VK_FORMAT_R16G16B16A16_UNORM;
	VkSampleCountFlagBits samplesCount = VK_SAMPLE_COUNT_1_BIT;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkImageType type = VK_IMAGE_TYPE_2D;


	vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevices[0], format, type, samplesCount, usageFlags, tiling,
		&physicalDeviceSparseImageFormatPropertiesCount, nullptr);

	physicalDeviceSparseImageFormatProperties = new VkSparseImageFormatProperties[physicalDeviceSparseImageFormatPropertiesCount];

	vkGetPhysicalDeviceSparseImageFormatProperties(physicalDevices[0], format, type, samplesCount, usageFlags, tiling,
		&physicalDeviceSparseImageFormatPropertiesCount,
		physicalDeviceSparseImageFormatProperties);

	std::cout
		<< "Physical Device support extent pertaining to sparse image format VK_FORMAT_R16G16B16A16_UNORM(2D) with optimal tiling usable as source and destination of transfer commands, as well as sampling, and allowing image view creation off itself, with one multisampling:\n";

	for (uint32_t i = 0; i < physicalDeviceSparseImageFormatPropertiesCount; i++) {

		std::string aspectMaskString = "";

		if ((physicalDeviceSparseImageFormatProperties[i].aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != 0) {
			aspectMaskString += "COLOR_BIT ";
		}

		if ((physicalDeviceSparseImageFormatProperties[i].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != 0) {
			aspectMaskString += "DEPTH_BIT ";
		}

		if ((physicalDeviceSparseImageFormatProperties[i].aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0) {
			aspectMaskString += "STENCIL_BIT ";
		}

		if ((physicalDeviceSparseImageFormatProperties[i].aspectMask & VK_IMAGE_ASPECT_METADATA_BIT) != 0) {
			aspectMaskString += "METADATA_BIT ";
		}

		std::cout << "\tApplied to following aspects: " << aspectMaskString << std::endl;

		std::cout << "\tImage Granularity Width: "
			<< physicalDeviceSparseImageFormatProperties[i].imageGranularity.width << std::endl;
		std::cout << "\tImage Granularity Height: "
			<< physicalDeviceSparseImageFormatProperties[i].imageGranularity.height << std::endl;
		std::cout << "\tImage Granularity Depth: "
			<< physicalDeviceSparseImageFormatProperties[i].imageGranularity.depth << std::endl;

		std::string flagsString = "";

		if ((physicalDeviceSparseImageFormatProperties[i].flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) != 0) {
			flagsString += "SINGLE_MIPTAIL ";
		}

		if ((physicalDeviceSparseImageFormatProperties[i].flags & VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT) != 0) {
			flagsString += "ALIGNED_MIP_SIZE ";
		}

		if ((physicalDeviceSparseImageFormatProperties[i].flags & VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT) !=
			0) {
			flagsString += "NONSTANDARD_BLOCK_SIZE ";
		}

		std::cout << "\tFlags: " << flagsString << std::endl;
	}
}

void VulkanEngine::getQueues() {
	vkGetDeviceQueue(logicalDevices[0], graphicsQueueFamilyIndex, 0, &graphicsQueue);

	if (graphicsQueue == VK_NULL_HANDLE) {
		throw VulkanException("Couldn't obtain graphics queue from device.");
	}
	else {
		std::cout << "Graphics Queue obtained successfully.\n";
	}

	vkGetDeviceQueue(logicalDevices[0], transferQueueFamilyIndex, 0, &transferQueue);

	if (transferQueue == VK_NULL_HANDLE) {
		throw VulkanException("Couldn't obtain transfer queue from device.");
	}
	else {
		std::cout << "Transfer Queue obtained successfully.\n";
	}
}

void VulkanEngine::getQueueFamilyPresentationSupport() {
	if (vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevices[0], graphicsQueueFamilyIndex) == VK_TRUE) {
		std::cout << "Selected Graphical Queue Family supports presentation." << std::endl;
	}
	else
		throw VulkanException("Selected queue family (graphical) doesn't support presentation.");
}

void VulkanEngine::createSurface() {
	VkResult result;

	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.pNext = nullptr;
	surfaceCreateInfo.flags = 0;
	surfaceCreateInfo.hinstance = hInstance;
	surfaceCreateInfo.hwnd = windowHandle;

	result = vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface);

	if (result == VK_SUCCESS) {
		std::cout << "Surface created and associated with the window." << std::endl;
	}
	else
		throw VulkanException("Failed to create and/or assocate surface with the window");
}

void VulkanEngine::createSwapchain() {
	VkResult surfaceSupportResult = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[0], graphicsQueueFamilyIndex,
		surface, &physicalDeviceSurfaceSupported);

	if (surfaceSupportResult == VK_SUCCESS && physicalDeviceSurfaceSupported == VK_TRUE) {
		std::cout << "Physical Device selected graphical queue family supports presentation." << std::endl;
	}
	else
		throw VulkanException("Physical Device selected graphical queue family doesn't support presentation.");

	VkResult surfaceCapabilitiesResult = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[0], surface,
		&surfaceCapabilities);

	if (surfaceCapabilitiesResult == VK_SUCCESS) {
		std::cout << "Successfully fetched device surface capabilities." << std::endl;

		std::cout << "Minimum swap chain image count: " << surfaceCapabilities.minImageCount << std::endl;
		std::cout << "Maximum swap chain image count: " << surfaceCapabilities.maxImageCount << std::endl;
	}
	else
		throw VulkanException("Couldn't fetch device surface capabilities.");

	if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[0], surface, &surfaceSupportedFormatsCount, nullptr) !=
		VK_SUCCESS)
		throw VulkanException("Couldn't get surface supported formats.");

	surfaceSupportedFormats = new VkSurfaceFormatKHR[surfaceSupportedFormatsCount];

	if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[0], surface, &surfaceSupportedFormatsCount,
		surfaceSupportedFormats) != VK_SUCCESS)
		throw VulkanException("Couldn't get surface supported formats.");


	uint32_t supportedFormatColorSpacePairIndex = -1;
	uint32_t supportedPresentModeIndex = -1;

	//    for (int i = 0; i < surfaceSupportedFormatsCount; i++) {
	//        if (surfaceSupportedFormats[i].format == VK_FORMAT_) {
	//            supportedFormatColorSpacePairIndex = i;
	//            break;
	//        }
	//    }

	supportedFormatColorSpacePairIndex = 0; // Selecing the first supported format

	if (supportedFormatColorSpacePairIndex == -1)
		throw VulkanException("Couldn't find R8G8B8A8_UNORM format in supported formats.");
	else {
		std::string formatString = "";

		surfaceImageFormat = surfaceSupportedFormats[supportedFormatColorSpacePairIndex].format;

		switch (surfaceSupportedFormats[supportedFormatColorSpacePairIndex].format) {
		case VK_FORMAT_R8G8B8A8_UNORM:
			formatString = "R8G8B8A8_UNORM";
			break;
		case VK_FORMAT_R8G8B8A8_UINT:
			formatString = "R8G8B8A8_UINT";
			break;
		case VK_FORMAT_R8G8B8A8_USCALED:
			formatString = "R8G8B8A8_USCALED";
			break;
		case VK_FORMAT_R8G8B8A8_SSCALED:
			formatString = "R8G8B8A8_SSCALED";
			break;
		case VK_FORMAT_R8G8B8A8_SRGB:
			formatString = "R8G8B8A8_SRGB";
			break;
		case VK_FORMAT_R8G8B8A8_SNORM:
			formatString = "R8G8B8A8_SNORM";
			break;
		case VK_FORMAT_R8G8B8A8_SINT:
			formatString = "R8G8B8A8_SINT";
			break;
		}

		std::cout << "Selected surface image format: " << formatString << std::endl;
	}

	if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[0], surface, &surfaceSupportedPresentModesCount,
		nullptr) != VK_SUCCESS) {
		throw VulkanException("Couldn't get surface supported presentation modes.");
	}

	surfaceSupportedPresentModes = new VkPresentModeKHR[surfaceSupportedPresentModesCount];

	if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevices[0], surface, &surfaceSupportedPresentModesCount,
		surfaceSupportedPresentModes) != VK_SUCCESS) {
		throw VulkanException("Couldn't get surface supported presentation modes.");
	}
	surfacePresentMode = (verticalSyncEnabled) ? (VK_PRESENT_MODE_FIFO_KHR) : (VK_PRESENT_MODE_IMMEDIATE_KHR);

	if (surfaceSupportedPresentModesCount == -1)
		throw VulkanException("Couldn't find IMMEDIATE present mode in supported present modes.");

	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = (2 >= surfaceCapabilities.minImageCount &&
		2 <= surfaceCapabilities.maxImageCount) ? (2)
		: (surfaceCapabilities.minImageCount);
	swapchainCreateInfo.imageFormat = surfaceSupportedFormats[supportedFormatColorSpacePairIndex].format;
	swapchainCreateInfo.imageColorSpace = surfaceSupportedFormats[supportedFormatColorSpacePairIndex].colorSpace;
	swapchainCreateInfo.imageExtent.width = surfaceCapabilities.currentExtent.width;
	swapchainCreateInfo.imageExtent.height = surfaceCapabilities.currentExtent.height;
	swapchainCreateInfo.imageArrayLayers = 1;

	if ((surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
		std::cout << "Surface supports COLOR_ATTACHMENT usage bits." << std::endl;
	}
	else
		throw VulkanException("Surface doesn't support COLOR_ATTACHMENT usage bits.");

	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = surfacePresentMode;
	swapchainCreateInfo.clipped = VK_TRUE;

	VkResult result = vkCreateSwapchainKHR(logicalDevices[0], &swapchainCreateInfo, nullptr, &swapchain);

	if (result == VK_SUCCESS) {
		std::cout << "Swapchain created successfully." << std::endl;
	}
	else
		throw VulkanException("Swapchain creation failed.");

	VkResult swapchainImageResult;

	if ((swapchainImageResult = vkGetSwapchainImagesKHR(logicalDevices[0], swapchain, &swapchainImagesCount,
		nullptr)) != VK_SUCCESS)
		throw VulkanException("Couldn't get swapchain images.");

	swapchainImages = new VkImage[swapchainImagesCount];

	NULL_HANDLE_INIT_ARRAY(swapchainImages, swapchainImagesCount)

		if ((swapchainImageResult = vkGetSwapchainImagesKHR(logicalDevices[0], swapchain, &swapchainImagesCount,
			swapchainImages)) == VK_SUCCESS) {
			std::cout << "Swapchain images obtained successfully." << std::endl;
		}
		else
			throw VulkanException("Couldn't get swapchain images.");
}

uint32_t VulkanEngine::acquireNextFramebufferImageIndex() {
	uint32_t imageIndex = -1;

	//if (pAcquireNextImageIndexFence == NULL) {
	//	pAcquireNextImageIndexFence = new VkFence;

	//	VkFenceCreateInfo fenceCreateInfo = {};

	//	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	//	fenceCreateInfo.pNext = nullptr;
	//	fenceCreateInfo.flags = 0;

	//	VkResult createFenceResult = vkCreateFence(logicalDevices[0], &fenceCreateInfo, nullptr, pAcquireNextImageIndexFence);
	//}

	//vkResetFences(logicalDevices[0], 1, pAcquireNextImageIndexFence);

	vkDeviceWaitIdle(logicalDevices[0]);

	VkResult acquireNextImageIndexResult = vkAcquireNextImageKHR(logicalDevices[0], swapchain, UINT64_MAX,
		indexAcquiredSemaphore, VK_NULL_HANDLE, &imageIndex);

	//vkDestroyFence(logicalDevices[0], *pAcquireNextImageIndexFence, nullptr);

	//pAcquireNextImageIndexFence = NULL;

	//std::cout << "Next available swapchain image index acquired." << std::endl;

	return imageIndex;
}


void VulkanEngine::createQueueDoneFence() {
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;
	fenceCreateInfo.flags = 0;

	VKASSERT_SUCCESS(vkCreateFence(logicalDevices[0], &fenceCreateInfo, nullptr, &queueDoneFence));
}

void VulkanEngine::present(uint32_t swapchainPresentImageIndex) {
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.swapchainCount = 1;
	presentInfo.pImageIndices = &swapchainPresentImageIndex;
	presentInfo.pResults = nullptr;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &waitToPresentSemaphore;

	VkResult result = vkQueuePresentKHR(graphicsQueue, &presentInfo);


	if (result == VK_SUBOPTIMAL_KHR)
		std::cout
		<< "Image presentation command successfully submitted to queue, but the swapchain is not longer optimal for the target surface."
		<< std::endl;
	else if (result != VK_SUCCESS)
		throw VulkanException("Failed to present.");

	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(logicalDevices[0], renderCommandPool, 1, &renderCommandBuffer);
}

void VulkanEngine::createRenderpass() {
	VkAttachmentDescription attachments[2];
	attachments[0].flags = 0;
	attachments[0].format = surfaceImageFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachments[1].flags = 0;
	attachments[1].format = depthFormat;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};


	VkAttachmentReference colorAttachment = {};
	colorAttachment.attachment = 0;
	colorAttachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthStencilAttachment = {};
	depthStencilAttachment.attachment = 1;
	depthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pDepthStencilAttachment = &depthStencilAttachment;
	subpass.pColorAttachments = &colorAttachment;
	subpass.pResolveAttachments = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;


	VkSubpassDependency subpassDependencies[1];

	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;                                // Producer of the dependency
	subpassDependencies[0].dstSubpass = 0;                                                    // Consumer is our single subpass that will wait for the execution depdendency
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	//// Second dependency at the end the renderpass
	//// Does the transition from the initial to the final layout
	//subpassDependencies[1].srcSubpass = 0;													// Producer of the dependency is our single subpass
	//subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;								// Consumer are all commands outside of the renderpass
	//subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	//subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	//subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	//subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	//subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = subpassDependencies;

	VkResult result = vkCreateRenderPass(logicalDevices[0], &renderPassCreateInfo, nullptr, &renderPass);

	if (result == VK_SUCCESS)
		std::cout << "Renderpass created successfully." << std::endl;
	else
		throw VulkanException("Couldn't create Renderpass.");
}

void VulkanEngine::createFramebuffers() {
	framebuffers = new VkFramebuffer[swapchainImagesCount];

	NULL_HANDLE_INIT_ARRAY(framebuffers, swapchainImagesCount)

		for (uint32_t i = 0; i < swapchainImagesCount; i++) {
			VkImageView attachments[2] = { swapchainImageViews[i], depthImageView };

			VkFramebufferCreateInfo framebufferCreateInfo = {};

			framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferCreateInfo.pNext = nullptr;
			framebufferCreateInfo.flags = 0;
			framebufferCreateInfo.renderPass = renderPass;
			framebufferCreateInfo.attachmentCount = 2;
			framebufferCreateInfo.pAttachments = attachments;
			framebufferCreateInfo.width = surfaceCapabilities.currentExtent.width;
			framebufferCreateInfo.height = surfaceCapabilities.currentExtent.height;
			framebufferCreateInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(logicalDevices[0], &framebufferCreateInfo, nullptr, framebuffers + i);

			if (result != VK_SUCCESS)
				throw VulkanException("Couldn't create framebuffer.");
		}

	std::cout << "Framebuffer(s) created successfully." << std::endl;
}

void VulkanEngine::createSwapchainImageViews() {

	swapchainImageViews = new VkImageView[swapchainImagesCount];

	NULL_HANDLE_INIT_ARRAY(swapchainImageViews, swapchainImagesCount)

		for (uint32_t i = 0; i < swapchainImagesCount; i++) {
			VkImage swapchainImage = swapchainImages[i];

			VkImageViewCreateInfo swapchainImageViewCreateInfo = {};
			swapchainImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			swapchainImageViewCreateInfo.pNext = nullptr;
			swapchainImageViewCreateInfo.flags = 0;
			swapchainImageViewCreateInfo.image = swapchainImages[i];
			swapchainImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			swapchainImageViewCreateInfo.format = surfaceImageFormat;
			swapchainImageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
			swapchainImageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
			swapchainImageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
			swapchainImageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
			swapchainImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			swapchainImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			swapchainImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			swapchainImageViewCreateInfo.subresourceRange.layerCount = 1;
			swapchainImageViewCreateInfo.subresourceRange.levelCount = 1;

			VkResult result = vkCreateImageView(logicalDevices[0], &swapchainImageViewCreateInfo, nullptr,
				swapchainImageViews + i);

			if (result == VK_SUCCESS)
				std::cout << "ImageView for swap chain image (" << i << ") created successfully." << std::endl;
			else
				throw VulkanException("ImageView creation failed.");
		}

}

void VulkanEngine::createGraphicsPipeline() {
	VkPipelineShaderStageCreateInfo stageCreateInfos[2];

	stageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageCreateInfos[0].pNext = nullptr;
	stageCreateInfos[0].flags = 0;
	stageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stageCreateInfos[0].module = graphicsVertexShaderModule;
	stageCreateInfos[0].pName = u8"main";
	stageCreateInfos[0].pSpecializationInfo = nullptr;

	stageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageCreateInfos[1].pNext = nullptr;
	stageCreateInfos[1].flags = 0;
	stageCreateInfos[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stageCreateInfos[1].module = graphicsFragmentShaderModule;
	stageCreateInfos[1].pName = u8"main";
	stageCreateInfos[1].pSpecializationInfo = nullptr;

	vertexBindingDescription.binding = 0;
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexBindingDescription.stride = sizeof(Attribute<float>);

	VkVertexInputAttributeDescription vertexAttributeDescriptions[5];

	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[0].offset = offsetof(Attribute<float>, position);

	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[1].offset = offsetof(Attribute<float>, normal);

	vertexAttributeDescriptions[2].binding = 0;
	vertexAttributeDescriptions[2].location = 2;
	vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions[2].offset = offsetof(Attribute<float>, uv);

	vertexAttributeDescriptions[3].binding = 0;
	vertexAttributeDescriptions[3].location = 3;
	vertexAttributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[3].offset = offsetof(Attribute<float>, tangent);

	vertexAttributeDescriptions[4].binding = 0;
	vertexAttributeDescriptions[4].location = 4;
	vertexAttributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[4].offset = offsetof(Attribute<float>, bitangent);

	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.pNext = nullptr;
	vertexInputStateCreateInfo.flags = 0;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 5;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;

	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.pNext = nullptr;
	inputAssemblyStateCreateInfo.flags = 0;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	viewport.width = static_cast<float>(swapchainCreateInfo.imageExtent.width);
	viewport.height = static_cast<float>(swapchainCreateInfo.imageExtent.height);
	viewport.x = 0;
	viewport.y = 0;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = std::lroundf(viewport.width);
	scissor.extent.height = std::lroundf(viewport.height);

	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.pNext = nullptr;
	viewportStateCreateInfo.flags = 0;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.pViewports = &viewport;
	viewportStateCreateInfo.scissorCount = 1;
	viewportStateCreateInfo.pScissors = &scissor;

	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationStateCreateInfo.pNext = nullptr;
	rasterizationStateCreateInfo.flags = 0;
	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE; // no back-face/front-face culling!
	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	rasterizationStateCreateInfo.lineWidth = 1.0f;


	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleStateCreateInfo.pNext = nullptr;
	multisampleStateCreateInfo.flags = 0;
	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleStateCreateInfo.pSampleMask = nullptr;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_MAX;
	colorBlendAttachmentState.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateCreateInfo.pNext = nullptr;
	colorBlendStateCreateInfo.flags = 0;
	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateCreateInfo.attachmentCount = 1;
	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCreateInfo.pNext = nullptr;
	depthStencilStateCreateInfo.flags = 0;
	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.pNext = nullptr;
	graphicsPipelineCreateInfo.flags = 0;
	graphicsPipelineCreateInfo.stageCount = 2;
	graphicsPipelineCreateInfo.pStages = stageCreateInfos;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	graphicsPipelineCreateInfo.pTessellationState = nullptr;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = nullptr;
	graphicsPipelineCreateInfo.layout = graphicsPipelineLayout;
	graphicsPipelineCreateInfo.renderPass = renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	graphicsPipelineCreateInfo.basePipelineIndex = -1;

	VkResult result = vkCreateGraphicsPipelines(logicalDevices[0], VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo,
		nullptr, &graphicsPipeline);

	if (result == VK_SUCCESS)
		std::cout << "Graphics Pipeline created successfully." << std::endl;
	else
		throw VulkanException("Couldn't create graphics pipeline.");
}

//typedef struct shaderc_compiler* shaderc_compiler_t;
//typedef struct shaderc_compilation_result* shaderc_compilation_result_t;
//typedef shaderc_compiler_t (*_shaderc_compiler_initialize_t)();
//typedef shaderc_compile_into_spv

void VulkanEngine::writeShaderBinaryToFile(const void* shaderBinary, size_t shaderBinarySize, const char* fileName)
{
	FILE* fp;
	fp = fopen(fileName, "wb");
	assert(fp);

	fwrite(shaderBinary, 1, shaderBinarySize, fp);

	fclose(fp);
}

size_t VulkanEngine::readShaderBinaryFromFile(void*& shaderBinary, const char* fileName) {
	size_t readSize = 0;

	FILE* fp;
	fp = fopen(fileName, "rb");
	assert(fp);

	fseek(fp, 0L, SEEK_END);
	readSize = ftell(fp);
	rewind(fp);

	shaderBinary = new char[readSize];

	fread(shaderBinary, 1, readSize, fp);

	fclose(fp);

	return readSize;
}

bool VulkanEngine::checkFileExist(const char* fileName) {
	FILE* fp = fopen(fileName, "r");
	bool is_exist = false;
	if (fp != NULL)
	{
		is_exist = true;
		fclose(fp); // close the file
	}
	return is_exist;
}

void VulkanEngine::createGraphicsShaderModule(const char* shaderFileName, VkShaderModule* shaderModule,
	shaderc_shader_kind shaderType) {


	std::string shaderPath(resourcesPath);
	shaderPath.append(shaderFileName);
	std::string compiledShaderFileName(shaderFileName);
	compiledShaderFileName.append(".compiled");

	bool compileShader = !checkFileExist(compiledShaderFileName.c_str());

	if (compileShader) {
		std::string shaderCode = loadShaderCode(shaderPath.c_str());

		try {
			std::vector<uint32_t> shaderBinaryVector = compileGLSLShader(shaderCode.c_str(), shaderType);
			writeShaderBinaryToFile(reinterpret_cast<const void*>(shaderBinaryVector.data()), shaderBinaryVector.size() * sizeof(uint32_t), compiledShaderFileName.c_str());
		}
		catch (VulkanException e) {
			std::string errorText = "shaderc: Couldn't compile shader file: ";
			errorText.append(getCurrentPath());
			errorText.append(shaderPath);
			errorText.append("\n\t");
			errorText.append(e.what());
			throw VulkanException(errorText.c_str());
		}

	}

	void* shaderBinary = nullptr;
	size_t shaderBinaryAbsoluteSize = 0;

	shaderBinaryAbsoluteSize = readShaderBinaryFromFile(shaderBinary, compiledShaderFileName.c_str());

	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = nullptr;
	shaderModuleCreateInfo.codeSize = shaderBinaryAbsoluteSize;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderBinary);

	if (vkCreateShaderModule(logicalDevices[0], &shaderModuleCreateInfo, nullptr, shaderModule) ==
		VK_SUCCESS) {
		std::cout << "Graphics Shader Module created successfully." << std::endl;
	}
	else
		throw VulkanException("Couldn't create vertex graphics shader module.");

}

std::string VulkanEngine::loadShaderCode(const char* fileName) {
	if (!checkFileExist(fileName))
	{
		std::string exceptionWhat = "Couldn't open shader file ";
		exceptionWhat.append(getCurrentPath());
		exceptionWhat.append(fileName);
		throw VulkanException(exceptionWhat.c_str());
	}

	std::string line;
	std::string code = "";
	std::ifstream infile;
	infile.open(fileName);
	while (!infile.eof()) // To get you all the lines.
	{
		getline(infile, line); // Saves the line in STRING.

		code.append(line);
		code.append("\n");
	}
	infile.close();

	return code;
}

//void VulkanEngine::createGeometryGraphicsShaderModule() {
//
//    void *pShaderData;
//    DWORD shaderSize = loadShaderCode(".\\Resources\\geom.spv", (char **) &pShaderData);
//
//    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//    shaderModuleCreateInfo.pNext = nullptr;
//    shaderModuleCreateInfo.codeSize = shaderSize;
//    shaderModuleCreateInfo.flags = 0;
//    shaderModuleCreateInfo.pCode = (uint32_t *) pShaderData;
//
//    if (vkCreateShaderModule(logicalDevices[0], &shaderModuleCreateInfo, nullptr, &graphicsGeometryShaderModule) ==
//        VK_SUCCESS) {
//        std::cout << "Graphics Shader Module created successfully." << std::endl;
//    } else
//        throw VulkanException("Couldn't create fragment graphics shader module.");
//}
//
//void VulkanEngine::createFragmentGraphicsShaderModule() {
//    void *pShaderData;
//    DWORD shaderSize = loadShaderCode(".\\Resources\\frag.spv", (char **) &pShaderData);
//
//    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//    shaderModuleCreateInfo.pNext = nullptr;
//    shaderModuleCreateInfo.codeSize = shaderSize;
//    shaderModuleCreateInfo.flags = 0;
//    shaderModuleCreateInfo.pCode = (uint32_t *) pShaderData;
//
//    if (vkCreateShaderModule(logicalDevices[0], &shaderModuleCreateInfo, nullptr, &graphicsFragmentShaderModule) ==
//        VK_SUCCESS) {
//        std::cout << "Graphics Shader Module created successfully." << std::endl;
//    } else
//        throw VulkanException("Couldn't create fragment graphics shader module.");
//}

void VulkanEngine::createPipelineAndDescriptorSetsLayout() {
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};

	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[4];
	descriptorSetLayoutBindings[0].binding = 0;
	descriptorSetLayoutBindings[0].descriptorCount = 1;
	descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;
	descriptorSetLayoutBindings[0].stageFlags =
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	descriptorSetLayoutBindings[1].binding = 1;
	descriptorSetLayoutBindings[1].descriptorCount = 1;
	descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;
	descriptorSetLayoutBindings[1].stageFlags =
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	descriptorSetLayoutBindings[2].binding = 2;
	descriptorSetLayoutBindings[2].descriptorCount = 1;
	descriptorSetLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings[2].pImmutableSamplers = nullptr;
	descriptorSetLayoutBindings[2].stageFlags =
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	descriptorSetLayoutBindings[3].binding = 3;
	descriptorSetLayoutBindings[3].descriptorCount = 1;
	descriptorSetLayoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorSetLayoutBindings[3].pImmutableSamplers = nullptr;
	descriptorSetLayoutBindings[3].stageFlags =
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = nullptr;
	descriptorSetLayoutCreateInfo.flags = 0;
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;
	descriptorSetLayoutCreateInfo.bindingCount = 4;

	VkResult result = vkCreateDescriptorSetLayout(logicalDevices[0], &descriptorSetLayoutCreateInfo, nullptr,
		&graphicsDescriptorSetLayout);

	if (result == VK_SUCCESS)
		std::cout << "Descriptor Set Layout created successfully." << std::endl;
	else
		throw VulkanException("Couldn't create descriptor set layout");

	VkPushConstantRange pushConstantRange = {};
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(ViewProjectionMatrices<float>);
	pushConstantRange.stageFlags =
		VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

	graphicsPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	graphicsPipelineLayoutCreateInfo.pNext = nullptr;
	graphicsPipelineLayoutCreateInfo.flags = 0;
	graphicsPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	graphicsPipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;
	graphicsPipelineLayoutCreateInfo.pSetLayouts = &graphicsDescriptorSetLayout;
	graphicsPipelineLayoutCreateInfo.setLayoutCount = 1;

	result = vkCreatePipelineLayout(logicalDevices[0], &graphicsPipelineLayoutCreateInfo, nullptr,
		&graphicsPipelineLayout);

	if (result == VK_SUCCESS) {
		std::cout << "Graphics Pipeline Layout created successfully." << std::endl;
	}
	else
		throw VulkanException("Couldn't create graphics pipeline layout.");
}

void VulkanEngine::createRenderCommandPool() {
	VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
													 VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, graphicsQueueFamilyIndex };
	VkResult result = vkCreateCommandPool(logicalDevices[0], &commandPoolCreateInfo, nullptr, &renderCommandPool);

	if (result == VK_SUCCESS) {
		std::cout << "Graphics Command Pool created successfully." << std::endl;
	}
	else
		throw VulkanException("Couldn't create graphics command pool.");
}

void VulkanEngine::createTransferCommandPool() {
	VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, nullptr,
													 VK_COMMAND_POOL_CREATE_TRANSIENT_BIT, transferQueueFamilyIndex };
	VkResult result = vkCreateCommandPool(logicalDevices[0], &commandPoolCreateInfo, nullptr, &transferCommandPool);

	if (result == VK_SUCCESS) {
		std::cout << "Transfer Command Pool created successfully." << std::endl;
	}
	else
		throw VulkanException("Couldn't create transfer command pool.");
}


void VulkanEngine::render(uint32_t drawableImageIndex) {
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.commandPool = renderCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VKASSERT_SUCCESS(vkAllocateCommandBuffers(logicalDevices[0], &commandBufferAllocateInfo, &renderCommandBuffer)); //probably cpu render bottle-neck


	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;
	commandBufferBeginInfo.flags = 0;

	VKASSERT_SUCCESS(vkBeginCommandBuffer(renderCommandBuffer, &commandBufferBeginInfo));

	VkRenderPassBeginInfo renderPassBeginInfo = {};

	VkClearValue clearValues[2];
	clearValues[0].color.float32[0] = clearColor[0];
	clearValues[0].color.float32[1] = clearColor[1];
	clearValues[0].color.float32[2] = clearColor[2];
	clearValues[0].color.float32[3] = 1.0f;

	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.pNext = nullptr;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.renderArea.extent.width = swapchainCreateInfo.imageExtent.width;
	renderPassBeginInfo.renderArea.extent.height = swapchainCreateInfo.imageExtent.height;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.framebuffer = framebuffers[drawableImageIndex];


	vkCmdBeginRenderPass(renderCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	//    const long long sysTimeMS = GetTickCount();

	vkCmdBindPipeline(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		vkCmdPushConstants(renderCommandBuffer, graphicsPipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
			sizeof(ViewProjectionMatrices<float>),
			&viewProjection);


		vkCmdBindDescriptorSets(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1,
			meshDescriptorSets + meshIndex, 0, nullptr);


		VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(renderCommandBuffer, 0, 1, vertexBuffersDevice + meshIndex, &offset);

		vkCmdBindIndexBuffer(renderCommandBuffer, indexBuffersDevice[meshIndex], 0, VK_INDEX_TYPE_UINT32);


		vkCmdDrawIndexed(renderCommandBuffer, static_cast<uint32_t>(sortedIndices[meshIndex].size()), 1, 0, 0, 0);
	}


	//vkCmdBindPipeline(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	//for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
	//	vkCmdPushConstants(renderCommandBuffer, graphicsPipelineLayout,
	//		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
	//		sizeof(ViewProjectionMatrices<float>),
	//		&viewProjection);


	//	vkCmdBindDescriptorSets(renderCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipelineLayout, 0, 1,
	//		meshDescriptorSets + meshIndex, 0, nullptr);


	//	VkDeviceSize offset = 0;

	//	vkCmdBindVertexBuffers(renderCommandBuffer, 0, 1, vertexBuffersDevice + meshIndex, &offset);

	//	vkCmdBindIndexBuffer(renderCommandBuffer, indexBuffersDevice[meshIndex], 0, VK_INDEX_TYPE_UINT32);


	//	vkCmdDrawIndexed(renderCommandBuffer, static_cast<uint32_t>(sortedIndices[meshIndex].size()), 1, 0, 0, 0);
	//}

	vkCmdEndRenderPass(renderCommandBuffer);

	VKASSERT_SUCCESS(vkEndCommandBuffer(renderCommandBuffer));

	VkSubmitInfo queueSubmit = {};
	queueSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	queueSubmit.pNext = nullptr;
	queueSubmit.waitSemaphoreCount = 1;
	queueSubmit.pWaitSemaphores = &indexAcquiredSemaphore;
	VkPipelineStageFlags dstSemaphoreStageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	queueSubmit.pWaitDstStageMask = &dstSemaphoreStageFlags;
	queueSubmit.commandBufferCount = 1;
	queueSubmit.pCommandBuffers = &renderCommandBuffer;
	queueSubmit.signalSemaphoreCount = 1;
	queueSubmit.pSignalSemaphores = &waitToPresentSemaphore;

	VKASSERT_SUCCESS(vkQueueSubmit(graphicsQueue, 1, &queueSubmit, queueDoneFence));

	VkResult waitForFenceResult = vkWaitForFences(logicalDevices[0], 1, &queueDoneFence, VK_TRUE,
		1000000000); // 1sec timeout

	switch (waitForFenceResult) {
	case VK_TIMEOUT:
		throw VulkanException("Queue execution timeout!");
		break;
	case VK_SUCCESS:
		break;
	default:
		throw VulkanException("Queue submission failed!");
	}

	VKASSERT_SUCCESS(vkResetFences(logicalDevices[0], 1, &queueDoneFence));

	//vkResetCommandPool(logicalDevices[0], renderCommandPool, VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT);

	//vkResetDescriptorPool(logicalDevices[0], descriptorPool, 0);
}

void VulkanEngine::createWaitToDrawSemaphore() {
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};

	semaphoreCreateInfo.flags = 0;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	VKASSERT_SUCCESS(vkCreateSemaphore(logicalDevices[0], &semaphoreCreateInfo, nullptr, &indexAcquiredSemaphore));
}

void VulkanEngine::createWaitToPresentSemaphore() {
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};

	semaphoreCreateInfo.flags = 0;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	VKASSERT_SUCCESS(vkCreateSemaphore(logicalDevices[0], &semaphoreCreateInfo, nullptr, &waitToPresentSemaphore));
}

void VulkanEngine::createSyncMeans() {
	createWaitToDrawSemaphore();
	createWaitToPresentSemaphore();
	createQueueDoneFence();
}

void VulkanEngine::destroySyncMeans() {
	if (logicalDevices[0] == VK_NULL_HANDLE)
		return;

	if (indexAcquiredSemaphore != VK_NULL_HANDLE) {
		vkDestroySemaphore(logicalDevices[0], indexAcquiredSemaphore, nullptr);
		std::cout << "Semaphore destroyed." << std::endl;
	}

	if (waitToPresentSemaphore != VK_NULL_HANDLE) {
		vkDestroySemaphore(logicalDevices[0], waitToPresentSemaphore, nullptr);
		std::cout << "Semaphore destroyed." << std::endl;
	}

	if (queueDoneFence != VK_NULL_HANDLE) {
		vkDestroyFence(logicalDevices[0], queueDoneFence, nullptr);
		std::cout << "Fence destroyed." << std::endl;
	}
}


void VulkanEngine::createDescriptorPool() {
	VkDescriptorPoolSize descriptorPoolSizes[2];
	descriptorPoolSizes[0].descriptorCount = cachedScene->mNumMeshes;
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	descriptorPoolSizes[1].descriptorCount = cachedScene->mNumMeshes * 3;
	descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = nullptr;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = cachedScene->mNumMeshes;
	descriptorPoolCreateInfo.poolSizeCount = 2;
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

	VKASSERT_SUCCESS(vkCreateDescriptorPool(logicalDevices[0], &descriptorPoolCreateInfo, nullptr, &descriptorPool));
}


void VulkanEngine::createDescriptorSets() {
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = nullptr;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 1.0f;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VKASSERT_SUCCESS(vkCreateSampler(logicalDevices[0], &samplerCreateInfo, nullptr, &textureSampler));

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		descriptorSetAllocateInfo = {};
		descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocateInfo.pNext = nullptr;
		descriptorSetAllocateInfo.descriptorPool = descriptorPool;
		descriptorSetAllocateInfo.descriptorSetCount = 1;
		descriptorSetAllocateInfo.pSetLayouts = &graphicsDescriptorSetLayout;

		VKASSERT_SUCCESS(vkAllocateDescriptorSets(logicalDevices[0], &descriptorSetAllocateInfo,
			meshDescriptorSets + meshIndex));

		VkDescriptorImageInfo descriptorSetImageInfo = {};
		descriptorSetImageInfo.imageView = colorTextureViews[meshIndex];
		descriptorSetImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorSetImageInfo.sampler = textureSampler;

		VkDescriptorImageInfo descriptorSetNormalImageInfo = {};
		descriptorSetNormalImageInfo.imageView = normalTextureViews[meshIndex];
		descriptorSetNormalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorSetNormalImageInfo.sampler = textureSampler;

		VkDescriptorImageInfo descriptorSetSpecImageInfo = {};
		descriptorSetSpecImageInfo.imageView = specTextureViews[meshIndex];
		descriptorSetSpecImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorSetSpecImageInfo.sampler = textureSampler;


		VkDescriptorBufferInfo descriptorSetBufferInfo = {};
		descriptorSetBufferInfo.buffer = uniformBuffersDevice[meshIndex];
		descriptorSetBufferInfo.offset = 0;
		descriptorSetBufferInfo.range = VK_WHOLE_SIZE;

		VkWriteDescriptorSet descriptorSetWrites[4];
		descriptorSetWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrites[0].pNext = nullptr;
		descriptorSetWrites[0].dstSet = meshDescriptorSets[meshIndex];
		descriptorSetWrites[0].dstBinding = 0;
		descriptorSetWrites[0].dstArrayElement = 0;
		descriptorSetWrites[0].descriptorCount = 1;
		descriptorSetWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorSetWrites[0].pImageInfo = nullptr;
		descriptorSetWrites[0].pBufferInfo = &descriptorSetBufferInfo;
		descriptorSetWrites[0].pTexelBufferView = nullptr;

		descriptorSetWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrites[1].pNext = nullptr;
		descriptorSetWrites[1].dstSet = meshDescriptorSets[meshIndex];
		descriptorSetWrites[1].dstBinding = 1;
		descriptorSetWrites[1].dstArrayElement = 0;
		descriptorSetWrites[1].descriptorCount = 1;
		descriptorSetWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorSetWrites[1].pImageInfo = &descriptorSetImageInfo;
		descriptorSetWrites[1].pBufferInfo = nullptr;
		descriptorSetWrites[1].pTexelBufferView = nullptr;

		descriptorSetWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrites[2].pNext = nullptr;
		descriptorSetWrites[2].dstSet = meshDescriptorSets[meshIndex];
		descriptorSetWrites[2].dstBinding = 2;
		descriptorSetWrites[2].dstArrayElement = 0;
		descriptorSetWrites[2].descriptorCount = 1;
		descriptorSetWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorSetWrites[2].pImageInfo = &descriptorSetNormalImageInfo;
		descriptorSetWrites[2].pBufferInfo = nullptr;
		descriptorSetWrites[2].pTexelBufferView = nullptr;

		descriptorSetWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorSetWrites[3].pNext = nullptr;
		descriptorSetWrites[3].dstSet = meshDescriptorSets[meshIndex];
		descriptorSetWrites[3].dstBinding = 3;
		descriptorSetWrites[3].dstArrayElement = 0;
		descriptorSetWrites[3].descriptorCount = 1;
		descriptorSetWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorSetWrites[3].pImageInfo = &descriptorSetSpecImageInfo;
		descriptorSetWrites[3].pBufferInfo = nullptr;
		descriptorSetWrites[3].pTexelBufferView = nullptr;

		vkUpdateDescriptorSets(logicalDevices[0], 4, descriptorSetWrites, 0, nullptr);
	}
}

void VulkanEngine::loadMesh(const char* fileName) {
	std::string err;

	//HMODULE assimpModule = LoadLibrary("assimp-vc140-mt.dll");
/*
	if (assimpModule == NULL)
		throw std::exception();

	FNP_aiImportFile *_aiImportFile = (FNP_aiImportFile *)GetProcAddress(assimpModule, "aiImportFile");*/

	std::string meshPath(resourcesPath);

	meshPath.append(fileName);

	const aiScene* scene = aiImportFile(meshPath.c_str(),
		aiProcess_ConvertToLeftHanded | aiProcess_Triangulate | aiProcess_CalcTangentSpace);

	if (scene == NULL)
	{
		std::string errorText = "Assimp: Couldn't load mesh file: ";
		errorText.append(getCurrentPath());
		errorText.append(meshPath);

		throw VulkanException(errorText.c_str());
	}
	else
		std::cout << "Mesh file loaded successfully." << std::endl;

	cachedScene = reinterpret_cast<aiScene*>(malloc(sizeof(aiScene)));
	memcpy(cachedScene, scene, sizeof(aiScene));

	std::cout << "Number of Meshes reported from Assimp: " << cachedScene->mNumMeshes << std::endl;

	for (uint32_t subMeshIndex = 0; subMeshIndex < cachedScene->mNumMeshes; subMeshIndex++) {
		std::cout << "\t" << cachedScene->mMeshes[subMeshIndex]->mName.C_Str() << std::endl;
	}

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {

		uint32_t numVertices = cachedScene->mMeshes[meshIndex]->mNumVertices;
		uint32_t numFaces = cachedScene->mMeshes[meshIndex]->mNumFaces;
		uint32_t numIndices = numFaces * 3; // Triangulated

		for (uint32_t i = 0; i < numVertices; i++) {

			Attribute<float> tmpAttribute = {};
			memcpy(((byte*)&tmpAttribute) + offsetof(Attribute<float>, position),
				&((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mVertices))[i].x, 3 * sizeof(float));
			memcpy(((byte*)&tmpAttribute) + offsetof(Attribute<float>, normal),
				&((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mNormals))[i].x, 3 * sizeof(float));
			memcpy(((byte*)&tmpAttribute) + offsetof(Attribute<float>, uv),
				&((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mTextureCoords[0]))[i].x, 2 * sizeof(float));
			memcpy(((byte*)&tmpAttribute) + offsetof(Attribute<float>, tangent),
				&((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mTangents))[i].x, 3 * sizeof(float));
			memcpy(((byte*)&tmpAttribute) + offsetof(Attribute<float>, bitangent),
				&((aiVector3D*)(cachedScene->mMeshes[meshIndex]->mBitangents))[i].x, 3 * sizeof(float));

			sortedAttributes[meshIndex].push_back(tmpAttribute);
		}

		for (uint32_t i = 0; i < numFaces; i++) {
			for (unsigned int j = 0; j < ((aiFace*)(cachedScene->mMeshes[meshIndex]->mFaces))[i].mNumIndices; j++) {
				uint32_t index = ((unsigned int*)(((aiFace*)(cachedScene->mMeshes[meshIndex]->mFaces))[i].mIndices))[j];
				sortedIndices[meshIndex].push_back(index);
			}
		}

		vertexBuffersSizes[meshIndex] = static_cast<uint32_t>(sortedAttributes[meshIndex].size() * sizeof(Attribute<float>));
		indexBuffersSizes[meshIndex] = static_cast<uint32_t>(sortedIndices[meshIndex].size() * 4);
	}
}

VkMemoryRequirements VulkanEngine::createBuffer(VkBuffer* buffer, VkDeviceSize size, VkBufferUsageFlags usageFlags) {
	VkBufferCreateInfo bufferCreateInfo = {};

	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = nullptr;
	bufferCreateInfo.flags = 0;
	bufferCreateInfo.usage = usageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.size = size;

	VKASSERT_SUCCESS(vkCreateBuffer(logicalDevices[0], &bufferCreateInfo, nullptr, buffer));

	VkMemoryRequirements uniformBufferMemoryRequirements;

	vkGetBufferMemoryRequirements(logicalDevices[0], *buffer, &uniformBufferMemoryRequirements);

	return uniformBufferMemoryRequirements;
}

VkMemoryRequirements VulkanEngine::createTexture(VkImage* textureImage, VkImageUsageFlags usageFlags) {
	VkImageCreateInfo textureImageCreateInfo = {};

	textureImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	textureImageCreateInfo.pNext = nullptr;
	textureImageCreateInfo.flags = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
	textureImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	textureImageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	textureImageCreateInfo.extent.width = textureDim;
	textureImageCreateInfo.extent.height = textureDim;
	textureImageCreateInfo.extent.depth = 1;
	textureImageCreateInfo.arrayLayers = 1;
	textureImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	textureImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	textureImageCreateInfo.usage = usageFlags;
	textureImageCreateInfo.mipLevels = 1;
	textureImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	textureImageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	textureImageCreateInfo.queueFamilyIndexCount = 0;
	textureImageCreateInfo.pQueueFamilyIndices = nullptr;

	VKASSERT_SUCCESS(vkCreateImage(logicalDevices[0], &textureImageCreateInfo, nullptr, textureImage));

	VkMemoryRequirements textureImageMemoryRequirements;

	vkGetImageMemoryRequirements(logicalDevices[0], *textureImage, &textureImageMemoryRequirements);

	return textureImageMemoryRequirements;
}

void VulkanEngine::createTextureView(VkImageView* textureImageView, VkImage textureImage) {
	VkImageViewCreateInfo textureImageViewCreateInfo = {};

	textureImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	textureImageViewCreateInfo.pNext = NULL;
	textureImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	textureImageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	textureImageViewCreateInfo.flags = 0;
	textureImageViewCreateInfo.image = textureImage;
	textureImageViewCreateInfo.subresourceRange = {};
	textureImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	textureImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	textureImageViewCreateInfo.subresourceRange.levelCount = 1;
	textureImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	textureImageViewCreateInfo.subresourceRange.layerCount = 1;

	VKASSERT_SUCCESS(vkCreateImageView(logicalDevices[0], &textureImageViewCreateInfo, nullptr, textureImageView));
}

void VulkanEngine::commitTextures() {
	VkCommandBuffer commandBuffer;
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.commandPool = transferCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VKASSERT_SUCCESS(vkAllocateCommandBuffers(logicalDevices[0], &commandBufferAllocateInfo, &commandBuffer));

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	VKASSERT_SUCCESS(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {

		VkImageMemoryBarrier imageMemoryBarriers[6];
		imageMemoryBarriers[0].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarriers[0].pNext = nullptr;
		imageMemoryBarriers[0].srcAccessMask = 0;
		imageMemoryBarriers[0].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarriers[0].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarriers[0].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarriers[0].image = colorTextureImagesDevice[meshIndex];
		imageMemoryBarriers[0].subresourceRange.layerCount = 1;
		imageMemoryBarriers[0].subresourceRange.baseArrayLayer = 0;
		imageMemoryBarriers[0].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarriers[0].subresourceRange.levelCount = 1;
		imageMemoryBarriers[0].subresourceRange.baseMipLevel = 0;
		imageMemoryBarriers[0].srcQueueFamilyIndex = transferQueueFamilyIndex;
		imageMemoryBarriers[0].dstQueueFamilyIndex = transferQueueFamilyIndex;

		imageMemoryBarriers[1].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarriers[1].pNext = nullptr;
		imageMemoryBarriers[1].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarriers[1].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarriers[1].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarriers[1].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarriers[1].image = colorTextureImages[meshIndex];
		imageMemoryBarriers[1].subresourceRange.layerCount = 1;
		imageMemoryBarriers[1].subresourceRange.baseArrayLayer = 0;
		imageMemoryBarriers[1].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarriers[1].subresourceRange.levelCount = 1;
		imageMemoryBarriers[1].subresourceRange.baseMipLevel = 0;
		imageMemoryBarriers[1].srcQueueFamilyIndex = transferQueueFamilyIndex;
		imageMemoryBarriers[1].dstQueueFamilyIndex = transferQueueFamilyIndex;

		imageMemoryBarriers[2].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarriers[2].pNext = nullptr;
		imageMemoryBarriers[2].srcAccessMask = 0;
		imageMemoryBarriers[2].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarriers[2].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarriers[2].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarriers[2].image = normalTextureImagesDevice[meshIndex];
		imageMemoryBarriers[2].subresourceRange.layerCount = 1;
		imageMemoryBarriers[2].subresourceRange.baseArrayLayer = 0;
		imageMemoryBarriers[2].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarriers[2].subresourceRange.levelCount = 1;
		imageMemoryBarriers[2].subresourceRange.baseMipLevel = 0;
		imageMemoryBarriers[2].srcQueueFamilyIndex = transferQueueFamilyIndex;
		imageMemoryBarriers[2].dstQueueFamilyIndex = transferQueueFamilyIndex;

		imageMemoryBarriers[3].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarriers[3].pNext = nullptr;
		imageMemoryBarriers[3].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarriers[3].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarriers[3].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarriers[3].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarriers[3].image = normalTextureImages[meshIndex];
		imageMemoryBarriers[3].subresourceRange.layerCount = 1;
		imageMemoryBarriers[3].subresourceRange.baseArrayLayer = 0;
		imageMemoryBarriers[3].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarriers[3].subresourceRange.levelCount = 1;
		imageMemoryBarriers[3].subresourceRange.baseMipLevel = 0;
		imageMemoryBarriers[3].srcQueueFamilyIndex = transferQueueFamilyIndex;
		imageMemoryBarriers[3].dstQueueFamilyIndex = transferQueueFamilyIndex;

		imageMemoryBarriers[4].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarriers[4].pNext = nullptr;
		imageMemoryBarriers[4].srcAccessMask = 0;
		imageMemoryBarriers[4].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarriers[4].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarriers[4].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarriers[4].image = specTextureImagesDevice[meshIndex];
		imageMemoryBarriers[4].subresourceRange.layerCount = 1;
		imageMemoryBarriers[4].subresourceRange.baseArrayLayer = 0;
		imageMemoryBarriers[4].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarriers[4].subresourceRange.levelCount = 1;
		imageMemoryBarriers[4].subresourceRange.baseMipLevel = 0;
		imageMemoryBarriers[4].srcQueueFamilyIndex = transferQueueFamilyIndex;
		imageMemoryBarriers[4].dstQueueFamilyIndex = transferQueueFamilyIndex;

		imageMemoryBarriers[5].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarriers[5].pNext = nullptr;
		imageMemoryBarriers[5].srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		imageMemoryBarriers[5].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarriers[5].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarriers[5].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarriers[5].image = specTextureImages[meshIndex];
		imageMemoryBarriers[5].subresourceRange.layerCount = 1;
		imageMemoryBarriers[5].subresourceRange.baseArrayLayer = 0;
		imageMemoryBarriers[5].subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarriers[5].subresourceRange.levelCount = 1;
		imageMemoryBarriers[5].subresourceRange.baseMipLevel = 0;
		imageMemoryBarriers[5].srcQueueFamilyIndex = transferQueueFamilyIndex;
		imageMemoryBarriers[5].dstQueueFamilyIndex = transferQueueFamilyIndex;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
			nullptr, 0, nullptr, 1, imageMemoryBarriers + 0);
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
			0, nullptr, 1, imageMemoryBarriers + 1);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
			nullptr, 0, nullptr, 1, imageMemoryBarriers + 2);
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
			0, nullptr, 1, imageMemoryBarriers + 3);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
			nullptr, 0, nullptr, 1, imageMemoryBarriers + 4);
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr,
			0, nullptr, 1, imageMemoryBarriers + 5);

		VkImageCopy region = {};
		region.srcOffset.x = 0;
		region.srcOffset.y = 0;
		region.srcOffset.z = 0;
		region.dstOffset.x = 0;
		region.dstOffset.y = 0;
		region.dstOffset.z = 0;
		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.layerCount = 1;
		region.dstSubresource.mipLevel = 0;
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.layerCount = 1;
		region.srcSubresource.mipLevel = 0;
		region.extent.depth = 1;
		region.extent.width = textureDim;
		region.extent.height = textureDim;


		vkCmdCopyImage(commandBuffer, colorTextureImages[meshIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			colorTextureImagesDevice[meshIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		vkCmdCopyImage(commandBuffer, normalTextureImages[meshIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			normalTextureImagesDevice[meshIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		vkCmdCopyImage(commandBuffer, specTextureImages[meshIndex], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			specTextureImagesDevice[meshIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	VKASSERT_SUCCESS(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo queueSubmit = {};
	queueSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	queueSubmit.pNext = nullptr;
	queueSubmit.waitSemaphoreCount = 0;
	queueSubmit.pWaitSemaphores = nullptr;
	queueSubmit.pWaitDstStageMask = nullptr;
	queueSubmit.commandBufferCount = 1;
	queueSubmit.pCommandBuffers = &commandBuffer;
	queueSubmit.signalSemaphoreCount = 0;
	queueSubmit.pSignalSemaphores = nullptr;

	VKASSERT_SUCCESS(vkQueueSubmit(transferQueue, 1, &queueSubmit, VK_NULL_HANDLE));

	VKASSERT_SUCCESS(vkDeviceWaitIdle(logicalDevices[0]));

	vkFreeCommandBuffers(logicalDevices[0], transferCommandPool, 1, &commandBuffer);
}

void VulkanEngine::destroyStagingMeans() {
	if (logicalDevices[0] == VK_NULL_HANDLE)
		return;

	for (uint16_t meshIndex = 0; meshIndex < cachedScene->mNumMeshes; meshIndex++) {
		if (uniformBuffers[meshIndex] != VK_NULL_HANDLE)
			vkDestroyBuffer(logicalDevices[0], uniformBuffers[meshIndex], nullptr);

		if (vertexBuffers[meshIndex] != VK_NULL_HANDLE)
			vkDestroyBuffer(logicalDevices[0], vertexBuffers[meshIndex], nullptr);

		if (indexBuffers[meshIndex] != VK_NULL_HANDLE)
			vkDestroyBuffer(logicalDevices[0], indexBuffers[meshIndex], nullptr);

		if (colorTextureImages[meshIndex] != VK_NULL_HANDLE)
			vkDestroyImage(logicalDevices[0], colorTextureImages[meshIndex], nullptr);

		if (normalTextureImages[meshIndex] != VK_NULL_HANDLE)
			vkDestroyImage(logicalDevices[0], normalTextureImages[meshIndex], nullptr);

		if (specTextureImages[meshIndex] != VK_NULL_HANDLE)
			vkDestroyImage(logicalDevices[0], specTextureImages[meshIndex], nullptr);
	}

	if (uniBuffersMemory != VK_NULL_HANDLE)
		vkFreeMemory(logicalDevices[0], uniBuffersMemory, nullptr);

	if (uniTexturesMemory != VK_NULL_HANDLE)
		vkFreeMemory(logicalDevices[0], uniTexturesMemory, nullptr);
}

//void VulkanEngine::createGraphicsNormalViewerPipeline() {
//
//	VkPipelineShaderStageCreateInfo stageCreateInfos[3];
//
//	stageCreateInfos[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//	stageCreateInfos[0].pNext = nullptr;
//	stageCreateInfos[0].flags = 0;
//	stageCreateInfos[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
//	stageCreateInfos[0].module = graphicsNormalViewerVertexShaderModule;
//	stageCreateInfos[0].pName = u8"main";
//	stageCreateInfos[0].pSpecializationInfo = nullptr;
//
//	stageCreateInfos[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//	stageCreateInfos[1].pNext = nullptr;
//	stageCreateInfos[1].flags = 0;
//	stageCreateInfos[1].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
//	stageCreateInfos[1].module = graphicsNormalViewerGeometryShaderModule;
//	stageCreateInfos[1].pName = u8"main";
//	stageCreateInfos[1].pSpecializationInfo = nullptr;
//
//	stageCreateInfos[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//	stageCreateInfos[2].pNext = nullptr;
//	stageCreateInfos[2].flags = 0;
//	stageCreateInfos[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//	stageCreateInfos[2].module = graphicsNormalViewerFragmentShaderModule;
//	stageCreateInfos[2].pName = u8"main";
//	stageCreateInfos[2].pSpecializationInfo = nullptr;
//
//	vertexBindingDescription.binding = 0;
//	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//	vertexBindingDescription.stride = sizeof(Attribute<float>);
//
//	VkVertexInputAttributeDescription vertexAttributeDescriptions[5];
//
//	vertexAttributeDescriptions[0].binding = 0;
//	vertexAttributeDescriptions[0].location = 0;
//	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
//	vertexAttributeDescriptions[0].offset = offsetof(Attribute<float>, position);
//
//	vertexAttributeDescriptions[1].binding = 0;
//	vertexAttributeDescriptions[1].location = 1;
//	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
//	vertexAttributeDescriptions[1].offset = offsetof(Attribute<float>, normal);
//
//	vertexAttributeDescriptions[2].binding = 0;
//	vertexAttributeDescriptions[2].location = 2;
//	vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
//	vertexAttributeDescriptions[2].offset = offsetof(Attribute<float>, uv);
//
//	vertexAttributeDescriptions[3].binding = 0;
//	vertexAttributeDescriptions[3].location = 3;
//	vertexAttributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
//	vertexAttributeDescriptions[3].offset = offsetof(Attribute<float>, tangent);
//
//	vertexAttributeDescriptions[4].binding = 0;
//	vertexAttributeDescriptions[4].location = 4;
//	vertexAttributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
//	vertexAttributeDescriptions[4].offset = offsetof(Attribute<float>, bitangent);
//
//	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//	vertexInputStateCreateInfo.pNext = nullptr;
//	vertexInputStateCreateInfo.flags = 0;
//	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
//	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
//	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 5;
//	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;
//
//	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//	inputAssemblyStateCreateInfo.pNext = nullptr;
//	inputAssemblyStateCreateInfo.flags = 0;
//	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
//
//	viewport.width = static_cast<float>(swapchainCreateInfo.imageExtent.width);
//	viewport.height = static_cast<float>(swapchainCreateInfo.imageExtent.height);
//	viewport.x = 0;
//	viewport.y = 0;
//	viewport.minDepth = 0.0f;
//	viewport.maxDepth = 1.0f;
//
//	scissor.offset.x = 0;
//	scissor.offset.y = 0;
//	scissor.extent.width = std::lroundf(viewport.width);
//	scissor.extent.height = std::lroundf(viewport.height);
//
//	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//	viewportStateCreateInfo.pNext = nullptr;
//	viewportStateCreateInfo.flags = 0;
//	viewportStateCreateInfo.viewportCount = 1;
//	viewportStateCreateInfo.pViewports = &viewport;
//	viewportStateCreateInfo.scissorCount = 1;
//	viewportStateCreateInfo.pScissors = &scissor;
//
//	rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//	rasterizationStateCreateInfo.pNext = nullptr;
//	rasterizationStateCreateInfo.flags = 0;
//	rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
//	rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
//	rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
//	rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_NONE; // no back-face/front-face culling!
//	rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
//	rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
//	rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
//	rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
//	rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
//	rasterizationStateCreateInfo.lineWidth = 1.0f;
//
//
//	multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//	multisampleStateCreateInfo.pNext = nullptr;
//	multisampleStateCreateInfo.flags = 0;
//	multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//	multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
//	multisampleStateCreateInfo.pSampleMask = nullptr;
//
//	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
//	colorBlendAttachmentState.blendEnable = VK_FALSE;
//	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
//	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_MAX;
//	colorBlendAttachmentState.colorWriteMask =
//		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//
//	colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//	colorBlendStateCreateInfo.pNext = nullptr;
//	colorBlendStateCreateInfo.flags = 0;
//	colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
//	colorBlendStateCreateInfo.attachmentCount = 1;
//	colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;
//
//	VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {};
//	depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//	depthStencilStateCreateInfo.pNext = nullptr;
//	depthStencilStateCreateInfo.flags = 0;
//	depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
//	depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
//	depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
//	depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;
//
//	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//	graphicsPipelineCreateInfo.pNext = nullptr;
//	graphicsPipelineCreateInfo.flags = 0;
//	graphicsPipelineCreateInfo.stageCount = 3;
//	graphicsPipelineCreateInfo.pStages = stageCreateInfos;
//	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
//	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
//	graphicsPipelineCreateInfo.pTessellationState = nullptr;
//	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
//	graphicsPipelineCreateInfo.pRasterizationState = &rasterizationStateCreateInfo;
//	graphicsPipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
//	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
//	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
//	graphicsPipelineCreateInfo.pDynamicState = nullptr;
//	graphicsPipelineCreateInfo.layout = graphicsPipelineLayout;
//	graphicsPipelineCreateInfo.renderPass = renderPass;
//	graphicsPipelineCreateInfo.subpass = 0;
//	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
//	graphicsPipelineCreateInfo.basePipelineIndex = -1;
//
//	VkResult result = vkCreateGraphicsPipelines(logicalDevices[0], VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo,
//		nullptr, &graphicsDebugPipeline);
//
//	if (result == VK_SUCCESS)
//		std::cout << "Graphics Pipeline created successfully." << std::endl;
//	else
//		throw VulkanException("Couldn't create graphics pipeline.");
//
//}

float VulkanEngine::getElapsedTime()
{
	QueryPerformanceCounter(&t2);

	LARGE_INTEGER elapsed;

	elapsed.QuadPart = t2.QuadPart - t1.QuadPart;

	t1 = t2;

	elapsed.QuadPart *= 1000000;
	elapsed.QuadPart /= frequency.QuadPart;

	return float(elapsed.QuadPart);
}