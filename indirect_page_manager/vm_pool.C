/*
 File: vm_pool.C

 Author:
 Date  : 2024/09/20

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "vm_pool.H"
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

VMPool* PageTable::vm_pool_head = nullptr;

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   V M P o o l */
/*--------------------------------------------------------------------------*/

VMPool::VMPool(unsigned long _base_address,
               unsigned long _size,
               ContFramePool *_frame_pool,
               PageTable *_page_table)
{

    base_addr = _base_address;
    size = _size;
    frame_pool = _frame_pool;
    page_table = _page_table;

    // Use the first page to store the arrays
    max_reg = Machine::PAGE_SIZE / sizeof(Region);
    alloc_reg = (Region *)base_addr;
    free_reg = (Region *)(alloc_reg + max_reg); // Start of free_reg is at the end of alloc_reg & max_reg is the size of the first region

    base_addr += Machine::PAGE_SIZE * 2;// Skip the first two pages
    size -= 2;

    n_alloc_reg = 0;
    n_free_reg = 1;

    page_table->register_pool(this); // Register the pool with the page table before accessing it

    // Initialize with one large free region
    free_reg[0].base_page = base_addr / Machine::PAGE_SIZE;
    free_reg[0].length = size / Machine::PAGE_SIZE;


    Console::puts("Constructed VMPool object.\n");
}

unsigned long VMPool::allocate(unsigned long _size)
{
    int n_pages = (_size + Machine::PAGE_SIZE - 1) / Machine::PAGE_SIZE;

    int free_regs_i = -1;
    for (int i = 0; i < n_free_reg; i++)
    {
        if (free_reg[i].length >= _size)
        {
            free_regs_i = i;
            break;
        }
    }

    unsigned long start_page = free_reg[free_regs_i].base_page * Machine::PAGE_SIZE;
    if (n_alloc_reg >= max_reg)
    {
        Console::puts("No more regions can be allocated.\n");
        return 0;
    }

    // Update the allocated region
    alloc_reg[n_alloc_reg].base_page = free_reg[free_regs_i].base_page;
    alloc_reg[n_alloc_reg].length = n_pages;
    n_alloc_reg++;

    // entire region used up
    if (free_reg[free_regs_i].length == n_pages)
    {
        // Shift region back by one
        for (int i = free_regs_i; i < n_free_reg - 1; i++)
        {
            free_reg[i] = free_reg[i + 1];
        }
        n_free_reg--;
    }
    else
    {
        // Only part of the free region used, cut it
        free_reg[free_regs_i].base_page += n_pages;
        free_reg[free_regs_i].length -= n_pages;
    }

    Console::puts("Allocated region of memory.\n");
    return start_page;
}

void VMPool::release(unsigned long _start_address)
{
    unsigned long page = _start_address / Machine::PAGE_SIZE;

    // Find the allocated region corresponding to start address
    int alloc_regs_i = -1;
    for(int i = 0; i < n_alloc_reg; i++)
    {
        if(alloc_reg[i].base_page == page)
        {
            alloc_regs_i = i;
            break;
        }
    }

    if (alloc_regs_i == -1)
    {
        Console::puts("No allocated region found.\n");
        return;
    }

    // Release pages
    for (unsigned long i = 0; i < alloc_reg[alloc_regs_i].length; i++) {
        page_table->free_page(alloc_reg[alloc_regs_i].base_page + i);
    }

    // Add to free regions
    if (n_free_reg < max_reg) {
        free_reg[n_free_reg] = alloc_reg[alloc_regs_i];
        n_free_reg++;
    }

    // Remove the region from the allocated regions array
    for (int i = alloc_regs_i; i < n_alloc_reg - 1; i++)
    {
        // Shift region back by one
        alloc_reg[i] = alloc_reg[i + 1];
    }
    n_alloc_reg--;

    Console::puts("Released region of memory.\n");
}

bool VMPool::is_legitimate(unsigned long _address)
{
    unsigned long page = _address / Machine::PAGE_SIZE;

    for (int i = 0; i < n_alloc_reg; i++)
    {
        if (alloc_reg[i].base_page <= page && page < alloc_reg[i].base_page + alloc_reg[i].length)
        {
            return true;
        }
    }

    return false;
}
