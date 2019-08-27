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
    std::string hashOneBlock(std::vector<uint8_t> data);
    std::atomic_bool _run;
    void addHasherThread(std::vector<uint8_t> &data);
    void writerThread(std::shared_ptr<std::ostream> output);
};
