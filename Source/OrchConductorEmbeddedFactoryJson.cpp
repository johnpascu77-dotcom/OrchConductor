#include "OrchConductorEmbeddedFactoryJson.h"

#include <BinaryData.h>

namespace orchconductor
{

EmbeddedFactoryJson getEmbeddedFactoryJson() noexcept
{
    return EmbeddedFactoryJson {
        BinaryData::orchconductor_library_v1_example_json,
        BinaryData::orchconductor_library_v1_example_jsonSize
    };
}

} // namespace orchconductor
