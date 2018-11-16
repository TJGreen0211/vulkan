#define GLFW_INCLUDE_VULKAN

#include <windows.h>
#include <GLFW/glfw3.h>
#include <STB/stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "linearAlg.h"
#include "arcballCamera.h"

#define _CRT_SECURE_NO_DEPRECATE 1
//#define _CRT_SECURE_NO_WARNINGS 1

//#define max(x,y) ((x) >= (y)) ? (x) : (y)
//#define min(x,y) ((x) <= (y)) ? (x) : (y)

//#include "vulkan_core.h"

#ifndef NDEBUG
	const unsigned int enableValidationLayers = 0;
#else
	const unsigned int enableValidationLayers = 1;
#endif

typedef struct vertexData {
	float pos[3];
	float color[3];
	float texCoord[2];
	//VkVertexInputBindingDescription (*getBindingDescription);
	//VkVertexInputAttributeDescription (*getAttributeDescriptions);
} vertexData;

float model[16];
//double view[4][4];
//double projection[4][4];

typedef struct uniformBufferObject {
	float model[16];
	float view[16];
	float projection[16];
} uniformBufferObject;

typedef struct swapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR *formats;
	VkPresentModeKHR *presentModes;
} swapChainSupportDetails;


typedef struct QueueFamilyIndices {
	int graphicsFamily;
	int presentFamily;
	int (*isComplete)(int, int);
} QueueFamilyIndices;

int checkComplete(int graphicsVal, int presentVal) {
	return graphicsVal >= 0 && presentVal >= 0;
}

const char *validationLayers[] = {"VK_LAYER_LUNARG_standard_validation"};

const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

unsigned int globalImageCount;

const vertexData vertices[8] = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}

};

const uint16_t vertexIndices[12] = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
};

VkInstance instance;
VkSurfaceKHR surface;
VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkDevice device;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkSwapchainKHR swapChain;
VkImage *swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
VkImageView *swapChainImageViews;
VkRenderPass renderPass;
VkPipelineLayout pipelineLayout;
VkPipeline graphicsPipeline;
VkFramebuffer *swapChainFramebuffers;
VkCommandPool commandPool;
VkCommandBuffer *commandBuffers;
VkBuffer vertexBuffer;
VkDeviceMemory vertexBufferMemory;
VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;
VkBuffer *uniformBuffer;
VkDeviceMemory *uniformBufferMemory;
GLFWwindow *window;

VkDescriptorSetLayout descriptorSetLayout;

VkDescriptorPool descriptorPool;
VkDescriptorSet *descriptorSets;

VkImage textureImage;
VkDeviceMemory textureImageMemory;

VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;

VkImageView textureImageView;
VkSampler textureSampler;

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;

VkDebugReportCallbackEXT callback;

static VkVertexInputBindingDescription getBindingDescription() {
	VkVertexInputBindingDescription bindingDescription ={
		.binding = 0,
		.stride = sizeof(vertexData),
		.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
	};
	return bindingDescription;
}

static VkVertexInputAttributeDescription *getAttributeDescriptions() {
	VkVertexInputAttributeDescription *attributeDescriptions = malloc(3*sizeof(VkVertexInputAttributeDescription));
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(vertexData, pos);

	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(vertexData, color);

	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(vertexData, texCoord);

	return attributeDescriptions;
}

void DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback
        , const VkAllocationCallbacks* pAllocator) {
    PFN_vkDestroyDebugReportCallbackEXT func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    //if (func != NULL) {
    //    func(instance, callback, pAllocator);
    //}
}

void cleanupSwapChain() {
	for(unsigned int i = 0; i < globalImageCount; i++) {
		vkDestroyFramebuffer(device, swapChainFramebuffers[i], NULL);
	}
	vkFreeCommandBuffers(device, commandPool, globalImageCount, commandBuffers);

	vkDestroyPipeline(device, graphicsPipeline, NULL);
	vkDestroyPipelineLayout(device, pipelineLayout, NULL);
	vkDestroyRenderPass(device, renderPass, NULL);

	for(unsigned int i = 0; i < globalImageCount; i++) {
		vkDestroyImageView(device, swapChainImageViews[i], NULL);
	}

	vkDestroySwapchainKHR(device, swapChain, NULL);
}

