#include "block_hasher.h"
#include "md5.h"

#include <exception>
#include <thread>
#include <fstream>
#include <utility>
#include <functional>
#include <algorithm>

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

        if (count < data->getCapacity())
        {
            break;
        }

        data = make_shared<Buffer>(_size);
    }
}

MultiThreadHasher::MultiThreadHasher(size_t blockSize, size_t threads) :
    BlockHasher(blockSize)
{
    _threads = max(static_cast<size_t>(1), threads);
}

void MultiThreadHasher::Hash(const std::string &inputFile, const std::string &outputFile)
{
    auto [input, output] = openFiles(inputFile, outputFile);
    auto data = make_shared<Buffer>(_size); // buffer for data to process by one thread

    _run = true;
    thread writer(&MultiThreadHasher::writerThread, this, output);

    try // exception handling needed for joining to writer thread
    {
        while (true)
        {
            // Read data from file to buffer and add to processing queue.
            // Reading data for all threads at once is not always memory efficient but
            // can be faster with large block size.
            size_t count = input->readsome(reinterpret_cast<char *>(data->get()), data->getCapacity());
            data->setSize(count);
            addHasherThread(data);

            if (_exceptOccurred ||               // exit cycle if exception was thrown
                    count < data->getCapacity()) // last segment processed
            {
                break;
            }

            data = make_shared<Buffer>(_size);
        }
    }
    catch (const exception &e)
    {
        if (!_exceptOccurred)
        {
            _exceptPtr = current_exception();
            _exceptOccurred = true;
            _writerCv.notify_all(); // notify writer to detect exception
        }
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
    string result;

    try
    {
        result = md5(data->get(), data->getSize());
        _writerCv.notify_all(); // notify writer to begin writing file
    }
    catch (...)
    {
        _writerCv.notify_all(); // notify writer to detect exception
        throw;
    }

    return result;
}

void MultiThreadHasher::addHasherThread(shared_ptr<Buffer> data)
{
    // this method is only used in main method Hash
    // so we don't need to redirect exceptions to eptr
    unique_lock<mutex> locker(_m);

    _readerCv.wait(locker, [this]() { return this->_resultQueue.size() < _threads; }); // waiting a future to finish and free memory
    _resultQueue.push(async(launch::async, &MultiThreadHasher::hashOneBlock, this, data));
}

void MultiThreadHasher::writerThread(shared_ptr<ostream> output)
{
    auto pred = [this]()
    {
        // if exception occured before any items was inserted to queue,
        // we need to awake writer and make it jump over _resultQueue.front().wait()
        if (_exceptOccurred)
        {
            rethrow_exception(_exceptPtr);
        }

        return !this->_resultQueue.empty();
    };

    try
    {
        string hash;
        while (_run)
        {
            {
                unique_lock<mutex> locker(_m);

                _writerCv.wait(locker, pred);
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
