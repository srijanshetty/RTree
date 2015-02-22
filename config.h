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
 *  2. If a function modifies the Node, it saves it back to disk
 */

// -- Mode of operation --
#define OUTPUT
// #define TIME

// -- Verbosity Level --
// #define DEBUG_NORMAL
// #define DEBUG_VERBOSE

// -- Auto Generated --
#define PAGESIZE 2048
#define DIMENSION 2