void cleanup() {
	cleanupSwapChain();
	vkDestroyImageView(device, depthImageView, NULL);
    vkDestroyImage(device, depthImage, NULL);
    vkFreeMemory(device, depthImageMemory, NULL);

	vkDestroySampler(device, textureSampler, NULL);
	vkDestroyImageView(device, textureImageView, NULL);
	vkDestroyImage(device, textureImage, NULL);
	vkFreeMemory(device, textureImageMemory, NULL);

	vkDestroyDescriptorPool(device, descriptorPool, NULL);
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, NULL);

	for (unsigned int i = 0; i < globalImageCount; i++) {
		vkDestroyBuffer(device, uniformBuffer[i], NULL);
		vkFreeMemory(device, uniformBufferMemory[i], NULL);
	}

	vkDestroyBuffer(device, indexBuffer, NULL);
	vkFreeMemory(device, indexBufferMemory, NULL);

	vkDestroyBuffer(device, vertexBuffer, NULL);
	vkFreeMemory(device, vertexBufferMemory, NULL);

	vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
	vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
	vkDestroyCommandPool(device, commandPool, NULL);

	vkDestroyDevice(device, NULL);
	DestroyDebugReportCallbackEXT(instance, callback, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);

	glfwDestroyWindow(window);
	glfwTerminate();
	printf("Cleanup done\n");
}

