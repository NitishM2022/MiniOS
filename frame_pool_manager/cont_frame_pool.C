/*
 File: ContFramePool.C
 
 Author: Nitish Malluru
 Date  : 9/15/24
 
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
ContFramePool* ContFramePool::head_frame_pool = nullptr;

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

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    // doubly linked list of frame pools
    if (head_frame_pool != nullptr) {
        head_frame_pool->prev_frame_pool = this;
        this->next_frame_pool = head_frame_pool;
    } else {
        this->next_frame_pool = nullptr;
    }

    head_frame_pool = this;
    this->prev_frame_pool = nullptr;

    base_frame_no = _base_frame_no;
    nframes = _n_frames;

    unsigned long n_info_frames = needed_info_frames(_n_frames);

    if (_info_frame_no == 0){
        info_frame_no = _base_frame_no;
        nframes -= n_info_frames;
    } else {
        info_frame_no = _info_frame_no;
    }

    unsigned long bmap_size = (nframes * 2 + 7) / 8;
    bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    for(unsigned long i = 0; i < bmap_size; i++){
        bitmap[i] = 0;
    }

    // set info frames as used
    if (_info_frame_no == 0){
        set_state(this, 0, FrameState::HOS);
        for (unsigned long i = 1; i < n_info_frames; i++) {
            set_state(this, i, FrameState::Used);
        }
    }
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames) {
    unsigned long bmap_size = (nframes * 2 + 7) / 8;
    unsigned long start = 0;

    while (start < nframes) {
        unsigned long curr;
        for (curr = start; curr < start + _n_frames && curr < nframes; curr++) {
            if (get_state(this, curr) != FrameState::Free) {
                break;
            }
        }

        if (curr == start + _n_frames) {
            set_state(this, start, FrameState::HOS);
            for (unsigned long i = start + 1; i < start + _n_frames; i++) {
                set_state(this, i, FrameState::Used);
            }
            return start + base_frame_no;
        }

        start = curr + 1;
    }

    return 0;
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    if (_base_frame_no < base_frame_no || _base_frame_no + _n_frames > base_frame_no + nframes) {
        return;
    }

    unsigned long start = _base_frame_no - base_frame_no;

    set_state(this, start, FrameState::HOS);    
    
    for (unsigned long i = start + 1; i < start + _n_frames; i++) {
        set_state(this, i, FrameState::Used);
    }
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    for (ContFramePool * curr = head_frame_pool; curr != nullptr; curr = curr->next_frame_pool) {
        unsigned long start = _first_frame_no - curr->base_frame_no;
        if (start < 0 || start >= curr->nframes) {
            continue;
        }
    
        if (get_state(curr, start) != FrameState::HOS) {
            continue;
        }
        
        set_state(curr, start, FrameState::Free);
        start++;
        
        while (start < curr->nframes) {
            FrameState state = get_state(curr, start);
            if (state == FrameState::Free || state == FrameState::HOS) {
                break;
            }
            set_state(curr, start, FrameState::Free);
            start++;
        }
        return;
    }
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    return (_n_frames * 2 + FRAME_SIZE - 1) / FRAME_SIZE;
}
