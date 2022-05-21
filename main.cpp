#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

const std::vector<const char *> validation_layers = {
    "VK_LAYER_KHRONOS_validation"};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
                                   VkDebugUtilsMessengerEXT debugMessenger,
                                   const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

class HelloTriangleApplication {
public:
  void run() {
    init_window();
    init_vulkan();
    main_loop();
    cleanup();
  }

private:
  const uint32_t window_width = 800;
  const uint32_t window_height = 600;

  GLFWwindow *window;

  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;

  VkSurfaceKHR surface;

  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkDevice device;
  VkQueue graphics_queue;
  VkQueue present_queue;

  void init_window() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(window_width, window_height, "Vulkan", nullptr,
                              nullptr);
  }

  void init_vulkan() {
    create_instance();
    setup_debug_messenger();
    create_surface();
    pick_physical_device();
    create_logical_device();
  }

  void create_surface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
        VK_SUCCESS) {
      throw std::runtime_error{"Failed to create window surface!"};
    }
  }

  void create_logical_device() {
    QueueFamilyIndices indices = findQueueFamilies(physical_device);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                              indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
      VkDeviceQueueCreateInfo queueCreateInfo{};
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = queueFamily;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &queuePriority;
      queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount =
        static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = 0;
    
    if (enableValidationLayers) {
      createInfo.enabledLayerCount =
          static_cast<uint32_t>(validation_layers.size());
      createInfo.ppEnabledLayerNames = validation_layers.data();
    } else {
      createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &createInfo, nullptr, &device) !=
        VK_SUCCESS) {
      throw std::runtime_error{"Failed to create logical device!"};
    }
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0,
                     &graphics_queue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &present_queue);
  }

  void pick_physical_device() {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
    if (device_count == 0) {
      throw std::runtime_error{"Failed to find GPUs with Vulkan support!"};
    }
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    std::multimap<int, VkPhysicalDevice> candidates;
    for (auto &device : devices) {
      int score = rate_device_suitability(device);
      candidates.insert(std::make_pair(score, device));
    }
    if (candidates.rbegin()->first > 0) {
      physical_device = candidates.rbegin()->second;
    } else {
      throw std::runtime_error{"Failed to find a suitable GPU!"};
    }
    if (physical_device == VK_NULL_HANDLE) {
      throw std::runtime_error{"Failed to find a suitable GPU!"};
    }
  }

  int rate_device_suitability(VkPhysicalDevice device) {
    int score = 0;
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    if (!deviceFeatures.geometryShader) {
      return 0;
    }
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      score += 1000;
    }
    score += deviceProperties.limits.maxImageDimension2D;

    QueueFamilyIndices indices = findQueueFamilies(device);
    if (!indices.isComplete()) {
      return 0;
    }

    return score;
  }

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
    QueueFamilyIndices indices;
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             nullptr);
    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             queue_families.data());
    VkBool32 present_support = false;
    int i = 0;
    for (const auto &queue_family : queue_families) {

      if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {

        indices.graphicsFamily = i;
      }
      vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                           &present_support);
      if (present_support) {
        indices.presentFamily = i;
      }
      if (indices.isComplete()) {
        break;
      }
      i++;
    }

    return indices;
  }

  // bool is_device_suitable(VkPhysicalDevice device) {
  //   VkPhysicalDeviceProperties deviceProperties;
  //   vkGetPhysicalDeviceProperties(device, &deviceProperties);

  //   VkPhysicalDeviceFeatures deviceFeatures;
  //   vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  //   return deviceProperties.deviceType ==
  //              VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
  //          deviceFeatures.geometryShader;
  // }

  void setup_debug_messenger() {
    if (!enableValidationLayers) {
      return;
    }
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populate_debug_messenger_create_info(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                     &debug_messenger) != VK_SUCCESS) {
      throw std::runtime_error{"Failed to set up debug messenger!"};
    }
  }
  void populate_debug_messenger_create_info(
      VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;
  }

  void main_loop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    vkDestroyDevice(device, nullptr);
    if (enableValidationLayers) {
      DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr); // before vkDestroyInstance
    vkDestroyInstance(instance, nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  void create_instance() {
    if (enableValidationLayers && !check_validation_layer_support()) {
      throw std::runtime_error{
          "Validation layers requested, byt not available!"};
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    if (enableValidationLayers) {
      createInfo.enabledLayerCount =
          static_cast<uint32_t>(validation_layers.size());
      createInfo.ppEnabledLayerNames = validation_layers.data();
      populate_debug_messenger_create_info(debugCreateInfo);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;

    } else {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;
    }

    // get extentions
    auto extensions = get_required_extensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    auto mes = vkCreateInstance(&createInfo, nullptr, &instance);
    if (mes != VK_SUCCESS) {
      throw std::runtime_error("Failed to create instance!");
    }
  }

  bool check_validation_layer_support() {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char *layer_name : validation_layers) {
      bool layer_found = false;
      for (const auto &layer_properties : available_layers) {
        if (strcmp(layer_name, layer_properties.layerName) == 0) {
          layer_found = true;
          break;
        }
      }
      if (!layer_found) {
        return false;
      }
    }
    return true;
  }

  std::vector<const char *> get_required_extensions() {
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
    std::vector<const char *> extensions(
        glfw_extensions, glfw_extensions + glfw_extension_count);
    if (enableValidationLayers) {
      extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    // // print available extensions
    // uint32_t extension_count = 0;
    // vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
    // nullptr); std::vector<VkExtensionProperties> extensions(extension_count);
    // vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
    //                                        extensions.data());
    // std::cout << "Available extensions:\n";
    // for (const auto &extension : extensions) {
    //   std::cout << "\t" << extension.extensionName << std::endl;
    // }
    // std::cout << "Wanted extensions:\n";
    // for (uint32_t i = 0; i < glfw_extension_count; ++i) {
    //   std::cout << "\t" << glfw_extensions[i] << std::endl;
    // }
    return extensions;
  }

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                void *pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }
};

int main() {
  HelloTriangleApplication app;
  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}