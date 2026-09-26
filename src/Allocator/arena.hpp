#pragma once

/*
arane allacoter is can use linear data example many ast nodes and parsers put them 1 indivicual chunck use and free is simply and good as using not perfect
but not worse


*/


class ArenaAllacator{
public:
    inline explicit ArenaAllacator(size_t bytes)  
        :m_size(std::move(bytes))
    {
        m_buffer=static_cast<std::byte*>(malloc(m_size)); // this code mean m_buffer have a memory equal each ast nodes sums
        m_offset = m_buffer;
    }


    template<typename T>
    inline T* alloc()
    {
        void* offset = m_offset;
        m_offset += sizeof(T);
        return static_cast<T*>(offset); /*this code take a param T* and allocate sizeof T and offset memory position example 10.index is a this templete added allocate last 9 index and 10.index sizeof(a)*/

    }

    inline ArenaAllacator(const ArenaAllacator& other) = delete;

    inline ArenaAllacator operator=(const ArenaAllacator& other) = delete;

    inline ~ArenaAllacator()
    {
        free(m_buffer);
    }

private:
    size_t m_size;
    std::byte* m_buffer;/*void because i dont have a single type variable or program*/
    std::byte* m_offset;
};