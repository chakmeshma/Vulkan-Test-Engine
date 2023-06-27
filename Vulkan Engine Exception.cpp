#include "Vulkan Engine Exception.h"


VulkanException::VulkanException(char const* const _Message) {
	msg.resize(strlen(_Message));
	msg.assign(_Message);
}

const char* VulkanException::what() const noexcept {
	return msg.data();
}