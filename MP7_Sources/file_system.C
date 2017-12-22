/*
     File        : file_system.C

     Author      : Riccardo Bettati
     Modified    : 2017/05/01

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
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

FileSystem::FileSystem() {
    Console::puts("In file system constructor.\n");
    super_block = 0;
    //assert(false);
}

/*--------------------------------------------------------------------------*/
/* FILE SYSTEM FUNCTIONS */
/*--------------------------------------------------------------------------*/

bool FileSystem::Mount(SimpleDisk * _disk) {
    Console::puts("mounting file system form disk\n");
    //0 occupied 1 free
    unsigned char _buf[512] = {};
    _disk->read(0, _buf);
    super_block = 0; 
    fileSys * entry = (fileSys *)_buf;
    //Console::puti(entry->num_free_blocks);Console::puts("\n");
    //Console::puti(entry->bitmap[0]);Console::puts("\n");
    if(entry->bitmap[0] & 0x01) return false;
    disk = _disk;
    return true;
    //assert(false);
}

bool FileSystem::Format(SimpleDisk * _disk, unsigned int _size) {
    Console::puts("formatting disk\n");
    fileSys entry;
    unsigned char bitmap1 = 0xfc; //11111100 
    int i =0;
    while(i < 256){
       entry.bitmap[i] = 0xff;
       i++;
    }
    entry.fileSys_size = _size/512;
    entry.num_free_blocks = entry.fileSys_size - 2;
    entry.num_inodes = 0;
    entry.inode_block = 1;
    entry.bitmap[0] = bitmap1;
    unsigned char * buf = (unsigned char *)&entry;
    _disk->write(0, buf);
    return true;
    //assert(false);
}

File * FileSystem::LookupFile(int _file_id) {
    Console::puts("looking up file\n");
    unsigned char * super_buf = new unsigned char[512];
    disk->read(super_block,super_buf);
    fileSys * entry = (fileSys *)super_buf;
    if(entry->num_inodes == 0) return NULL;
    unsigned char * inode_buf = new unsigned char[512];
    disk->read(1, inode_buf);
    inode * inode_entry = (inode *)inode_buf;
    int i = 0; 
    while(i < entry->num_inodes){
      if(inode_entry->fileId == _file_id) break;
      inode_entry++;
    }
    if(entry->num_inodes == i) return NULL;
    //Check the num_inodes in the fs 
    File * file = new File(inode_entry, this);
    delete super_buf;
    delete inode_buf;
    return file;
   // assert(false);
}

bool FileSystem::CreateFile(int _file_id) {
    Console::puts("creating file\n");
    unsigned char * super_buf = new unsigned char[512];
    disk->read(0,super_buf);
    fileSys * entry = (fileSys *)super_buf;
    unsigned char * inode_buf = new unsigned char[512];
    disk->read(1, inode_buf);
    inode * inode_entry = (inode *)inode_buf;
    inode * newEntry = inode_entry + entry->num_inodes;
    newEntry->fileId = _file_id;
    newEntry->fileSize = 0;
    newEntry->numBlocks = 0;
    int i =0;
    int j =0;
    bool found = false;
    while(i < 256){
       j = 0; 
       char map = 0x01;
       while(j < 8){
         if(entry->bitmap[i] & map){
           entry->bitmap[i] = entry->bitmap[i] ^ map;
           found = true; 
           break;
         }
         map = map << 1;
         j++;
       }
       if(found) break;
       i++;
    }
    if(i == 256) return false; 
//    Console::puts("creating file i =");Console::puti(i);Console::puts("j = ");Console::puti(j); Console::puts("\n");
    newEntry->dataBlock[newEntry->numBlocks] = 8*i + j;
    newEntry->numBlocks++;
    entry->num_inodes++; 
    disk->write(super_block, super_buf);
//    for(int k = 0; k < entry->num_inodes; k++){
//      inode * temp = inode_entry + k;
//      Console::puts("Inode_files:: "); Console::puti(temp->fileId); Console::puts("\n"); 
//    }
    disk->write(entry->inode_block, inode_buf);
    delete super_buf;
    delete inode_buf;
    return true;
    //Check the bitmap and allocate a new inode
    //assert(false);
}

