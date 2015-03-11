/* Structure of file
   -----------------
   fileIndex
   leaf
   parent
   nextLeaf
   previousLeaf
   keySize
   key1
   key2
   ...
   keyn
   child1
   child2
   ...
   child (n+1)
   ------------------
   */

/*  Conventions
 *  1. Caller ensures the Node is loaded into memory.
 *  2. Callee makes in-memory changes, caller saves the changes to disk.
 */

// -- Mode of operation --
#define OUTPUT
// #define TIME

// -- Verbosity Level --
// #define DEBUG_NORMAL
// #define DEBUG_V
// #define DEBUG_INSERT
// #define DEBUG_SPLITNODE
// #define DEBUG_INSERTPOSITION

// If the DEBUG level is VERBOSE then it is automatically normal
#ifdef DEBUG_NORMAL
#define DEBUG_INSERT
#endif

// -- Verbosity Level VV --
#ifdef DEBUG_V
#define DEBUG_INSERTPOSITION
#define DEBUG_SPLITNODE
#define DEBUG_INSERT
#endif

// -- Auto Generated --
#define PAGESIZE 2048
#define DIMENSION 2
