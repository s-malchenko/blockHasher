#include "block_hasher.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <exception>

using namespace std;

static void printUsage()
{
    cout << "Usage: blockHasher <file to hash> <output file>" << endl;
    cout << "       [-b <block size in bytes, default is 1 MB>]" << endl;
}

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
    if (argc < 3)
    {
        printUsage();
        return -1;
    }

    size_t blockSize = 1024 * 1024; // 1 MB default block size

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

    string input(argv[1]);
    string output(argv[2]);

    try
    {
        unique_ptr<BlockHasher> hasherPtr = make_unique<SingleThreadHasher>(blockSize);
        hasherPtr->Hash(input, output);
    }
    catch (const exception &e)
    {
        cout << "Error: " << e.what() << endl;
        return -1;
    }

    return 0;
}