unsigned int presentModeCount;
unsigned int formatCount;
swapChainSupportDetails querySwapChainSupport(VkPhysicalDevice vkDevice) {
	swapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkDevice, surface, &details.capabilities);


	vkGetPhysicalDeviceSurfaceFormatsKHR(vkDevice, surface, &formatCount, NULL);

	if(formatCount != 0) {
		details.formats = malloc(formatCount*sizeof(details.formats));
		vkGetPhysicalDeviceSurfaceFormatsKHR(vkDevice, surface, &formatCount, details.formats);
	}
	else {
		details.formats = malloc(sizeof(details.formats));
		details.formats = NULL;
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(vkDevice, surface, &presentModeCount, NULL);

	if(presentModeCount != 0) {
		details.presentModes = malloc(formatCount*sizeof(details.presentModes));
		vkGetPhysicalDeviceSurfacePresentModesKHR(vkDevice, surface, &presentModeCount, details.presentModes);
	}
	else {
		details.presentModes = malloc(sizeof(details.presentModes));
		details.presentModes = NULL;
	}

	return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR *availableFormats) {
	if(availableFormats != NULL && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		VkSurfaceFormatKHR newFormat = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
		return newFormat;
	}

	for(unsigned int i = 0; i < formatCount; i++) {
		if(availableFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM && availableFormats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
			return availableFormats[i];
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes) {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for(unsigned int i = 0; i < presentModeCount; i++) {
		if(availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentModes[i];
		}
		else if(availablePresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentModes[i];
		}
	}
	return bestMode;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities) {
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	if(capabilities.currentExtent.width != (uintmax_t)(UINT32_MAX)) {
		return capabilities.currentExtent;
	}
	else {
		VkExtent2D actualExtent = {width, height};

		actualExtent.width = max(capabilities.minImageExtent.width, min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = max(capabilities.minImageExtent.height, min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

unsigned int checkValidationLayerSupport() {
	unsigned int layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);

	VkLayerProperties *availableLayers = malloc(layerCount*sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	for(int i = 0; i < sizeof(validationLayers)/sizeof(validationLayers[0]); i++) {
		unsigned int layerFound = 0;
		for(int j = 0; j < (int)layerCount; j++) {
			if(strcmp(validationLayers[i], availableLayers[j].layerName) == 0) {
				printf("%s, %s\n", validationLayers[i], availableLayers[j].layerName);
				layerFound = 1;
				break;
			}
		}
		if(!layerFound) {
			return 0;
		}
	}
	free(availableLayers);
	return 1;
}

const char **getRequiredExtensions(unsigned int *extensionCount) {
	unsigned int glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	*extensionCount = glfwExtensionCount;

	if(enableValidationLayers) {
		*extensionCount = *extensionCount + 1;
	}
	const char **extensions = malloc(*extensionCount*sizeof(glfwExtensions[0]));

	for(int i = 0; i < (int)*extensionCount; i++) {
		extensions[i] = i < (int)glfwExtensionCount ? glfwExtensions[i] : VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
	}

	return extensions;
}

void createInstance() {
	if(enableValidationLayers && !checkValidationLayerSupport()) {
		printf("Validation layers requested but not available.\n");
		cleanup();
	}

	VkApplicationInfo appInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		//.apiVersion = VK_API_VERSION_1_0,
	};

	unsigned int extensionCount = 0;
	const char **extensions = getRequiredExtensions(&extensionCount);
	VkInstanceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &appInfo,
		.enabledLayerCount = 0,
		.enabledExtensionCount = extensionCount,
		.ppEnabledExtensionNames = extensions,
	};

	if(enableValidationLayers) {
		createInfo.enabledLayerCount = sizeof(validationLayers)/sizeof(validationLayers[0]);
		createInfo.ppEnabledLayerNames = validationLayers;
	}

	/*unsigned int extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
	VkExtensionProperties *extensions = malloc(extensionCount*sizeof(VkExtensionProperties));
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);

	for (int i = 0; i < sizeof(extensions)/sizeof(extensions[0]); i++) {
		printf("%s\n", extensions[i].extensionName);
	}*/

	if(vkCreateInstance(&createInfo, NULL, &instance) != VK_SUCCESS) {
		printf("Failed to create vulkan instance.\n");
	}
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData) {
		printf("Validation layer: %s\n", msg);
	return VK_FALSE;
}

//VkResult CreateDebugReportCallbackExt(VkInstance instance, const VkDebugReportCallbackCreateInfoExt* pCreateInfo, ) {
//
//}

void setupDebugCallback() {
	//if(!enableValidationLayers) return;

	//VkDebugUtilsMessengerCreateInfoEXT createInfo = {
	//	.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
	//	.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT,
	//	.pfnCallback = debugCallback,
	//}
	//if (CreateDebugReportCallbackEXT(instance, &createInfo, NULL, &callback) != VK_SUCCESS) {
	//	printf("failed to set up debug callback");
	//	cleanup();
	//}
}

QueueFamilyIndices *findQueueFamilies(VkPhysicalDevice vkDevice) {
	QueueFamilyIndices *indices = malloc(sizeof(QueueFamilyIndices));
	indices->graphicsFamily = -1;
	indices->presentFamily = -1;
	indices->isComplete = checkComplete;

	unsigned int queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &queueFamilyCount, NULL);

	VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount*sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(vkDevice, &queueFamilyCount, queueFamilies);
	for(int i = 0; i < (int)queueFamilyCount; i++) {
		if(queueFamilies[i].queueCount > 0 && queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices->graphicsFamily = i;
		}

		int presentSupport = 0;
		vkGetPhysicalDeviceSurfaceSupportKHR(vkDevice, i, surface, &presentSupport);

		if(queueFamilies[i].queueCount > 0 &&presentSupport) {
			indices->presentFamily = i;
		}
		if(indices->isComplete(indices->graphicsFamily, indices->presentFamily)) {
			break;
		}
	}
	return indices;
}

int checkDeviceExtensionSupported(VkPhysicalDevice vkDevice) {
	unsigned int extensionCount;
	vkEnumerateDeviceExtensionProperties(vkDevice, NULL, &extensionCount, NULL);

	VkExtensionProperties *availableExtensions = malloc(extensionCount*sizeof(VkExtensionProperties));
	vkEnumerateDeviceExtensionProperties(vkDevice, NULL, &extensionCount, availableExtensions);

	for(int i = 0; i < 1; i++) {
		unsigned int layerFound = 0;
		for(int j = 0; j < (int)extensionCount; j++) {
			if(strcmp(deviceExtensions[i], availableExtensions[j].extensionName) == 0) {
				//printf("%s, %s\n", deviceExtensions[i], availableExtensions[j].extensionName);
				layerFound = 1;
				break;
			}
		}
		if(!layerFound) {
			return 0;
		}
	}
	free(availableExtensions);
	return 1;
}

int isDeviceSuitable(VkPhysicalDevice vkDevice) {
	//VkPhysicalDeviceProperties deviceProperties;
	//vkGetPhysicalDeviceProperties(vkDevice, &deviceProperties);
    //
	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(vkDevice, &deviceFeatures);

	QueueFamilyIndices *indices = findQueueFamilies(vkDevice);

	int extensionsSupported = checkDeviceExtensionSupported(vkDevice);

	int swapChainSuitable = 0;
	if(extensionsSupported) {
		swapChainSupportDetails swapChainSupport = querySwapChainSupport(vkDevice);
		swapChainSuitable = swapChainSupport.formats != NULL && swapChainSupport.presentModes != NULL;
	}
	return indices->isComplete(indices->graphicsFamily, indices->presentFamily) && extensionsSupported && swapChainSuitable && deviceFeatures.samplerAnisotropy;
}

void pickPhysicalDevice() {
	unsigned int deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, NULL);
	if(deviceCount == 0) {
		printf("GPU support for vulkan not available.\n");
		cleanup();
	}
	VkPhysicalDevice *devices = malloc(deviceCount*sizeof(VkPhysicalDevice));
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices);
	for(int i = 0; i < (int)deviceCount; i++) {
		if(isDeviceSuitable(devices[i])) {
			physicalDevice = devices[i];
			break;
		}
	}
	if(physicalDevice == VK_NULL_HANDLE) {
		printf("Failed to find a suitable GPU.\n");
		cleanup();
	}
	free(devices);
}

void createLogicalDevice() {
	QueueFamilyIndices *indices = findQueueFamilies(physicalDevice);
	float queuePriority = 1.0f;

	//TODO: Change for multiple queue families.

	//VkDeviceQueueCreateInfo *queueCreateInfo = malloc()

	VkDeviceQueueCreateInfo queueCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = indices->graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority,
	};

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pQueueCreateInfos = &queueCreateInfo,
		.queueCreateInfoCount = 1,
		.pEnabledFeatures = &deviceFeatures,
		.enabledExtensionCount = sizeof(deviceExtensions)/sizeof(deviceExtensions[0]),
		.ppEnabledExtensionNames = deviceExtensions,
		.enabledLayerCount = 0,
	};

	if(enableValidationLayers) {
		createInfo.enabledLayerCount = sizeof(validationLayers)/sizeof(validationLayers[0]);
		createInfo.ppEnabledLayerNames = validationLayers;
	}

	if(vkCreateDevice(physicalDevice, &createInfo, NULL, &device) != VK_SUCCESS) {
		printf("Failed to create logical device.\n");
		cleanup();
	}

	vkGetDeviceQueue(device, indices->graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices->presentFamily, 0, &presentQueue);
}

