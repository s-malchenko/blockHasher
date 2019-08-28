#include "block_hasher.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <exception>
#include <chrono>

using namespace std;
using namespace std::chrono;

static void printUsage()
{
    cout << "Usage: blockHasher <file to hash> <output file>" << endl;
    cout << "       [-b <block size in bytes, default is 1 MB>]" << endl;
    cout << "       [-m [threads count, default is 4]" << endl;
}

/**
 * @brief      Class for parsing command line options.
 */
class InputParser
{
public:
    InputParser (int &argc, char **argv)
    {
        for (int i = 1; i < argc; ++i)
            this->tokens.push_back(std::string(argv[i]));
    }

    const std::string &getCmdOption(const std::string &option) const
    {
        std::vector<std::string>::const_iterator itr;
        itr =  std::find(this->tokens.begin(), this->tokens.end(), option);
        if (itr != this->tokens.end() && ++itr != this->tokens.end())
        {
            return *itr;
        }
        static const std::string empty_string("");
        return empty_string;
    }

    bool cmdOptionExists(const std::string &option) const
    {
        return std::find(this->tokens.begin(), this->tokens.end(), option)
               != this->tokens.end();
    }
private:
    std::vector <std::string> tokens;
};

int main(int argc, char **argv)
{
    try
    {
        if (argc < 3)
        {
            printUsage();
            return -1;
        }

        size_t blockSize = 1024 * 1024; // 1 MB default block size
        size_t threads = 0; // 0 for single-thread implementation

        auto parser = InputParser(argc, argv);
        auto sizeStr = parser.getCmdOption("-b");

        if (!sizeStr.empty())
        {
            try
            {
                blockSize = stoll(sizeStr);
            }
            catch (...)
            {
                printUsage();
                return -1;
            }
        }

        unique_ptr<BlockHasher> hasherPtr;
        string input(argv[1]);
        string output(argv[2]);
        auto start = steady_clock::now();

        if (parser.cmdOptionExists("-m"))
        {
            threads = 4; // default number of threads
        }

        auto threadsStr = parser.getCmdOption("-m");

        if (!threadsStr.empty())
        {
            try
            {
                threads = stoll(threadsStr);
            }
            catch (...)
            {
                printUsage();
                return -1;
            }
        }

        if (threads > 0)
        {
            hasherPtr = make_unique<MultiThreadHasher>(blockSize, threads);
            cout << "Multithreading mode, " << threads << " threads" << endl;
        }
        else
        {
            hasherPtr = make_unique<SingleThreadHasher>(blockSize);
            cout << "Single thread mode" << endl;
        }

        cout << "Hashing " << input << " by blocks of " << blockSize <<
             " to file " << output << endl;

        hasherPtr->Hash(input, output);
        auto msec = duration_cast<milliseconds>(steady_clock::now() - start).count();
        cout << "Hashed in " << msec << " milliseconds" << endl;
    }
    catch (const exception &e)
    {
        cout << "Error: " << e.what() << endl;
        return -1;
    }

    return 0;
}
