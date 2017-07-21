/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/
fPool * ContFramePool::fPoolHead = NULL;
fPool * ContFramePool::fPoolTail = NULL;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no,
                             unsigned long _n_info_frames)
{
    // Bitmap using 2-bits per frame must fit in a single frame!
    // 11 --> Free
    // 00 --> Allocated and Head of Seq
    // 01 --> Just Allocated
    if(_info_frame_no == 0){
        assert(_n_frames <= FRAME_SIZE*4);
    }else{
        assert(_n_frames <= FRAME_SIZE*_n_info_frames*4);
    }
    
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    n_info_frames = _n_info_frames;
    // If _info_frame_no is zero then we keep management info in the first
    //frame, else we use the provided frame to keep management info
    if(info_frame_no == 0) {
        n_info_frames = 1;
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }
    
    // Number of frames must be "fill" the bitmap!
    assert ((nframes % 4 ) == 0);
    
    // Everything ok. Proceed to mark all bits in the bitmap
    for(int i=0; i*4 < _n_frames; i++) {
        bitmap[i] = 0xFF;
    }
    // Populating the frame pool linked-list with this pool info.
    if(fPoolHead == NULL){
        fPoolHead = &poolNode;
        fPoolHead->next = NULL;
    }
    if(fPoolTail != NULL){
        fPoolTail->next = &poolNode;
    }
    fPoolTail = &poolNode;
    fPoolTail->pool = this;
    fPoolTail->next = NULL; 
    
    // Mark the first frame as being used if it is being used
    if(_info_frame_no == 0) {
        bitmap[0] = 0x3F;
        nFreeFrames--;
    }

    Console::puts("Frame Pool initialized\n");
    // TODO: IMPLEMENTATION NEEEDED!
    // assert(false);
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{   
    assert(_n_frames > 0);
    if(_n_frames > nFreeFrames){
        return 0;
    }
    unsigned int frame_no = base_frame_no;
    unsigned int i = 0;
    unsigned int j = 0;
    unsigned int n_cont_frames=0;
    unsigned char mask = 0xC0;
    unsigned char bit_val;
    while(i < (nframes/4) && n_cont_frames < _n_frames){
        bit_val = bitmap[i];
        j=4;
        while(j!=0 && n_cont_frames < _n_frames){
            if((bit_val & mask) == 0xC0){
                n_cont_frames++;
            }else{
                frame_no = frame_no + n_cont_frames + 1;
                n_cont_frames = 0;
            }
            bit_val = bit_val << 2; 
            j--;
        }
        i++; 
    }
    unsigned int bit_index= (frame_no-base_frame_no)/4;
    mask = 0xc0 >> (((frame_no-base_frame_no)%4)*2);
    // ele_mask = 01010101;
    unsigned char ele_mask = 0x55;      
    if(n_cont_frames == _n_frames){
        bitmap[bit_index] = bitmap[bit_index] & (~mask);
    }else{
        Console::puts("No continuous pages\n");
        return 0;
    }
    mask = mask >> 2;
    i = 1;
    while(i < _n_frames){
        while(mask != 0x00 && i < _n_frames){
            bitmap[bit_index] = bitmap[bit_index] & (~mask);
            bitmap[bit_index] = bitmap[bit_index] | (ele_mask & mask);
            i++;
            mask = mask >> 2;
        }
        bit_index++;
        mask = 0xc0;
    }
    nFreeFrames -= _n_frames;
    return (frame_no);
    // TODO: IMPLEMENTATION NEEEDED!
    // assert(false);
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // Mark all frames in the range as being used.
    unsigned int bit_index= (_base_frame_no-base_frame_no)/4;
    unsigned char mask = 0xc0 >> (((_base_frame_no-base_frame_no)%4)*2);
    unsigned char ele_mask = 0x55;
    bitmap[bit_index] = bitmap[bit_index] & (~mask);
    mask = mask >> 2;
    if(mask == 0x00){
        mask = 0xc0;
        bit_index++;
    }
    unsigned int i = 1;
    while(i < _n_frames){
        while(mask != 0x00 && i < _n_frames){
            bitmap[bit_index] = bitmap[bit_index] & (~mask);
            bitmap[bit_index] = bitmap[bit_index] | (ele_mask & mask);
            i++;
            mask = mask >> 2;
        }
        bit_index++;
        mask = 0xc0;
    }
    nFreeFrames -= _n_frames;
    // TODO: IMPLEMENTATION NEEEDED!
    // assert(false);
}

void ContFramePool::release_frame(unsigned long _first_frame_no){
    unsigned int bit_index= (_first_frame_no-base_frame_no)/4;
    unsigned int shift = (((_first_frame_no-base_frame_no)%4)*2);
    unsigned char mask = 0xc0 >> shift;
    if((bitmap[bit_index] & mask) != 0x00) {
        Console::puts("Error, First frame being released is not a HOS\n");
        assert(false);
    }
    // Marking the HOS as free
    bitmap[bit_index] = bitmap[bit_index] | mask;
    unsigned int frames_freed = 1;
    mask = mask >> 2;
    shift +=2;
    if(mask == 0x00){
        shift = 0;
        mask = 0xc0;
        bit_index++;
    }
    while(((bitmap[bit_index] & mask) << shift)  == 0x40){
        while((((bitmap[bit_index] & mask) << shift) == 0x40) && (mask != 0x00)){
            bitmap[bit_index] = bitmap[bit_index] | mask;
            frames_freed++;
            mask = mask >> 2;
            shift += 2;
        }
        shift = 0;
        mask = 0xc0;
        bit_index++;
    }
    nFreeFrames += frames_freed;
    // TODO: IMPLEMENTATION NEEEDED!
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    bool foundPool = false;
    fPool * pool_p = fPoolHead;
    ContFramePool * pool = (ContFramePool *) pool_p->pool;
    while(pool_p !=NULL && !foundPool){
        if((pool->base_frame_no <= _first_frame_no) && (pool->nframes > _first_frame_no - pool->base_frame_no)){
            foundPool = true;
        }else{
            pool_p = pool_p->next;
            pool = (ContFramePool *) pool_p->pool;
        }
    }
    
    if(foundPool){
        ((ContFramePool *) pool_p->pool)->release_frame(_first_frame_no);
    }else{
        Console::puts("Unable to find the frames memory pool\n ");
        assert(false);
    }
    // TODO: IMPLEMENTATION NEEEDED!
    // assert(false);
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    return (_n_frames/(FRAME_SIZE*4) + (_n_frames % (FRAME_SIZE*4) > 0 ? 1 : 0));
    // TODO: IMPLEMENTATION NEEEDED!
    // assert(false);
}