void createSurface() {
	if(glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
		printf("Failed to create surface.\n");
		cleanup();
	}
}

void createSwapChain() {
	swapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);
	unsigned int imageCount = swapChainSupport.capabilities.minImageCount + 1;
	globalImageCount = imageCount;

	if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	};

	QueueFamilyIndices *indices = findQueueFamilies(physicalDevice);
	unsigned int queueFamilyIndices[] = {(unsigned int)indices->graphicsFamily, (unsigned int)indices->presentFamily};

	if (indices->graphicsFamily != indices->presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if(vkCreateSwapchainKHR(device, &createInfo, NULL, &swapChain) != VK_SUCCESS) {
		printf("Failed to create swap chain.\n");
		cleanup();
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, NULL);
	swapChainImages = malloc(imageCount*sizeof(VkImage));
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages);

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

}

void createImageViews() {
	swapChainImageViews = malloc(globalImageCount*sizeof(VkImageView));
	for(unsigned int i = 0; i < globalImageCount; i++) {
		VkImageViewCreateInfo createInfo = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = swapChainImages[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = swapChainImageFormat,
			.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
			.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.subresourceRange.baseMipLevel = 0,
			.subresourceRange.levelCount =1,
			.subresourceRange.baseArrayLayer = 0,
			.subresourceRange.layerCount = 1,
		};

		if(vkCreateImageView(device, &createInfo, NULL, &swapChainImageViews[i]) != VK_SUCCESS) {
			printf("Failed to create VK image views\n");
			cleanup();
		}
	}
}

VkShaderModule createShaderModule(const char* code, size_t shaderSize) {
	//uint32_t *pCodeArray = malloc(shaderSize*sizeof(uint32_t));
	//for(int i = 0; i < shaderSize; i++) {
	//	pCodeArray[i] = (uint32_t)code[i];
	//}

	VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = shaderSize,
		.pCode = (uint32_t*)code,
	};

	VkShaderModule shaderModule;
	if(vkCreateShaderModule(device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
		printf("Failed to create shader module.\n");
		cleanup();
	}
	return shaderModule;
}

static char *readShader(char *filename, size_t *size)
{
	FILE *fp;
	char *content = NULL;

	size_t count = 0;
	if(filename != NULL)
	{
		fp = fopen(filename, "rb");
		if(fp != NULL){
			fseek(fp, 0, SEEK_END);
			count = ftell(fp);
			rewind(fp);
			if (count > 0) {
            	content = (char *)malloc(sizeof(char) * (count+1));
            	count = fread(content,sizeof(char),count,fp);
            	content[count] = '\0';
        	}
        }
        fclose(fp);
	}
	*size = count;
	return content;
}

VkFormat findSupportedFormat(VkFormat *candidates, int numFormats, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for(int i = 0; i < numFormats; i++) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, candidates[i], &props);

		if(tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return candidates[i];
		} else if(tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return candidates[i];
		}
	}

	printf("No device formats supported.");
	cleanup();
	return 0;
}

VkFormat findDepthFormat() {
	VkFormat candidates[3] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	return findSupportedFormat(candidates, 3, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void createRenderPass() {
	VkAttachmentDescription colorAttachment = {
		.format = swapChainImageFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
	};

	VkAttachmentDescription depthAttachment = {
		.format = findDepthFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pDepthStencilAttachment = &depthAttachmentRef,
	};

	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
	};

	VkAttachmentDescription attachments[2];
	attachments[0] = colorAttachment;
	attachments[1] = depthAttachment;

	VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = 2,
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};

	if(vkCreateRenderPass(device, &renderPassInfo, NULL, &renderPass) != VK_SUCCESS) {
		printf("Failed to create render pass.\n");
		cleanup();
	}
}

