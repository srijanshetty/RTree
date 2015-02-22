/*
 * Copyright (c) 2015 Srijan R Shetty <srijan.shetty+code@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


// Configuration parameters
#include "./config.h"

// CONSTANTS
#define NODE_PREFIX "leaves/leaf_"
#define DEFAULT -1

// Standard Streams
#include <iostream>
#include <fstream>

// STL
#include <string>
#include <cstring>
#include <vector>
#include <queue>
#include <algorithm>

// Math
#include <math.h>
#include <limits>

// Timing
#include <chrono>

#include <stdlib.h>

using namespace std;

namespace RTree {
    class Node {
        private:
            static long nodeCount;
            static long lowerBound;
            static long upperBound;

        private:
            // Entries required to completely specify a node
            bool leaf = false;
            long fileIndex = DEFAULT;
            long parentIndex = DEFAULT;
            long sizeOfSubtree = DEFAULT;
            vector<long> upperPoints = vector<long>(DIMENSION, DEFAULT);
            vector<long> lowerPoints = vector<long>(DIMENSION, DEFAULT);

            // A node can store either of the below
            vector<long> childIndices;
            vector<long> objectIndices;

        public:
            // Construct a node object for the first time
            Node ();

            //  Read a node from disk
            Node (long _fileIndex);

            // Initialize the lower and upper bounds
            static void initialize();

            // Get the name of the file
            string getFileName() { return NODE_PREFIX + to_string(fileIndex); };

            // Get the role of the node
            bool isLeaf() { return leaf; }

            // Store the node to disk
            void storeNodeToDisk();

            // Read the node from the disk
            void readNodeFromDisk();
    };
};

int main() {
    return 0;
}
