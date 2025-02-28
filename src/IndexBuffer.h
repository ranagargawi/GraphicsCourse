#pragma once

#include <Debugger.h>

// EBO
class IndexBuffer
{
    private:
        unsigned int m_RendererID;
        unsigned int m_Count;
    public:
        IndexBuffer(const unsigned int* data, unsigned int size);
        ~IndexBuffer();

        void Bind() const;
        void Unbind() const;

        inline unsigned int GetCount() const { return m_Count; }
};