void createGraphicsPipeline() {

	size_t vertSize, fragSize;
	char *vertShaderCode = readShader("shaders/vert.spv", &vertSize);
	char *fragShaderCode = readShader("shaders/frag.spv", &fragSize);

	VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, vertSize);
	VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, fragSize);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertShaderModule,
		.pName = "main",
	};

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragShaderModule,
		.pName = "main",
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};


	VkVertexInputBindingDescription bindingDescription = getBindingDescription();
	VkVertexInputAttributeDescription *attributeDescriptions;
	attributeDescriptions = getAttributeDescriptions();
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = &bindingDescription,
		.vertexAttributeDescriptionCount = 3,
		.pVertexAttributeDescriptions = attributeDescriptions,
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = (float)swapChainExtent.width,
		.height = (float)swapChainExtent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = swapChainExtent,
	};

	VkPipelineViewportStateCreateInfo viewportState = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor,
	};

	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f,
		.depthBiasClamp = 0.0f,
		.depthBiasSlopeFactor = 0.0f,
	};

	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f,
		.pSampleMask = NULL,
		.alphaToCoverageEnable = VK_FALSE,
		.alphaToOneEnable = VK_FALSE,
	};

	VkPipelineDepthStencilStateCreateInfo depthStencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
		.stencilTestEnable = VK_FALSE,
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
		//.srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
		//.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
		//.colorBlendOp = VK_BLEND_OP_ADD,
		//.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
		//.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		//.alphaBlendOP = VK_BLEND_OP_ADD,
	};

	//colorBlendAttachment.blendescriptorSetLayoutdEnable = VK_TRUE;
	//colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	//colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	//colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	//colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	//colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	//colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants[0] = 0.0f,
		.blendConstants[1] = 0.0f,
		.blendConstants[2] = 0.0f,
		.blendConstants[3] = 0.0f
	};

	//VkDynamicState dynamicStates[] = {
	//	VK_DYNAMIC_STATE_VIEWPORT,
	//	VK_DYNAMIC_STATE_LINE_WIDTH,
	//};
	//VkPipelineDynamicStateCreate_info dynamicState = {
	//	.sType = VVK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
	//	.dynamicStateCount = 2,
	//	.dynamicStates = dynamicStates
	//};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout,
		//.pSetLayouts = NULL,
		//.pushConstantRangeCount = 0,
		//.pPushConstantRanges = 0,
	};

	if(vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS) {
		printf("Failed to create pipeline layout.\n");
		cleanup();
	};

	VkGraphicsPipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = 2,
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depthStencil,
		.pColorBlendState = &colorBlending,
		.pDynamicState = NULL,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};

	if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &graphicsPipeline) != VK_SUCCESS) {
		printf("Failed to create graphics pipeline.\n");
		cleanup();
	}

	vkDestroyShaderModule(device, vertShaderModule, NULL);
	vkDestroyShaderModule(device, fragShaderModule, NULL);

	//printf("%zd, %zd\n", vertSize, fragSize);
}

void createFramebuffers() {
	swapChainFramebuffers = malloc(globalImageCount*sizeof(VkFramebuffer));
	for(unsigned int i = 0; i < globalImageCount; i++) {
		VkImageView attachments[2] = {
			swapChainImageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = renderPass,
			.attachmentCount = 2,
			.pAttachments = attachments,
			.width = swapChainExtent.width,
			.height = swapChainExtent.height,
			.layers = 1,
		};

		if(vkCreateFramebuffer(device, &framebufferInfo, NULL, &swapChainFramebuffers[i]) != VK_SUCCESS) {
			printf("Failed to create framebuffer.\n");
			cleanup();
		}
	}
}

void createCommandPool() {
	QueueFamilyIndices *queueFamilyIndices = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.queueFamilyIndex = queueFamilyIndices->graphicsFamily,
		.flags = 0,
	};

	if(vkCreateCommandPool(device, &poolInfo, NULL, &commandPool) != VK_SUCCESS) {
		printf("Failed to create command pool.\n");
	}

}

