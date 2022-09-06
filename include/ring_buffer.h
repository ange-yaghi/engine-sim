#ifndef ATG_ENGINE_SIM_RING_BUFFER_H
#define ATG_ENGINE_SIM_RING_BUFFER_H

#include "part.h"

#include <cstring>

template <typename T_Data>
class RingBuffer {
public:
    RingBuffer() {
        m_buffer = nullptr;
        m_capacity = 0;
        m_writeIndex = 0;
        m_start = 0;
    }

    ~RingBuffer() {
        destroy();
    }

    void initialize(size_t capacity) {
        m_buffer = new T_Data[capacity];
        m_capacity = capacity;
        m_writeIndex = 0;
        m_start = 0;
    }

    void destroy() {
        if (m_buffer != nullptr) {
            delete[] m_buffer;
            m_buffer = nullptr;
        }

        m_capacity = 0;
        m_writeIndex = 0;
        m_start = 0;
    }

    inline void write(T_Data data) {
        m_buffer[m_writeIndex] = data;

        if (++m_writeIndex >= m_capacity) {
            m_writeIndex = 0;
        }
    }

    inline void overwrite(T_Data data, size_t index) {
        if (start + index < m_capacity) {
            m_buffer[m_start + index] = data;
        }
        else {
            m_buffer[m_start + index - m_capacity] = data;
        }
    }

    inline size_t index(size_t base, int offset) {
        if (offset == 0) return base;
        else if (offset < 0) {
            const size_t offset_u = -offset;
            if (offset_u <= base) return base - offset_u;
            else return (base + m_capacity) - offset_u;
        }
        else {
            const size_t offset_u = offset;
            const size_t rawOffset = base + offset_u;
            if (rawOffset >= m_capacity) return rawOffset - m_capacity;
            else return rawOffset;
        }
    }

    inline T_Data read(size_t index) const {
        return (m_start + index) >= m_capacity
            ? m_buffer[m_start + index - m_capacity]
            : m_buffer[m_start + index];
    }

    inline void read(size_t n, T_Data *target) {
        if (m_start + n < m_capacity) {
            memcpy(target, m_buffer + m_start, n * sizeof(T_Data));
        }
        else {
            memcpy(
                target,
                m_buffer + m_start,
                (m_capacity - m_start) * sizeof(T_Data));
            memcpy(
                target + (m_capacity - m_start),
                m_buffer,
                (n - (m_capacity - m_start)) * sizeof(T_Data));
        }
    }

    inline void readAndRemove(size_t n, T_Data *target) {
        if (m_start + n < m_capacity) {
            memcpy(target, m_buffer + m_start, n * sizeof(T_Data));
        }
        else {
            memcpy(
                target,
                m_buffer + m_start,
                (m_capacity - m_start) * sizeof(T_Data));
            memcpy(
                target + (m_capacity - m_start),
                m_buffer,
                (n - (m_capacity - m_start)) * sizeof(T_Data));
        }

        m_start += n;
        if (m_start >= m_capacity) {
            m_start -= m_capacity;
        }
    }

    inline void setWriteIndex(size_t writeIndex) {
        m_writeIndex = writeIndex;
    }

    inline void removeBeginning(size_t n) {
        m_start += n;
        if (m_start >= m_capacity) {
            m_start -= m_capacity;
        }
    }

    inline void setStartIndex(size_t startIndex) {
        m_start = startIndex;
    }

    inline size_t size() const {
        return (m_writeIndex < m_start)
            ? m_writeIndex + (m_capacity - m_start)
            : m_writeIndex - m_start;
    }

    inline size_t writeIndex() const {
        return m_writeIndex;
    }

    inline size_t start() const {
        return m_start;
    }

private:
    T_Data *m_buffer;
    size_t m_capacity;
    size_t m_writeIndex;
    size_t m_start;
};

#endif /* ATG_ENGINE_SIM_RING_BUFFER_H */
