#include "block_hasher.h"
#include "md5.h"

#include <exception>
#include <thread>
#include <fstream>
#include <iostream>
#include <utility>
#include <functional>

using namespace std;

static pair<shared_ptr<ifstream>, shared_ptr<ofstream>> openFiles(const string &inputFile, const string &outputFile)
{
    auto input = make_shared<ifstream>(inputFile, ios::binary);
    auto output = make_shared<ofstream>(outputFile, ios::app);

    if (!input->is_open())
    {
        throw invalid_argument("cannot open file " + inputFile);
    }

    if (!output->is_open())
    {
        throw invalid_argument("cannot open file " + outputFile);
    }

    return make_pair(input, output);
}

SingleThreadHasher::SingleThreadHasher(size_t blockSize) : BlockHasher(blockSize)
{
}

void SingleThreadHasher::Hash(const string &inputFile, const string &outputFile)
{
    auto [input, output] = openFiles(inputFile, outputFile);
    vector<uint8_t> data;
    data.reserve(_size);

    uint8_t b;
    while (input->read(reinterpret_cast<char *>(&b), sizeof(b)))
    {
        data.push_back(b);

        if (data.size() == _size)
        {
            *output << md5(data.data(), data.size()) << endl;
            data.clear();
        }
    }

    if (!data.empty())
    {
        *output << md5(data.data(), data.size()) << endl;
    }
}


MultiThreadHasher::MultiThreadHasher(size_t blockSize, size_t threads) :
    BlockHasher(blockSize)
{
    _threads = threads < 1 ? 1 : threads;
}

void MultiThreadHasher::Hash(const std::string &inputFile, const std::string &outputFile)
{
    auto [input, output] = openFiles(inputFile, outputFile);
    vector<uint8_t> data;
    data.reserve(_size);

    _run = true;
    thread writer(&MultiThreadHasher::writerThread, this, output);

    uint8_t b;
    while (input->read(reinterpret_cast<char *>(&b), sizeof(b)))
    {
        data.push_back(b);

        if (data.size() == _size)
        {
            addHasherThread(data);
        }
    }

    if (!data.empty())
    {
        addHasherThread(data);
    }

    _run = false;

    if (writer.joinable())
    {
        writer.join();
    }
}

string MultiThreadHasher::hashOneBlock(vector<uint8_t> data)
{
    auto result = md5(data.data(), data.size());
    _writerCv.notify_all();
    return result;
}

void MultiThreadHasher::addHasherThread(vector<uint8_t> &data)
{
    unique_lock<mutex> locker(_m);

    if (_resultQueue.size() > _threads)
    {
        _readerCv.wait(locker); // waiting a future to finish
    }

    _resultQueue.push(async(&MultiThreadHasher::hashOneBlock, this, move(data)));
}

void MultiThreadHasher::writerThread(shared_ptr<ostream> output)
{

    string hash;
    while (_run)
    {
        {
            unique_lock<mutex> locker(_m);
            if (_resultQueue.empty())
            {
                _writerCv.wait(locker);
            }

            _resultQueue.front().wait();
            hash = _resultQueue.front().get();
            _resultQueue.pop();
            _readerCv.notify_all();
        }

        *output << hash << "\n";
    }

    while (!_resultQueue.empty())
    {
        _resultQueue.front().wait();
        *output << _resultQueue.front().get() << "\n";
        _resultQueue.pop();
    }
}
