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
#define OBJECT_FILE "objects/objectFile"
#define DEFAULT -1

// If the DEBUG level is VERBOSE then it is automatically normal
#ifdef DEBUG_VERBOSE
#define DEBUG_NORMAL
#endif

// Standard Streams
#include <iostream>
#include <fstream>
#include <stdlib.h>

// STL
#include <string>
#include <cstring>
#include <vector>
#include <queue>
#include <algorithm>
#include <tuple>
#include <iterator>

// Math
#include <math.h>
#include <limits>

// Timing functions
#include <chrono>

namespace RTree {
    // We use the std namespace freely
    using namespace std;

    // Database objects
    class DBObject {
        private:
            static long objectCount;

        public:
            // Get the objectCount
            static long getObjectCount() { return objectCount; }

            // Set the objectCount
            static void setObjectCount(long _objectCount) { objectCount = _objectCount; }

        private:
            // Contents of the Object
            vector<double> point;
            long fileIndex = DEFAULT;
            string dataString = "";

        public:
            DBObject(vector<double> _point, string _dataString) : point(_point), dataString(_dataString) {
                fileIndex = objectCount++;

                // Open a file and write the string to it
                ofstream ofile(OBJECT_FILE, ios::app);
                ofile << dataString << endl;
                ofile.close();
            }

            DBObject(vector<double> _point, long _fileIndex) : point(_point), fileIndex(_fileIndex) {
                // Open a file and read the dataString
                ifstream ifile(OBJECT_FILE);
                for (long i = 0; i < fileIndex + 1; ++i) {
                    getline(ifile, dataString);
                }
                ifile.close();
            }

            // Return the key of the object
            vector<double> getPoint() { return point; }

            // Return the string
            string getDataString() const { return dataString; }

            // Return the fileIndex
            long getFileIndex() const { return fileIndex; }
    };

    // Initial static values
    long DBObject::objectCount = 0;

    // An RTree Node
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

            // Get the lowerBound
            static long getLowerBound() { return lowerBound; }

            // Get the upperBound
            static long getUpperBound() { return upperBound; }

        private:
            // Entries required to completely specify a node
            bool leaf = true;
            long fileIndex = DEFAULT;
            long parentIndex = DEFAULT;
            long sizeOfSubtree = 0;
            vector<double> upperPoints = vector<double>(DIMENSION, numeric_limits<double>::min());
            vector<double> lowerPoints = vector<double>(DIMENSION, numeric_limits<double>::max());
            vector< vector<double> > childLowerPoints;
            vector< vector<double> > childUpperPoints;

        public:
            vector<long> childIndices;

        public:
            // Construct a node object for the first time
            Node () : fileIndex(++fileCount) {}

            //  Read a node from disk
            Node (long _fileIndex) : fileIndex(_fileIndex) { loadNodeFromDisk(); }

            // Get the role of the node
            bool isLeaf() const { return leaf; }

            // Get the index of the file
            long getFileIndex() const { return fileIndex; }

            // Get the name of the file
            string getFileName() const { return NODE_PREFIX + to_string(fileIndex); };

            // Get the childCount
            long getChildCount() const { return childIndices.size(); }

            // Increment the size of subTree
            void incrementSubtree() { ++sizeOfSubtree; }

            // Get the volume of MBR
            double getVolume() const;

            // Distance of Point
            double getDistanceOfPoint(vector<double> point) const;

            // Store the node to disk
            void storeNodeToDisk() const;

            // Read the node from the disk
            void loadNodeFromDisk();

            // Print the node
            void printNode() const;

            // Serialize the tree
            void serialize() const;

            // Get the position of insertion of a point
            long getInsertPosition(vector<double> point) const;

            // Update the MBR of a node
            void updateMBR(vector<double> point);

            // Insert an object to a leaf
            void insertObject(DBObject object);

