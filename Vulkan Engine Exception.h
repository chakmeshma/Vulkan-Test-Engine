//
// Created by chakmeshma on 20.11.2017.
//

#ifndef VULKAN_TEST_VULKANENGINEEXCEPTION_H
#define VULKAN_TEST_VULKANENGINEEXCEPTION_H

#pragma once

#include <string>

struct VulkanException : public std::exception {
public:
	explicit VulkanException(char const* const _Message);

	virtual const char* what() const noexcept override;

private:
	std::string msg;
};


#endif //VULKAN_TEST_VULKANENGINEEXCEPTION_H
