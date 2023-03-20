#include "Ring.h"
#include "Assert.h"

namespace Tool {


    uint32_t Ring::GetWriteIndex(uint32_t in_offset) const {
        ASSERT_FORMAT(in_offset < GetAvailableToWrite(), "Unavailable Write Index");
        return (m_writerIndex + in_offset) % m_size;
    }

    void Ring::Allocate(uint32_t in_n) {
        ASSERT_FORMAT((m_counter + in_n) <= m_size, "Out Of Range");
        m_writerIndex += in_n;
        m_counter += in_n;
    }

    uint32_t Ring::GetReadIndex(uint32_t in_offset) const {
        ASSERT_FORMAT(in_offset < GetReadyToRead(), "Unavailable Read Index");
        return (m_readerIndex + in_offset) % m_size;
    }

    void Ring::Free(uint32_t in_n) {
        ASSERT_FORMAT(m_counter >= in_n, "Out Of Range");
        m_readerIndex += in_n;
        m_counter -= in_n;
    }

}