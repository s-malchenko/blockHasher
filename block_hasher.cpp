#include "block_hasher.h"
#include "md5.h"

#include <exception>
#include <thread>
#include <fstream>
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
    auto data = make_shared<Buffer>(_size);

    while (true)
    {
        size_t count = input->readsome(reinterpret_cast<char *>(data->get()), data->getCapacity());
        data->setSize(count);
        *output << md5(data->get(), data->getSize()) << endl;

        if (count < _size)
        {
            break;
        }

        data = make_shared<Buffer>(_size);
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
    auto data = make_shared<Buffer>(_size);

    _run = true;
    thread writer(&MultiThreadHasher::writerThread, this, output);

    while (true)
    {
        // read data from file to buffer and add to processing queue
        size_t count = input->readsome(reinterpret_cast<char *>(data->get()), data->getCapacity());
        data->setSize(count);
        addHasherThread(data);

        if (count < _size) // last segment
        {
            break;
        }

        if (_exceptOccurred)
        {
            if (writer.joinable()) // detach writer thread for avoiding termination
            {
                writer.detach();
            }

            rethrow_exception(_exceptPtr); // current method is always in main thread so we can safely rethrow
        }

        data = make_shared<Buffer>(_size);
    }

    _run = false;

    if (writer.joinable())
    {
        writer.join();
    }

    if (_exceptOccurred)
    {
        rethrow_exception(_exceptPtr); // current method is always in main thread so we can safely rethrow
    }
}

string MultiThreadHasher::hashOneBlock(shared_ptr<Buffer> data)
{
    try
    {
        auto result = md5(data->get(), data->getSize());
        _writerCv.notify_all(); // notify writer to begin writing file
        return result;
    }
    catch (const exception &e)
    {
        if (!_exceptOccurred)
        {
            _exceptPtr = current_exception();
            _exceptOccurred = true;
        }

        _readerCv.notify_all(); // awake probably sleeping thread to detect exception
    }

    return {};
}

void MultiThreadHasher::addHasherThread(shared_ptr<Buffer> data)
{
    // this method is only used in main method Hash
    // so we don't need to redirect exceptions to eptr
    unique_lock<mutex> locker(_m);

    if (_resultQueue.size() > _threads)
    {
        _readerCv.wait(locker); // waiting a future to finish and free memory
    }

    _resultQueue.push(async(&MultiThreadHasher::hashOneBlock, this, data));
}

void MultiThreadHasher::writerThread(shared_ptr<ostream> output)
{
    try
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

                if (_exceptOccurred)
                {
                    _readerCv.notify_all(); // awake sleeping thread to detect exception
                    return; // immediatly return for avoiding wrong data in output file
                }

                hash = _resultQueue.front().get();
                _resultQueue.pop();
                _readerCv.notify_all();
            } // end of mutex-blocking code, file output witout block

            *output << hash << endl;
        }

        while (!_resultQueue.empty())
        {
            _resultQueue.front().wait();
            *output << _resultQueue.front().get() << endl;
            _resultQueue.pop();
        }
    }
    catch (const exception &e)
    {
        if (!_exceptOccurred)
        {
            _exceptPtr = current_exception();
            _exceptOccurred = true;
        }

        _readerCv.notify_all(); // awake sleeping thread to detect exception
    }
}