            // Split a node
            void splitNode();
    };

    // The root of the tree
    Node *RRoot = nullptr;

    // Initial static values
    long Node::fileCount = 0;
    long Node::lowerBound = 0;
    long Node::upperBound = 0;

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

    double Node::getVolume() const {
        double volume = 1;

        for (long i = 0; i < DIMENSION; ++i) {
            volume *= (upperPoints[i] - lowerPoints[i]);
        }

        return volume;
    }

    double Node::getDistanceOfPoint(vector<double> point) const {
        double distance = 0;
        double component = 0;

        for (long i = 0; i < DIMENSION; ++i) {

            // For each component we check where it lies for every projection
            if (point[i] >= lowerPoints[i] && point[i] <= upperPoints[i]) {
                component = 0;
            } else if (point[i] < lowerPoints[i]) {
                component = lowerPoints[i] - point[i];
            } else {
                component = point[i] - upperPoints[i];
            }

            // Add the square of component to distance
            distance += component * component;
        }

        return sqrt(distance);
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

        // We have to store the bumber of children so that we can load them properly
        long numberOfChildren = childIndices.size();
        memcpy(buffer + location, &numberOfChildren, sizeof(numberOfChildren));
        location += sizeof(numberOfChildren);

        // Now we put the childIndices to the buffer
        for (long i = 0; i < numberOfChildren; ++i) {
            memcpy(buffer + location, &childIndices[i], sizeof(childIndices[i]));
            location += sizeof(childIndices[i]);

            // Copy the given child
            for (long j = 0; j < DIMENSION; ++j) {
                memcpy(buffer + location, &childLowerPoints[i][j], sizeof(childLowerPoints[i][j]));
                location += sizeof(childLowerPoints[i][j]);

                memcpy(buffer + location, &childUpperPoints[i][j], sizeof(childUpperPoints[i][j]));
                location += sizeof(childUpperPoints[i][j]);
            }
        }

        // Now we copy the buffer to disk
        ofstream nodeFile(getFileName(), ios::binary | ios::out);
        nodeFile.write(buffer, PAGESIZE);
        nodeFile.close();
    }

    void Node::loadNodeFromDisk() {
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
        double upperPoint = 0;
        for (long i = 0; i < DIMENSION; ++i) {
            memcpy((char *) &upperPoint, buffer + location, sizeof(upperPoint));
            upperPoints.push_back(upperPoint);
            location += sizeof(upperPoint);
        }

        // Retrieve lowerPoints
        lowerPoints.clear();
        double lowerPoint = 0;
        for (long i = 0; i < DIMENSION; ++i) {
            memcpy((char *) &lowerPoint, buffer + location, sizeof(lowerPoint));
            lowerPoints.push_back(lowerPoint);
            location += sizeof(lowerPoint);
        }

        // We need to get the number of children in this case
        long numberOfChildren = 0;
        memcpy((char *) &numberOfChildren, buffer + location, sizeof(numberOfChildren));
        location += sizeof(numberOfChildren);

        // Now we get the childIndices from the buffer
        childIndices.clear();
        childLowerPoints.clear();
        childUpperPoints.clear();
        for (long i = 0, childIndex = 0; i < numberOfChildren; ++i) {
            memcpy((char *) &childIndex, buffer + location, sizeof(childIndex));
            childIndices.push_back(childIndex);
            location += sizeof(childIndex);

            // Load the child
            vector<double> childLowerPoint;
            vector<double> childUpperPoint;
            double lowerPoint, upperPoint;
            for (long j = 0; j < DIMENSION; ++j) {
                memcpy((char *) &lowerPoint, buffer + location, sizeof(lowerPoint));
                childLowerPoint.push_back(lowerPoint);
                location += sizeof(lowerPoint);

                memcpy((char *) &upperPoint, buffer + location, sizeof(upperPoint));
                childUpperPoint.push_back(upperPoint);
                location += sizeof(upperPoint);
            }
            childLowerPoints.push_back(childLowerPoint);
            childUpperPoints.push_back(childUpperPoint);
        }
    }

#ifdef DEBUG_NORMAL
    void Node::printNode() const {
        cout << "Leaf: " << leaf << endl;
        cout << "FileIndex: " << fileIndex << endl;
        cout << "Parent: " << parentIndex << endl;
        cout << "SizeOfSubtree: " << sizeOfSubtree << endl;

        cout << "UpperPoints: ";
        // Add the bounds of the MBR to the tree
        for (auto upperPoint : upperPoints) {
            cout << upperPoint << " ";
        }
        cout << endl;

        cout << "LowerPoints: ";
        for (auto lowerPoint : lowerPoints) {
            cout << lowerPoint << " ";
        }
        cout << endl;

        // Now we put the childIndices to the buffer
        long numberOfChildren = childIndices.size();
        for (long i = 0; i < numberOfChildren; ++i) {
            cout << "Child " << i << ": " << endl;
            cout << "\t Index: " << childIndices[i] << endl;

            // Copy the given child
            cout << "\t LowerPoints: ";
            for (long j = 0; j < DIMENSION; ++j) {
                cout << childLowerPoints[i][j] << " ";
            }
            cout << endl;

            cout << "\t UpperPoints: ";
            for (long j = 0; j < DIMENSION; ++j) {
                cout << childUpperPoints[i][j] << " ";
            }
            cout << endl;
        }
    }
#endif