VkCommandBuffer beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandPool = commandPool,
		.commandBufferCount = 1,
	};
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
	};
	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void createCommandBuffer() {
	commandBuffers = malloc(globalImageCount*sizeof(VkCommandBuffer));
	VkCommandBufferAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = commandPool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (uint32_t)globalImageCount,
	};

	if(vkAllocateCommandBuffers(device, &allocInfo, commandBuffers) != VK_SUCCESS) {
		printf("Failed to allocate command buffers.\n");
		cleanup();
	}

	for(unsigned int i = 0; i < globalImageCount; i++) {
		VkCommandBufferBeginInfo beginInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
			.pInheritanceInfo = NULL,
		};
		vkBeginCommandBuffer(commandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = renderPass,
			.framebuffer = swapChainFramebuffers[i],
			.renderArea.offset = {0, 0},
			.renderArea.extent = swapChainExtent,
		};

		VkClearValue clearColor = {.color = {0.0f, 0.0f, 0.0f, 1.0f},};
		VkClearValue clearDepth = {.depthStencil = {1.0f, 0},};

		VkClearValue clearValues[2];// = malloc(2*sizeof(VkClearValue));
		clearValues[0] = clearColor;
		clearValues[1] = clearDepth;

		//VkClearValue clearColor = {0.0f, 0.1f, 0.3f, 1.0f};
		renderPassInfo.clearValueCount = 2;
		renderPassInfo.pClearValues = clearValues;

		vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			VkBuffer vertexBuffers[] = {vertexBuffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
			vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[i], 0, NULL);
			//vkCmdDraw(commandBuffers[i], (uint32_t)(sizeof(vertices)/sizeof(vertices[0])), 1, 0, 0);
			vkCmdDrawIndexed(commandBuffers[i], (uint32_t)(sizeof(vertexIndices)/sizeof(vertexIndices[0])), 1, 0, 0, 0);
			//printf("(uint32_t)sizeof(vertices): %d\n", (uint32_t)sizeof(vertices)/sizeof(vertices[0]));

		vkCmdEndRenderPass(commandBuffers[i]);

		if(vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
			printf("Failed to record command buffer.\n");
			cleanup();
		}
	}
}

void createSemaphores() {
	VkSemaphoreCreateInfo semaphoreInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};
	if(vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphore) != VK_SUCCESS
	|| vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphore) != VK_SUCCESS) {
		printf("Failed to create semaphores.\n");
		cleanup();
	}
}

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if(typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	printf("Failed to find a suitable memory type.\n");
	cleanup();
	return 2;
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer *buffer, VkDeviceMemory *bufferMemory) {
		VkBufferCreateInfo bufferInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	if(vkCreateBuffer(device, &bufferInfo, NULL, buffer) != VK_SUCCESS) {
		printf("Failed to create vertex buffer.\n");
		cleanup();
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties),
	};

	if(vkAllocateMemory(device, &allocInfo, NULL, bufferMemory) != VK_SUCCESS) {
		printf("Failed to allocate vertex buffer memory.\n");
		cleanup();
	}
	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy copyRegion = {
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size,
	};
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	endSingleTimeCommands(commandBuffer);
}

void createVertexBuffer() {
	VkDeviceSize size = sizeof(vertices);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, vertices, sizeof(vertices));
	//printf("asdf: %zu\nS", sizeof(vertices));
	//for(int i = 0; i < 15; i++) {
	//	printf("%f\n", *((float *) ((char *) data + sizeof(float) * i)));
	//	//printf("%f %f\n", testData->color.x, testData->color.y);
	//}
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertexBuffer, &vertexBufferMemory);
	copyBuffer(stagingBuffer, vertexBuffer, size);

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);
}

void createIndexBuffer() {
	VkDeviceSize size = sizeof(vertexIndices);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, size, 0, &data);
	memcpy(data, vertexIndices, sizeof(vertexIndices));
	vkUnmapMemory(device, stagingBufferMemory);

	createBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &indexBuffer, &indexBufferMemory);
	copyBuffer(stagingBuffer, indexBuffer, size);

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);
}

void createDescriptorSetLayout() {
	VkDescriptorSetLayoutBinding uboLayoutBinding = {
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		.pImmutableSamplers = NULL,
	};

	VkDescriptorSetLayoutBinding samplerLayoutBinding = {
		.binding = 1,
		.descriptorCount = 1,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImmutableSamplers = NULL,
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
	};

	VkDescriptorSetLayoutBinding *bindings = malloc(2*sizeof(VkDescriptorSetLayoutBinding));
	bindings[0] = uboLayoutBinding;
	bindings[1] = samplerLayoutBinding;

	VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 2,
		.pBindings = bindings,
	};

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout) != VK_SUCCESS) {
		printf("Failed to create descriptor set layout.");
		cleanup();
	}
}

void createUniformBuffer() {

	VkDeviceSize bufferSize = sizeof(uniformBufferObject);
	uniformBuffer = malloc(sizeof(VkBuffer)*globalImageCount);
	uniformBufferMemory = malloc(sizeof(VkDeviceMemory)*globalImageCount);

	for(unsigned int i = 0; i < globalImageCount; i++) {
		createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &uniformBuffer[i], &uniformBufferMemory[i]);
	}
}

