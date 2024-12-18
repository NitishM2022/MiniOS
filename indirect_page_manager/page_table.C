#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable *PageTable::current_page_table = nullptr;
unsigned int PageTable::paging_enabled = 0;
ContFramePool *PageTable::kernel_mem_pool = nullptr;
ContFramePool *PageTable::process_mem_pool = nullptr;
unsigned long PageTable::shared_size = 0;

void PageTable::init_paging(ContFramePool *_kernel_mem_pool,
                            ContFramePool *_process_mem_pool,
                            const unsigned long _shared_size)
{
    kernel_mem_pool = _kernel_mem_pool;
    process_mem_pool = _process_mem_pool;
    shared_size = _shared_size;
    Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
    page_directory = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);
    unsigned long *page_table = (unsigned long *)(process_mem_pool->get_frames(1) * PAGE_SIZE);

    for (int i = 0; i < ENTRIES_PER_PAGE; i++)
    {
        page_table[i] = (i * PAGE_SIZE) | (0x1 << 0) // Present flag
                        | (0x1 << 1);                // Read/Write flag
    }

    page_directory[0] = ((unsigned long)page_table) | 0x3;
    for (int i = 1; i < ENTRIES_PER_PAGE; i++)
    {
        page_directory[i] = 0 | (0x1 << 1); // Read/Write flag
    }
    page_directory[1023] = ((unsigned long)page_directory) | 0x3; // Self reference

    Console::puts("Constructed Page Table object\n");
}

void PageTable::load()
{
    current_page_table = this;
    write_cr3((unsigned long)page_directory);
    Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
    unsigned long cr0 = read_cr0();
    cr0 |= 0x80000000;
    write_cr0(cr0);

    paging_enabled = 1;
}

void PageTable::handle_fault(REGS *_r)
{
    unsigned long error = _r->err_code;
    unsigned long fault_addr = read_cr2();

    if (!(error & 0x1))
    {
        unsigned long directory_index = fault_addr >> 22;
        unsigned long table_index = (fault_addr >> 12) & 0x3FF;

        // Check if address is in any of the VM Pools
        VMPool *temp = vm_pool_head;
        while (temp->next != nullptr)
        {
            if (temp->is_legitimate(fault_addr))
            {
                break;
            }
            temp = temp->next;
        }
        if (temp == nullptr)
        {
            Console::puts("Invalid address\n");
            return;
        }

        unsigned long *PDE_address = (unsigned long *)(0xFFFFF000); // Recursive mapping to access page directory

        // Check if the page directory entry is present (bit 0)
        if ((PDE_address[directory_index] & 0x1) == 0)
        {
            // Address of page table in memory
            unsigned long *new_page_table = (unsigned long *)((process_mem_pool->get_frames(1)) * PAGE_SIZE);
            
            // Set page directory entry with physical address of page table and set flags
            PDE_address[directory_index] = ((unsigned long)new_page_table & 0xFFFFF000) // Physical address of new page table
                                           | (0x1 << 0)                                 // Present flag
                                           | (0x1 << 1);                                // Read/Write flag
        }


        unsigned long *PTE_address = (unsigned long *)(0xFFC00000 | (directory_index << 12));

        if ((PTE_address[table_index] & 0x1) == 0)
        {

            // Allocate new frame for page
            unsigned long *new_page = (unsigned long *)((process_mem_pool->get_frames(1)) * PAGE_SIZE);

            // Set page table entry with physical address of the frame and set flags
            PTE_address[table_index] = ((unsigned long)new_page & 0xFFFFF000) | (0x1 << 0) // Present flag
                                       | (0x1 << 1);                                       // Read/Write flag
        }
    }
    Console::puts("Page Fault handled\n");
}

void PageTable::register_pool(VMPool *_vm_pool)
{
    if (vm_pool_head == nullptr)
    {
        vm_pool_head = _vm_pool;
    }
    else
    {
        VMPool *temp = vm_pool_head;
        while (temp->next != nullptr)
        {
            temp = temp->next;
        }
        temp->next = _vm_pool;
    }
}

void PageTable::free_page(unsigned long _page_no)
{
    unsigned long directory_index = _page_no >> 22;
    unsigned long table_index = (_page_no >> 12) & 0x3FF;

    // Last 4MB of virtual memory is used for recursive mapping
    unsigned long *page_table = (unsigned long *)(0xFFC00000 | (directory_index << 12));
    unsigned long frame_no = (page_table[table_index] & 0xFFFFF000) / PAGE_SIZE; // Clear flags and divide by PAGE_SIZE to get frame number
    process_mem_pool->release_frames(frame_no);

    page_table[table_index] = 0x1 << 1; // Read/Write flag

    load(); // Flush TLB

    Console::puts("freed page\n");
}
