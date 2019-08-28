#pragma once

#include <string>
#include <future>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <cstdint>
#include <ostream>
#include <atomic>
#include <memory>
#include <exception>

/**
 * @brief      Abstract class for block hasher.
 */
class BlockHasher
{
public:
    /**
     * @brief      Constructs the block hasher.
     *
     * @param[in]  blockSize  The block size in bytes.
     */
    BlockHasher(size_t blockSize) : _size(blockSize) {}
    virtual void Hash(const std::string &inputFile, const std::string &outputFile) = 0;
protected:
    size_t _size; // block size in bytes

    /**
     * @brief      Simple class for buffer with size and capacity.
     */
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

        size_t getSize() const
        {
            return _dataSize;
        }

        size_t getCapacity() const
        {
            return _capacity;
        }

    private:
        uint8_t *_ptr;
        size_t _dataSize = 0;
        size_t _capacity;
    };
};

/**
 * @brief      Class for single thread hasher.
 */
class SingleThreadHasher : public BlockHasher
{
public:
    SingleThreadHasher(size_t blockSize = 1024 * 1024);
    virtual void Hash(const std::string &inputFile, const std::string &outputFile) override;
};

/**
 * @brief      Class for multi thread hasher.
 */
class MultiThreadHasher : public BlockHasher
{
public:
    MultiThreadHasher(size_t blockSize = 1024 * 1024, size_t threads = 4);
    virtual void Hash(const std::string &inputFile, const std::string &outputFile) override;
private:
    size_t _threads;    // number of simultaneously processed threads
    std::mutex _m;      // mutex for access to _resultQueue
    std::queue<std::future<std::string>> _resultQueue; // queue of calculated hashes
    std::condition_variable _writerCv, _readerCv; // condition variables to wake up reader and writer threads
    std::atomic_bool _run;                        // run flag
    std::exception_ptr _exceptPtr = nullptr;      // ptr for handling exceptions in threads
    std::atomic_bool _exceptOccurred = false;     // exception flag

    /**
     * @brief      Hashes data block.
     *
     * @param[in]  data  The data.
     *
     * @return     Hash.
     */
    std::string hashOneBlock(std::shared_ptr<Buffer> data, size_t offset);

    /**
     * @brief      Adds a hasher future to the queue.
     *
     * @param[in]  data  The data
     */
    void addHasherThread(std::shared_ptr<Buffer> data, size_t offset);

    /**
     * @brief      Thread function for writing calculated hashes to stream.
     *
     * @param[in]  output  Stream to write.
     */
    void writerThread(std::shared_ptr<std::ostream> output);
};