void createDescriptorSets() {
	VkDescriptorSetLayout *layouts;
	layouts = malloc(sizeof(VkDescriptorSetLayout)*globalImageCount);

	for(unsigned int i = 0; i < globalImageCount; i++) {
		layouts[i] = descriptorSetLayout;
	}
	VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = descriptorPool,
		.descriptorSetCount = globalImageCount,
		.pSetLayouts = layouts,
	};

	descriptorSets = malloc(sizeof(VkDescriptorSet)*globalImageCount);
	if(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets) != VK_SUCCESS) {
		printf("Failed to allocate descriptor sets.");
	}

	for(unsigned int i = 0; i < globalImageCount; i++) {
		VkDescriptorBufferInfo bufferInfo = {
			.buffer = uniformBuffer[i],
			.offset = 0,
			.range = sizeof(uniformBufferObject),
		};

		VkDescriptorImageInfo imageInfo = {
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.imageView = textureImageView,
			.sampler = textureSampler,
		};

		VkWriteDescriptorSet descriptorWrite = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.pBufferInfo = &bufferInfo,
			.pImageInfo = NULL,
			.pTexelBufferView = NULL,
		};

		VkWriteDescriptorSet descriptorWriteImage = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSets[i],
			.dstBinding = 1,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.pImageInfo = &imageInfo,
		};

		VkWriteDescriptorSet *descriptorWrites = malloc(2*sizeof(VkWriteDescriptorSet));
		descriptorWrites[0] = descriptorWrite;
		descriptorWrites[1] = descriptorWriteImage;

		vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, NULL);
	}
}

void createDescriptorPool() {

	VkDescriptorPoolSize *poolSizes = malloc(2*sizeof(VkDescriptorPoolSize));
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = globalImageCount;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = globalImageCount;

	VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.poolSizeCount = 2,
		.pPoolSizes = poolSizes,
		.maxSets = globalImageCount,
	};

	if(vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptorPool) != VK_SUCCESS) {
		printf("Failed to create descriptor pool.");
		cleanup();
	}
}

void createImage(unsigned int width, unsigned int height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *imageMemory) {
	VkImageCreateInfo imageInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = width,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = format,
		.tiling = tiling,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = usage,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	if(vkCreateImage(device, &imageInfo, NULL, image) != VK_SUCCESS) {
		printf("Failed to create image");
		cleanup();
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, *image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties),
	};

	if(vkAllocateMemory(device, &allocInfo, NULL, imageMemory) != VK_SUCCESS) {
		printf("Failed to allocate image memory");
	}

	vkBindImageMemory(device, *image, *imageMemory, 0);
}

int hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
	};

	if(newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if(hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = 0;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if(oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if(oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		printf("Unsupported layout.");
		cleanup();
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage,
		destinationStage,
		0,
		0, NULL,
		0, NULL,
		1, &barrier
	);

	endSingleTimeCommands(commandBuffer);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, unsigned int width, unsigned int height) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.mipLevel = 0,
		.imageSubresource.baseArrayLayer = 0,
		.imageSubresource.layerCount = 1,

		.imageOffset = {0,0,0},
		.imageExtent = {width,height,1},
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	endSingleTimeCommands(commandBuffer);
}

void createTextureImage() {
	int texWidth, texHeight, texChannels;
	stbi_uc *pixels = stbi_load("textures/earth.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	VkDeviceSize imageSize = texWidth * texHeight * 4;

	if(!pixels) {
		printf("Failed to load texture");
		cleanup();
	}

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &stagingBuffer, &stagingBufferMemory);

	void *data;
	vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, (const void *)pixels, imageSize);
	vkUnmapMemory(device, stagingBufferMemory);

	stbi_image_free(pixels);

	//createImage(unsigned int width, unsigned int height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage *image, VkDeviceMemory *imageMemory)
	createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &textureImage, &textureImageMemory);

	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	copyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);
	transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);

}

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.subresourceRange.aspectMask = aspectFlags,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount =1,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
	};

	VkImageView imageView;
	if(vkCreateImageView(device, &viewInfo, NULL, &imageView) != VK_SUCCESS) {
		printf("Failed to create texture image view.");
		cleanup();
	}

	return imageView;
}

void createTextureImageView() {
	textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
}

void createTextureSampler() {
	VkSamplerCreateInfo samplerInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = 16,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.mipLodBias = 0.0f,
		.minLod = 0.0f,
		.maxLod = 0.0f,
	};

	if (vkCreateSampler(device, &samplerInfo, NULL, &textureSampler) != VK_SUCCESS) {
		printf("Failed to create texture sampler");
		cleanup();
	}
}

void createDepthResources() {
	VkFormat depthFormat = findDepthFormat();

	createImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depthImage, &depthImageMemory);
	depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);


	transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	}

