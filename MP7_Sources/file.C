/*
     File        : file.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

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
#include "file_system.H"
#include "simple_disk.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/
FileSystem* File::file_system;
 
File::File(inode * file_inode, FileSystem * _file_system) {
    /* We will need some arguments for the constructor, maybe pointer to disk
     block with file management and allocation data. */
//    Console::puts("In file constructor::");Console::puti(file_inode->fileId);Console::puts("\n");
    //file_start = file_inode->dataBlock[0]; 
    myInode.fileId = file_inode->fileId; 
    myInode.fileSize = file_inode->fileSize; 
    myInode.numBlocks = file_inode->numBlocks; 
    for(int i = 0; i < myInode.numBlocks ; i++){
      myInode.dataBlock[i] = file_inode->dataBlock[i];
    }
    fileId = file_inode->fileId;
    file_system = _file_system;
    curr_block_num = 0;
    curr_offset = 0;
    last_offset = 0;
    //assert(false);
}

/*--------------------------------------------------------------------------*/
/* FILE FUNCTIONS */
/*--------------------------------------------------------------------------*/

int File::Read(unsigned int _n, char * _buf) {
    Console::puts("reading from file\n");
    unsigned char readBuf[512] = {};
    file_system->disk->read(myInode.dataBlock[curr_block_num], readBuf);
    int i = 0;
    for(; i < _n; i++){
      _buf[i] = readBuf[i + curr_offset];
  //     Console::puts("buff::");Console::puti(_buf[i]);Console::puts("\t");
    }
    return i;
    Console::puts("\n");
    //assert(false);
}


void File::Write(unsigned int _n, const char * _buf) {
    Console::puts("writing to file\n");
    if(curr_offset + _n >= 512) curr_block_num++;
    if(curr_block_num == myInode.numBlocks){
      int newBlock = file_system->addInodeBlock(fileId);
      myInode.dataBlock[myInode.numBlocks] = newBlock;
      myInode.numBlocks++;
    }
    unsigned char readBuf[512] = {}; 
    file_system->disk->read(myInode.dataBlock[curr_block_num], readBuf);
    for(int i= 0; i < _n; i++){
      readBuf[i + curr_offset] = _buf[i];
    }
    file_system->disk->write(myInode.dataBlock[curr_block_num], readBuf);
    //Update readbuf with _buf and WB
    curr_offset = (curr_offset + _n) % 512; 
    last_offset = (curr_block_num == myInode.numBlocks - 1 && curr_offset > last_offset)? curr_offset:last_offset;
    myInode.fileSize = myInode.fileSize + _n;
   // assert(false);
}

void File::Reset() {
    Console::puts("reset current position in file\n");
    curr_offset = 0; 
    curr_block_num = 0;
    //assert(false);
    
}

void File::Rewrite() {
    Console::puts("erase content of file\n");
    assert(file_system->FreeInodeBlocks(fileId));
    myInode.numBlocks = 0; 
    curr_block_num = 0; 
    curr_offset = 0; 
    last_offset = 0; 
    myInode.fileSize = 0; 
 //   assert(false);
}


bool File::EoF() {
    Console::puts("testing end-of-file condition\n");
    return (curr_offset == last_offset && curr_block_num == myInode.numBlocks - 1);
    assert(false);
}
