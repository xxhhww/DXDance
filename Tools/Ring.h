#pragma once
#include <cstdint>
#include <atomic>

namespace Tool {

    class Ring
    {
    public:
        Ring(uint32_t in_size) : m_size(in_size) {}

        //-------------------------
        // writer methods
        //-------------------------
        uint32_t GetAvailableToWrite() const { return m_size - m_counter; } // how many could be written
        uint32_t GetWriteIndex(uint32_t in_offset = 0) const;
        void Allocate(uint32_t in_n = 1);

        //-------------------------
        // reader methods
        //-------------------------
        uint32_t GetReadyToRead() const { return m_counter; } // how many can be read
        uint32_t GetReadIndex(uint32_t in_offset = 0) const;
        void Free(uint32_t in_n = 1);

    private:
        std::atomic<uint32_t> m_counter{ 0 };
        const uint32_t m_size;
        uint32_t m_writerIndex{ 0 };
        uint32_t m_readerIndex{ 0 };
    };

}