#pragma once

#include <string>

class BlockHasher
{
public:
    BlockHasher(size_t blockSize = 1024 * 1024) : _size(blockSize) {}
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
