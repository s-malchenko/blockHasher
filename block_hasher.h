#pragma once

#include <string>
#include <future>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <cstdint>
#include <ostream>
#include <atomic>
#include <vector>
#include <memory>

class BlockHasher
{
public:
    BlockHasher(size_t blockSize) : _size(blockSize) {}
    virtual void Hash(const std::string &inputFile, const std::string &outputFile) = 0;
protected:
    class Buffer
    {
    public:
        Buffer(size_t capacity) : _capacity(capacity)
        {
            _ptr = new uint8_t[capacity];
        }
        ~Buffer()
        {
            delete[] _ptr;
        }

        uint8_t *get()
        {
            return _ptr;
        }

        void setSize(size_t newSize)
        {
            _dataSize = newSize < _capacity ? newSize : _capacity;
        }

        size_t getSize() const { return _dataSize; }
    private:
        uint8_t *_ptr;
        size_t _dataSize = 0;
        size_t _capacity;
    };
    size_t _size;
};

class SingleThreadHasher : public BlockHasher
{
public:
    SingleThreadHasher(size_t blockSize = 1024 * 1024);
    virtual void Hash(const std::string &inputFile, const std::string &outputFile) override;
};

class MultiThreadHasher : public BlockHasher
{
public:
    MultiThreadHasher(size_t blockSize = 1024 * 1024, size_t threads = 4);
    virtual void Hash(const std::string &inputFile, const std::string &outputFile) override;
private:
    size_t _threads;
    std::mutex _m;
    std::queue<std::future<std::string>> _resultQueue;
    std::condition_variable _writerCv, _readerCv;
    std::string hashOneBlock(std::shared_ptr<Buffer> data);
    std::atomic_bool _run;
    void addHasherThread(std::shared_ptr<Buffer> data);
    void writerThread(std::shared_ptr<std::ostream> output);
};
