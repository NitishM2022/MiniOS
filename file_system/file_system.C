/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2021/11/28

     Description : Implementation of simple File System class.
                   Has support for numerical file identifiers.
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
#include "file_system.H"

/*--------------------------------------------------------------------------*/
/* CLASS Inode */
/*--------------------------------------------------------------------------*/

/* You may need to add a few functions, for example to help read and store
   inodes from and to disk. */

/*--------------------------------------------------------------------------*/
/* CLASS FileSystem */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem()
{
    Console::puts("In file system constructor.\n");
    inodes = new Inode[MAX_INODES];
    free_blocks = new unsigned char[SimpleDisk::BLOCK_SIZE];
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

FileSystem::~FileSystem()
{
    Console::puts("unmounting file system\n");
    /* Make sure that the inode list and the free list are saved. */
    delete[] inodes;
    delete[] free_blocks;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk *_disk)
{
    Console::puts("mounting file system from disk\n");
    /* Here you read the inode list and the free list into memory */
    disk = _disk;
    size = _disk->NaiveSize();

    unsigned char inode_block[SimpleDisk::BLOCK_SIZE];
    disk->read(0, inode_block);
    for (unsigned int i = 0; i < MAX_INODES; i++)
    {
        inodes[i].id = *(long *)&inode_block[i * sizeof(Inode)];
        inodes[i].blockNo = *(int *)&inode_block[i * sizeof(Inode) + sizeof(long)];
        inodes[i].size = *(int *)&inode_block[i * sizeof(Inode) + sizeof(long) + sizeof(int)];
    }

    disk->read(1, free_blocks);
    return true;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

bool FileSystem::Format(SimpleDisk *_disk, unsigned int _size)
{ // static!
    Console::puts("formatting disk\n");
    /* Here you populate the disk with an initialized (probably empty) inode list
       and a free list. Make sure that blocks used for the inodes and for the free list
       are marked as used, otherwise they may get overwritten. */

    Inode* emptyInodes = new Inode[MAX_INODES];
    for (unsigned i = 0; i < MAX_INODES; i++)
    {
        emptyInodes[i].id = -1;
        emptyInodes[i].blockNo = -1;
        emptyInodes[i].size = 0;
    }
    _disk->write(0, reinterpret_cast<unsigned char*>(emptyInodes));
    delete[] emptyInodes;

    unsigned char* emptyFreeBlock = new unsigned char[SimpleDisk::BLOCK_SIZE];
    for (int i = 0; i < SimpleDisk::BLOCK_SIZE; i++)
    {
        emptyFreeBlock[i] = 0;
    }
    emptyFreeBlock[0] = 1;  
    emptyFreeBlock[1] = 1;
    _disk->write(1, reinterpret_cast<unsigned char*>(emptyFreeBlock));
    delete[] emptyFreeBlock;

    return true;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

Inode *FileSystem::LookupFile(int _file_id)
{
    Console::puts("looking up file with id = ");
    Console::puti(_file_id);
    Console::puts("\n");
    /* Here you go through the inode list to find the file. */
    for (int i = 0; i < MAX_INODES; ++i) {
        if (inodes[i].id == _file_id) {
            return &inodes[i];
        }
    }
    return nullptr;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

bool FileSystem::CreateFile(int _file_id)
{
    Console::puts("creating file with id:");
    Console::puti(_file_id);
    Console::puts("\n");
    /* Here you check if the file exists already. If so, throw an error.
       Then get yourself a free inode and initialize all the data needed for the
       new file. After this function there will be a new file on disk. */
    if (LookupFile(_file_id) != nullptr) {
        return false;
    }

    int inode_id = GetFreeInode();
    if (inode_id == -1) {
        return false;
    }

    int free_block = GetFreeBlock();
    if (free_block == -1) {
        return false;
    }

    inodes[inode_id].id = _file_id;
    inodes[inode_id].blockNo = free_block;
    inodes[inode_id].size = 0;

    free_blocks[free_block] = 1;

    disk->write(0, reinterpret_cast<unsigned char*>(inodes));
    disk->write(1, reinterpret_cast<unsigned char*>(free_blocks));

    return true;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}

bool FileSystem::DeleteFile(int _file_id)
{
    Console::puts("deleting file with id:");
    Console::puti(_file_id);
    Console::puts("\n");
    /* First, check if the file exists. If not, throw an error.
       Then free all blocks that belong to the file and delete/invalidate
       (depending on your implementation of the inode list) the inode. */
    Inode* inode = LookupFile(_file_id);
    if (inode == nullptr) {
        return false;
    }

    free_blocks[inode->blockNo] = 0;

    inode->id = -1;
    inode->blockNo = -1;
    inode->size = 0;

    disk->write(0, reinterpret_cast<unsigned char*>(inodes));
    disk->write(1, reinterpret_cast<unsigned char*>(free_blocks));

    return true;
    // Console::puts("FUNCTION NOT IMPLEMENTED\n");
    // assert(false);
}
