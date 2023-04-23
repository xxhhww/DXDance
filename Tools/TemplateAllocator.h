#pragma once
#include <vector>

template<typename T> class AlignedAllocator : public std::allocator<T>
{
public:
    AlignedAllocator(uint32_t in_alignment = 256u) : m_alignment(in_alignment) {} // default is both UINT32 SIMD8 and D3D12_TEXTURE_DATA_PITCH_ALIGNMENT
    uint32_t GetAlignment() { return m_alignment; }
    T* allocate(std::size_t n) { return (T*)_aligned_malloc(n * sizeof(T), m_alignment); }
    void deallocate(T* p, std::size_t n) { _aligned_free(p); }
private:
    const uint32_t m_alignment;
};