#pragma once

namespace orchconductor
{

struct EmbeddedFactoryJson
{
    const char* data = nullptr;
    int size = 0;

    bool isValid() const noexcept
    {
        return data != nullptr && size > 0;
    }
};

EmbeddedFactoryJson getEmbeddedFactoryJson() noexcept;

} // namespace orchconductor
