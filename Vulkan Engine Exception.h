// Created by chakmeshma on 20.11.2017.
#pragma once

#include <string>


struct VulkanException : public std::exception {
public:
	explicit VulkanException(char const* const _Message);

	virtual const char* what() const noexcept override;

private:
	std::string msg;
};