bool FileSystem::FreeInodeBlocks(int _file_id) {
    Console::puts("Freeing file\n");
    unsigned char * super_buf = new unsigned char[512];
    disk->read(super_block,super_buf);
    fileSys * entry = (fileSys *)super_buf;
    if(entry->num_inodes == 0) return false;
    unsigned char * inode_buf = new unsigned char[512];
    disk->read(1, inode_buf);
    inode * inode_entry = (inode *)inode_buf;
    int i = 0; 
    while(i < entry->num_inodes){
      if(inode_entry->fileId == _file_id) break;
      inode_entry++;
    }
    if(entry->num_inodes == i) return false;
    int j = 0;
    while(inode_entry->numBlocks){
      inode_entry->numBlocks--;
      int freeBlock = inode_entry->dataBlock[inode_entry->numBlocks];
      j = freeBlock%8;
      i = freeBlock/8;
      entry->bitmap[i] = entry->bitmap[i] | (0x01 << j);
    }
    disk->write(super_block, super_buf);
    disk->write(entry->inode_block, inode_buf);
    //Check the bitmap and delete the allocated fileBlocks
    delete super_buf;
    delete inode_buf;
    return true;
   // assert(false);
}

int FileSystem::addInodeBlock(int _file_id) {
    Console::puts("Increasing file size\n");
    unsigned char * super_buf = new unsigned char[512];
    disk->read(0,super_buf);
    fileSys * entry = (fileSys *)super_buf;
    unsigned char * inode_buf = new unsigned char[512];
    disk->read(1, inode_buf);
    inode * inode_entry = (inode *)inode_buf;
    int i = 0; 
    while(i < entry->num_inodes){
      if(inode_entry->fileId == _file_id) break;
      inode_entry++;
    }
    if(entry->num_inodes == i) return false;
    i =0;
    int j =0;
    bool found = false;
    while(i < 256){
       j = 0; 
       char map = 0x01;
       while(j < 8){
         if(entry->bitmap[i] & map){
           entry->bitmap[i] = entry->bitmap[i] ^ map;
           found = true; 
           break;
         }
         map = map << 1;
         j++;
       }
       if(found) break;
       i++;
    }
    if(i == 256) return false; 
    inode_entry->dataBlock[inode_entry->numBlocks] = 8*i + j;
    inode_entry->numBlocks++;
    disk->write(super_block, super_buf);
    disk->write(entry->inode_block, inode_buf);
    delete super_buf;
    delete inode_buf;
    return 8*i + j; 
    //assert(false);
}

bool FileSystem::DeleteFile(int _file_id) {
    Console::puts("deleting file\n");
    unsigned char * super_buf = new unsigned char[512];
    disk->read(super_block,super_buf);
    fileSys * entry = (fileSys *)super_buf;
    if(entry->num_inodes == 0) return NULL;
    unsigned char * inode_buf = new unsigned char[512];
    disk->read(1, inode_buf);
    inode * inode_entry = (inode *)inode_buf;
    int i = 0; 
    int j = 0; 
    while(i < entry->num_inodes){
      if(inode_entry->fileId == _file_id) break;
      inode_entry++;
    } 
    if(entry->num_inodes == i) return false;
    while(inode_entry->numBlocks){
      inode_entry->numBlocks--;
      int freeBlock = inode_entry->dataBlock[inode_entry->numBlocks];
      j = freeBlock%8;
      i = freeBlock/8;
      entry->bitmap[i] = entry->bitmap[i] | (0x01 << j);
    }
    entry->num_inodes--;
    disk->write(super_block, super_buf);
    delete super_buf;
    delete inode_buf;
    return true;
    //Check the bitmap and delete the allocated inode
    //assert(false);
}
