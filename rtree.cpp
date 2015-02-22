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
#define SESSION_FILE ".tree.session"
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
            static long fileCount;
            static long lowerBound;
            static long upperBound;

        public:
            // Initialize the lower and upper bounds
            static void initialize();

            // Get the fileCount
            static long getFileCount() { return fileCount; }

            // Set the fileCount
            static void setFileCount(long _fileCount) { Node::fileCount = _fileCount; }

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

            // Get the role of the node
            bool isLeaf() const { return leaf; }

            // Get the index of the file
            long getFileIndex() const { return fileIndex; }

            // Get the name of the file
            string getFileName() const { return NODE_PREFIX + to_string(fileIndex); };

            // Store the node to disk
            void storeNodeToDisk() const;

            // Read the node from the disk
            void readNodeFromDisk();
    };

    // The root of the tree
    Node *RRoot = nullptr;

    // Initial static values
    long Node::fileCount = 0;
    long Node::lowerBound = 0;
    long Node::upperBound = 0;

    Node::Node() {
        // Assign a fileIndex to the current node
        fileIndex = ++fileCount;
    }

    Node::Node(long _fileIndex) {
        // Assign the given fileIndex to the node
        fileIndex = _fileIndex;

        // Load the node from the disk
        readNodeFromDisk();
    }

    void Node::initialize() {
        // Save some place in the file for the header
        long headerSize =
            sizeof(leaf)
            + sizeof(fileIndex)
            + sizeof(parentIndex)
            + sizeof(sizeOfSubtree);
        long pageSize = PAGESIZE - headerSize;
        long keySize = sizeof(childIndices.front());
        long nodeSize = sizeof(fileIndex);

        // Compute the bounds
        upperBound = pageSize / (2 * DIMENSION * keySize + nodeSize);
        lowerBound = upperBound / 2;

        // TODO: clear this up
        upperBound = 10;
        lowerBound = 5;
    }

    void Node::storeNodeToDisk() const {
        char buffer[PAGESIZE];
        long location = 0;

        // Store the contents to disk
        memcpy(buffer + location, &leaf, sizeof(leaf));
        location += sizeof(leaf);

        memcpy(buffer + location, &fileIndex, sizeof(fileIndex));
        location +=sizeof(fileIndex);

        memcpy(buffer + location, &parentIndex, sizeof(parentIndex));
        location += sizeof(parentIndex);

        memcpy(buffer + location, &sizeOfSubtree, sizeof(sizeOfSubtree));
        location += sizeof(sizeOfSubtree);

        // Add the bounds of the MBR to the tree
        for (auto upperPoint : upperPoints) {
            memcpy(buffer + location, &upperPoint, sizeof(upperPoint));
            location += sizeof(upperPoint);
        }

        for (auto lowerPoint : lowerPoints) {
            memcpy(buffer + location, &lowerPoint, sizeof(lowerPoint));
            location += sizeof(lowerPoint);
        }

        // We store the objects for a leaf and children for internal node
        if (!leaf) {
            // We have to store the bumber of children so that we can load them properly
            long numberOfChildren = childIndices.size();
            memcpy(buffer + location, &numberOfChildren, sizeof(numberOfChildren));
            location += sizeof(numberOfChildren);

            // Now we get the childIndices from the buffer
            for (long i = 0, childIndex = 0; i < numberOfChildren; ++i) {
                memcpy(buffer + location, &childIndex, sizeof(childIndex));
                location += sizeof(childIndex);
            }
        }

        // Now we copy the buffer to disk
        ofstream nodeFile(getFileName(), ios::binary | ios::out);
        nodeFile.write(buffer, PAGESIZE);
        nodeFile.close();
    }

    void Node::readNodeFromDisk() {
        // Create a char buffer to read contents from disk
        char buffer[PAGESIZE];
        long location = 0;

        // Open the binary file ane read into memory
        ifstream nodeFile(getFileName(), ios::binary | ios::in);
        nodeFile.read(buffer, PAGESIZE);
        nodeFile.close();

        // Retrieve the contents
        memcpy((char *) &leaf, buffer + location, sizeof(leaf));
        location += sizeof(leaf);

        memcpy((char *) &fileIndex, buffer + location, sizeof(fileIndex));
        location += sizeof(fileIndex);

        memcpy((char *) &parentIndex, buffer + location, sizeof(parentIndex));
        location += sizeof(parentIndex);

        memcpy((char *) &sizeOfSubtree, buffer + location, sizeof(sizeOfSubtree));
        location += sizeof(sizeOfSubtree);

        // Retrieve upperPoints
        upperPoints.clear();
        for (long i = 0, upperPoint = 0; i < DIMENSION; ++i) {
            memcpy((char *) &upperPoint, buffer + location, sizeof(upperPoint));
            upperPoints.push_back(upperPoint);
            location += sizeof(upperPoint);
        }

        // Retrieve lowerPoints
        lowerPoints.clear();
        for (long i = 0, lowerPoint = 0; i < DIMENSION; ++i) {
            memcpy((char *) &lowerPoint, buffer + location, sizeof(lowerPoint));
            lowerPoints.push_back(lowerPoint);
            location += sizeof(lowerPoint);
        }

        // We load the objects for a leaf and children for internal node
        if (!leaf) {
            // We need to get the number of children in this case
            long numberOfChildren = 0;
            memcpy((char *) &numberOfChildren, buffer + location, sizeof(numberOfChildren));
            location += sizeof(numberOfChildren);

            // Now we get the childIndices from the buffer
            childIndices.clear();
            for (long i = 0, childIndex = 0; i < numberOfChildren; ++i) {
                memcpy((char *) &childIndex, buffer + location, sizeof(childIndex));
                childIndices.push_back(childIndex);
                location += sizeof(childIndex);
            }
        }
    }

    // Store the current session to disk
    void storeSession() {
        // Create a character buffer which will be written to disk
        char buffer[PAGESIZE];
        long location = 0;

        // Store RRoot's fileIndex
        long fileIndex = RRoot->getFileIndex();
        memcpy(buffer + location, &fileIndex, sizeof(fileIndex));
        location += sizeof(fileIndex);

        // Store the global fileCount
        long fileCount = Node::getFileCount();
        memcpy(buffer + location, &fileCount, sizeof(fileCount));
        location += sizeof(fileCount);

        // Create a binary file and write to memory
        ofstream sessionFile(SESSION_FILE, ios::binary | ios::out);
        sessionFile.write(buffer, PAGESIZE);
        sessionFile.close();
    }

    void loadSession() {
        // Create a character buffer which will be written to disk
        long location = 0;
        char buffer[PAGESIZE];

        // Open the binary file ane read into memory
        ifstream sessionFile(SESSION_FILE, ios::binary | ios::in);
        sessionFile.read(buffer, PAGESIZE);
        sessionFile.close();

        // Retrieve the fileIndex of RRoot
        long fileIndex = 0;
        memcpy((char *) &fileIndex, buffer + location, sizeof(fileIndex));
        location += sizeof(fileIndex);

        // Retreive the global fileCount
        long fileCount = 0;
        memcpy((char *) &fileCount, buffer + location, sizeof(fileCount));
        location += sizeof(fileCount);

        // Store the session variables
        Node::setFileCount(fileCount);

        // Delete the current root and load it from disk
        delete RRoot;
        RRoot = new Node(fileIndex);
        RRoot->readNodeFromDisk();
    }
};

using namespace RTree;

int main() {
    // Initialize the BPlusTree module
    Node::initialize();

    // Create a new tree
    RRoot = new Node();

    // Load session or build a new tree
    // ifstream sessionFile(SESSION_FILE);
    // if (sessionFile.good()) {
        // loadSession();
    // } else {
        // buildTree();
    // }

    // Process queries
    // processQuery();

    cout << RRoot->getFileName();

    // Store the session
    // storeSession();

    return 0;
}
