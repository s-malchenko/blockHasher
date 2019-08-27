#include "block_hasher.h"
#include "md5.h"

#include <fstream>
#include <exception>
#include <iostream>
#include <vector>
#include <cstdint>

using namespace std;

SingleThreadHasher::SingleThreadHasher(size_t blockSize) : BlockHasher(blockSize)
{
}

void SingleThreadHasher::Hash(const string &inputFile, const string &outputFile)
{
    ifstream input(inputFile, ios::binary);
    ofstream output(outputFile, ios::app);

    if (!input.is_open())
    {
        throw invalid_argument("cannot open file " + inputFile);
    }

    if (!output.is_open())
    {
        throw invalid_argument("cannot open file " + outputFile);
    }

    vector<uint8_t> data;
    data.reserve(_size);
    cout << "size " << _size << endl;

    size_t bytes = 0;
    uint8_t b;
    while (input.read(reinterpret_cast<char *>(&b), sizeof(b)))
    {
        ++bytes;
        data.push_back(b);

        if (data.size() == _size)
        {
            output << md5(data.data(), data.size()) << endl;
            data.clear();
        }
    }

    if (!data.empty())
    {
        output << md5(data.data(), data.size()) << endl;
    }
}
