/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File class, with support for
                   sequential read/write operations.
*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "console.H"
#include "file.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR */
/*--------------------------------------------------------------------------*/

File::File(FileSystem *_fs, int _id) {
    Console::puts("Opening file.\n");
    fs = _fs;
    inode = fs->LookupFile(_id);
    curPos = 0;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

File::~File() {
    Console::puts("Closing file ");
    Console::puti(inode->id);
    Console::puts("\n");
    /* Make sure that you write any cached data to disk. */
    /* Also make sure that the inode in the inode list is updated. */    
    if(!inode) {
        return;
    }

    fs->disk->write(inode->blockNo, block_cache);
    curPos = 0;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char *_buf) {
    Console::puts("reading from file\n");
    if(!inode) {
        return 0;
    }

    unsigned int charsToRead = _n;
    if (curPos + _n > inode->size) {
        charsToRead = inode->size - curPos;
    }

    memcpy(_buf, block_cache + curPos, charsToRead);
    curPos += charsToRead;

    return charsToRead;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

int File::Write(unsigned int _n, const char *_buf) {
    Console::puts("writing to file\n");
    if(!inode) {
        return 0;
    }

    unsigned int charsToWrite = _n;
    if (curPos + _n > SimpleDisk::BLOCK_SIZE) {
        charsToWrite = SimpleDisk::BLOCK_SIZE - curPos;
    }


    memcpy(block_cache + curPos, _buf, charsToWrite);

    
    if (curPos + charsToWrite > inode->size) {
        inode->size = curPos + charsToWrite;
    }

    curPos += charsToWrite;
    return charsToWrite;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

void File::Reset() {
    Console::puts("resetting file\n");
    curPos = 0;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

bool File::EoF() {
    Console::puts("checking for EoF\n");
    if(!inode) {
        return false;
    }

    return curPos >= inode->size;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}