#ifdef DEBUG_NORMAL
   void Node::serialize() const {
        // Return if node is empty
        if (childIndices.size() == 0) {
            return;
        }

        // Prettify
        cout << endl << endl;

        // To store the previous Level
        queue< pair<long, char> > previousLevel;
        previousLevel.push(make_pair(fileIndex, 'N'));

        // To store the leaves
        queue< pair< vector<double>, char> > leaves;

        long currentIndex;
        Node *iterator;
        char type;
        while (!previousLevel.empty()) {
            queue< pair<long, char> > nextLevel;

            while (!previousLevel.empty()) {
                // Get the front and pop
                currentIndex = previousLevel.front().first;
                iterator = new Node(currentIndex);
                type = previousLevel.front().second;
                previousLevel.pop();

                // If it a seperator, print and move ahead
                if (type == '|') {
                    cout << "|| ";
                    continue;
                }

                // Print the MBR
                cout << "[( ";
                copy(upperPoints.begin(), upperPoints.end(), ostream_iterator<double>(cout, " "));
                cout << "),( ";
                copy(lowerPoints.begin(), lowerPoints.end(), ostream_iterator<double>(cout, " "));
                cout << ")]";

                if (!leaf) {
                    // Enqueue all the children
                    for (auto childIndex : iterator->childIndices) {
                        nextLevel.push(make_pair(childIndex, 'N'));

                        // Insert a marker to indicate end of child
                        nextLevel.push(make_pair(DEFAULT, '|'));
                    }
                } else {
                    // Add all child points to the leaf
                    for (auto childPoint : childLowerPoints) {
                        leaves.push(make_pair(childPoint, 'L'));
                    }

                    // marker for end of leaf
                    leaves.push(make_pair(vector<double>(), '|'));
                }

                // Delete allocated memory
                delete iterator;
            }

            // Seperate different levels
            cout << endl << endl;
            previousLevel = nextLevel;
        }

        // Print all the leaves
        while (!leaves.empty()) {
            // Get the front and pop
            vector<double> point = leaves.front().first;
            type = leaves.front().second;
            leaves.pop();

            // If it a seperator, print and move ahead
            if (type == '|') {
                cout << "|| ";
                continue;
            }

            // Print the MBR
            cout << "( ";
            copy(point.begin(), point.end(), ostream_iterator<double>(cout, " "));
            cout << ") ";
        }

        // Prettify
        cout << endl << endl;
   }
#endif

   void Node::updateMBR(vector<double> point) {
       // Increment the sizeOfSubtree
       incrementSubtree();

       for (long i = 0; i < DIMENSION; ++i) {
           // lowerPoint is the min of existing and point
           lowerPoints[i] = min(lowerPoints[i], point[i]);

           // upperPoint is max of existing and point
           upperPoints[i] = max(upperPoints[i], point[i]);
       }
   }

   long Node::getInsertPosition(vector<double> point) const {
   }

   void Node::insertObject(DBObject object) {
       vector<double> objectPoint = object.getPoint();

       // Update the in-memory node
       childIndices.push_back(object.getFileIndex());
       childLowerPoints.push_back(objectPoint);
       childUpperPoints.push_back(objectPoint);

       // udpate the MBR
       updateMBR(objectPoint);

       // Persist the changes to disk
       storeNodeToDisk();
   }

   void Node::splitNode() {
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

       // Store the global objectCount
       long objectCount = DBObject::getObjectCount();
       memcpy(buffer + location, &objectCount, sizeof(objectCount));
       location += sizeof(objectCount);

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

       // Retreive the global objectCount
       long objectCount = 0;
       memcpy((char *) &objectCount, buffer + location, sizeof(objectCount));
       location += sizeof(objectCount);

       // Store the session variables
       Node::setFileCount(fileCount);
       DBObject::setObjectCount(objectCount);

       // Delete the current root and load it from disk
       delete RRoot;
       RRoot = new Node(fileIndex);
       RRoot->loadNodeFromDisk();
   }

   // Insert a node into the tree
   void insert(Node *root, DBObject object) {
       // If the node is a leaf, then we insert
       if (root->isLeaf()) {
           // Insert the object
           root->insertObject(object);

           // Check for overflow
           if (root->getChildCount() > Node::getUpperBound()) {
               root->splitNode();
           }

#ifdef DEBUG_VERBOSE
            // Serialize
            RRoot->serialize();
#endif
       } else {
           // We traverse the tree
           long position = root->getInsertPosition(object.getPoint());

           // Load the node from disk
           Node *nextRoot = new Node(root->childIndices[position]);

           // Recurse into the node
           insert(nextRoot, object);

           // Clean up
           delete nextRoot;
       }
   }
};

using namespace RTree;

int main() {
    // Initialize the BPlusTree module
    Node::initialize();

    // Create a new tree
    RRoot = new Node();

    vector<double> p1 = {1,2};
    insert(RRoot, DBObject(p1, "srijan"));
    vector<double> p2 = {3,1};
    insert(RRoot, DBObject(p2, "srijan"));
    insert(RRoot, DBObject(p2, "srijan"));
    RRoot = new Node(1);
    // RRoot->printNode();

    // Load session or build a new tree
    // ifstream sessionFile(SESSION_FILE);
    // if (sessionFile.good()) {
    // loadSession();
    // } else {
    // buildTree();
    // }

    // Process queries
    // processQuery();

    // Store the session
    // storeSession();

    return 0;
}
