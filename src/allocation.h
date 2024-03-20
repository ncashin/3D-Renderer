#pragma once

enum NGFX_MemoryType{
    NGFX_MEMORY_TYPE_DEVICE_LOCAL,
    NGFX_MEMORY_TYPE_COHERENT,
};
namespace ngfx{
struct MemoryAllocation{
    uint8_t resource_index;
    size_t offset;
    size_t size;
};
}
