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
            vector<double> upperCoordinates = vector<double>(DIMENSION, numeric_limits<double>::min());
            vector<double> lowerCoordinates = vector<double>(DIMENSION, numeric_limits<double>::max());

        public:
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

            // Set the size of the subtree
            void setSizeOfSubtree(long _sizeOfSubtree) { sizeOfSubtree = _sizeOfSubtree; }

            // Set the parentIndex
            void setParentIndex(long _parentIndex) { parentIndex = _parentIndex; }

            // Get the volume of MBR
            double getVolume() const;

            // Get the volume of two passed points
            double getVolume (vector<double> upperPoint, vector<double> lowerPoint) const;

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
            long getPointPosition(vector<double> point) const;

            // Update the MBR of a node
            void updateMBR(vector<double> point);
            void updateMBR(Node *nodeToInsert);

            // Resize the tree by using childIndices
            void resizeNode();

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
        upperBound = 4;
        lowerBound = 2;
    }

    double Node::getVolume() const {
        double volume = 1;
        for (long i = 0; i < DIMENSION; ++i) {
            volume *= (upperCoordinates[i] - lowerCoordinates[i]);
        }
        return volume;
    }

    double Node::getVolume(vector<double> upperPoint, vector<double> lowerPoint) const {
        double volume = 1;
        for (long i = 0; i < DIMENSION; ++i) {
            volume *= abs(upperPoint[i] - lowerPoint[i]);
        }
        return volume;
    }

    double Node::getDistanceOfPoint(vector<double> point) const {
        double distance = 0;
        double component = 0;

        for (long i = 0; i < DIMENSION; ++i) {
            // For each component we check where it lies for every projection
            if (point[i] >= lowerCoordinates[i] && point[i] <= upperCoordinates[i]) {
                component = 0;
            } else if (point[i] < lowerCoordinates[i]) {
                component = lowerCoordinates[i] - point[i];
            } else {
                component = point[i] - upperCoordinates[i];
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
        for (auto upperCoordinate : upperCoordinates) {
            memcpy(buffer + location, &upperCoordinate, sizeof(upperCoordinate));
            location += sizeof(upperCoordinate);
        }

        for (auto lowerCoordinate : lowerCoordinates) {
            memcpy(buffer + location, &lowerCoordinate, sizeof(lowerCoordinate));
            location += sizeof(lowerCoordinate);
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

        // Retrieve upperCoordinates
        upperCoordinates.clear();
        double upperCoordinate = 0;
        for (long i = 0; i < DIMENSION; ++i) {
            memcpy((char *) &upperCoordinate, buffer + location, sizeof(upperCoordinate));
            upperCoordinates.push_back(upperCoordinate);
            location += sizeof(upperCoordinate);
        }

        // Retrieve lowerCoordinates
        lowerCoordinates.clear();
        double lowerCoordinate = 0;
        for (long i = 0; i < DIMENSION; ++i) {
            memcpy((char *) &lowerCoordinate, buffer + location, sizeof(lowerCoordinate));
            lowerCoordinates.push_back(lowerCoordinate);
            location += sizeof(lowerCoordinate);
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

        cout << "UpperCoordinates: ";
        // Add the bounds of the MBR to the tree
        for (auto upperCoordinate : upperCoordinates) {
            cout << upperCoordinate << " ";
        }
        cout << endl;

        cout << "LowerCoordinates: ";
        for (auto lowerCoordinate : lowerCoordinates) {
            cout << lowerCoordinate << " ";
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
                copy(upperCoordinates.begin(), upperCoordinates.end(), ostream_iterator<double>(cout, " "));
                cout << "),( ";
                copy(lowerCoordinates.begin(), lowerCoordinates.end(), ostream_iterator<double>(cout, " "));
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
           lowerCoordinates[i] = min(lowerCoordinates[i], point[i]);

           // upperPoint is max of existing and point
           upperCoordinates[i] = max(upperCoordinates[i], point[i]);
       }

       // We have changed the node so we store changes
       storeNodeToDisk();
   }

   void Node::updateMBR(Node *nodeToInsert) {
       // Increment the sizeOfSubtree
       sizeOfSubtree += nodeToInsert->sizeOfSubtree;

       for (long i = 0; i < DIMENSION; ++i) {
           // lowerPoint is the min of existing and point
           lowerCoordinates[i] = min(lowerCoordinates[i], nodeToInsert->lowerCoordinates[i]);

           // upperPoint is max of existing and point
           upperCoordinates[i] = max(upperCoordinates[i], nodeToInsert->upperCoordinates[i]);
       }

       // We have changed the node so we store changes
       storeNodeToDisk();
   }

   void Node::resizeNode() {
       // TODO: Update the sizeof Subtree
       // sizeOfSubtree

       // update the MBR
       for (long i = 0; i < (long) childIndices.size(); ++i) {
           for (long j = 0; j < DIMENSION; ++j) {
               // lowerPoint is the min of existing and point
               lowerCoordinates[j] = min(lowerCoordinates[j], childLowerPoints[i][j]);

               // upperPoint is max of existing and point
               upperCoordinates[j] = max(upperCoordinates[j], childUpperPoints[i][j]);
           }
       }
   }

   long Node::getPointPosition(vector<double> point) const {
   }

   void Node::insertObject(DBObject object) {
       vector<double> objectPoint = object.getPoint();

       // Update the in-memory node
       childIndices.push_back(object.getFileIndex());
       childLowerPoints.push_back(objectPoint);
       childUpperPoints.push_back(objectPoint);

       // udpate the MBR
       updateMBR(objectPoint);
   }

   void Node::splitNode() {
       // QUADRATIC SPLIT

       // Find the first two seeds using volume wasted
       long size = childIndices.size();
       long firstSeed = 0;
       long secondSeed = 0;

       double maxWaste = numeric_limits<double>::min();
       double waste = 0;

       vector<double> maxCoordinates;
       vector<double> minCoordinates;

       // Find the seeds by computing max wastage
       for (long i = 0; i < size; ++i) {
           for (long j = i; j < size; ++j) {
               // Get the covering rectangle of the points in consideration
               maxCoordinates.clear();
               minCoordinates.clear();
               for (long k = 0; k < DIMENSION; ++k) {
                   maxCoordinates.push_back(max(childUpperPoints[i][k], childUpperPoints[j][k]));
                   minCoordinates.push_back(min(childLowerPoints[i][k], childLowerPoints[j][k]));
               }

               // Compute max wastage for the points in consideration
               waste = getVolume(maxCoordinates, minCoordinates)
                   - getVolume(childUpperPoints[i], childLowerPoints[i])
                   - getVolume(childUpperPoints[j], childLowerPoints[j]);

               if (waste > maxWaste) {
                   maxWaste = waste;
                   firstSeed = i;
                   secondSeed = j;
               }
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
           long position = root->getPointPosition(object.getPoint());

           // Load the node from disk
           Node *nextRoot = new Node(root->childIndices[position]);

           // Update the node with new MBR
           nextRoot->updateMBR(object.getPoint());

           // Recurse into the node
           insert(nextRoot, object);

           // Store the changes made to the node to disk and clean up
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
    vector<double> p3 = {5,10};
    insert(RRoot, DBObject(p3, "srijan"));
    vector<double> p4 = {1,1};
    insert(RRoot, DBObject(p4, "srijan"));
    vector<double> p5 = {3,4};
    insert(RRoot, DBObject(p5, "srijan"));
    RRoot = new Node(1);

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