void initVulkan() {
	createInstance();
	setupDebugCallback();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();

	createDescriptorSetLayout();

	createGraphicsPipeline();
	createCommandPool();
	createDepthResources();
	createFramebuffers();
	createTextureImage();
	createTextureImageView();
	createTextureSampler();
	createVertexBuffer();
	createIndexBuffer();
	createUniformBuffer();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffer();
	createSemaphores();
}

int getWindowHeight() {
	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	return mode->height;
}

int getWindowWidth() {
	const GLFWvidmode * mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	return mode->width;
}

void recreateSwapChain() {
	int width, height;
    glfwGetWindowSize(window, &width, &height);
    if (width == 0 || height == 0) return;
	vkDeviceWaitIdle(device);

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthResources();
	createFramebuffers();
	createCommandBuffer();
}


static void onWindowResized(GLFWwindow *window, int width, int height) {
	recreateSwapChain();
}

void initWindow() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	window = glfwCreateWindow(400, 400, "Vulkan", NULL, NULL);

	glfwSetWindowSizeCallback(window, onWindowResized);
}

void updateUniformBuffer(double *deltaTime, double *lastFrame, uint32_t currentImage) {
	double currentFrame = glfwGetTime();
	*deltaTime = currentFrame - *lastFrame;
	*lastFrame = currentFrame;

	mat4 m = scale(0.5);
	mat4 v = getViewMatrix();
	mat4 p = perspective(45.0, swapChainExtent.width / swapChainExtent.height, 0.5, 100000);
	p.m[1][1] *= -1;

	uniformBufferObject ubo[1] = {
		{{
		(float)m.m[0][0], (float)m.m[0][1], (float)m.m[0][2], (float)m.m[0][3],
		(float)m.m[1][0], (float)m.m[1][1], (float)m.m[1][2], (float)m.m[1][3],
		(float)m.m[2][0], (float)m.m[2][1], (float)m.m[2][2], (float)m.m[2][3],
		(float)m.m[3][0], (float)m.m[3][1], (float)m.m[3][2], (float)m.m[3][3]},
		{
		(float)v.m[0][0], (float)v.m[0][1], (float)v.m[0][2], (float)v.m[0][3],
		(float)v.m[1][0], (float)v.m[1][1], (float)v.m[1][2], (float)v.m[1][3],
		(float)v.m[2][0], (float)v.m[2][1], (float)v.m[2][2], (float)v.m[2][3],
		(float)v.m[3][0], (float)v.m[3][1], (float)v.m[3][2], (float)v.m[3][3]},
		{
		(float)p.m[0][0], (float)p.m[0][1], (float)p.m[0][2], (float)p.m[0][3],
		(float)p.m[1][0], (float)p.m[1][1], (float)p.m[1][2], (float)p.m[1][3],
		(float)p.m[2][0], (float)p.m[2][1], (float)p.m[2][2], (float)p.m[2][3],
		(float)p.m[3][0], (float)p.m[3][1], (float)p.m[3][2], (float)p.m[3][3]}}

	};

	//printf("asdf: %zu, %zu\n", sizeof(ubo), sizeof(uniformBufferObject));
	void *data;
	vkMapMemory(device, uniformBufferMemory[currentImage], 0, sizeof(ubo), 0, &data);
	memcpy(data, (const void *)&ubo[0], sizeof(ubo));
	vkUnmapMemory(device, uniformBufferMemory[currentImage]);

	//for(int i = 0; i < 48; i++) {
	//	printf("%f, ", *((float *) ((char *) data + sizeof(float) * i)));
	//	if((i+1)%4 == 0) printf("\n");
	//}
	//printf("\n");
}


void drawFrame() {
	double deltaTime = 0.0;
	double lastFrame = 0.0;

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);


	if(result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain();
		return;
	}
	else if(result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		printf("Failed to acquire swap chain image.\n");
		cleanup();
	}
	updateUniformBuffer(&deltaTime, &lastFrame, imageIndex);

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	};
	VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if(vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		printf("Failed to submit draw command buffer.\n");
		cleanup();
	}

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,
	};

	VkSwapchainKHR swapChains[] = {swapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		recreateSwapChain();
	}
	else if(result != VK_SUCCESS) {
		printf("Failed to present swap chain image.\n");
		cleanup();
	}

	vkQueueWaitIdle(presentQueue);
}

void mainLoop() {
	while(!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		drawFrame();
	}
	vkDeviceWaitIdle(device);
}

void run() {
	initWindow();
	if (window == NULL)
	{
		printf("Failed to create GLFW window.");
		glfwTerminate();
		cleanup();
	}
	if(!glfwVulkanSupported()) {
		printf("Vulkan not supported.");
		cleanup();
	}
	initVulkan();
	mainLoop();

	cleanup();
}


int main(int argc, char *argv[]) {
	run();
	printf("Program finished");
	return 0;
}