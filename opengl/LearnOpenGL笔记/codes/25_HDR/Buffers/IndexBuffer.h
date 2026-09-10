#pragma once

#include <cstdint>
#include <span>

#include "Buffer.h"


class IndexBuffer : public Buffer {
public:
    IndexBuffer();
    IndexBuffer(std::span<const std::uint32_t> indices);

    void setData(std::span<const std::uint32_t> indices);
    size_t count() const;
private:
    size_t count_{ 0 };
};