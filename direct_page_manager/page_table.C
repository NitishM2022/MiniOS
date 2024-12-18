#include "assert.H"
#include "exceptions.H"
#include "console.H"
#include "paging_low.H"
#include "page_table.H"

PageTable * PageTable::current_page_table = nullptr;
unsigned int PageTable::paging_enabled = 0;
ContFramePool * PageTable::kernel_mem_pool = nullptr;
ContFramePool * PageTable::process_mem_pool = nullptr;
unsigned long PageTable::shared_size = 0;

void PageTable::init_paging(ContFramePool * _kernel_mem_pool,
                            ContFramePool * _process_mem_pool,
                            const unsigned long _shared_size)
{
   kernel_mem_pool = _kernel_mem_pool;
   process_mem_pool = _process_mem_pool;
   shared_size = _shared_size;
   Console::puts("Initialized Paging System\n");
}

PageTable::PageTable()
{
   page_directory = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);
   unsigned long * page_table = (unsigned long *) (kernel_mem_pool->get_frames(1) * PAGE_SIZE);

   for (int i = 0; i < ENTRIES_PER_PAGE; i++)
   {
      page_table[i] = (i * PAGE_SIZE) 
                     | (0x1 << 0)  // Present flag
                     | (0x1 << 1); // Read/Write flag
   }

   page_directory[0] = ((unsigned long) page_table) | 0x3;
   for (int i = 1; i < ENTRIES_PER_PAGE; i++)
   {
      page_directory[i] = 0 | (0x1 << 1); // Read/Write flag
   }
   Console::puts("Constructed Page Table object\n");
}


void PageTable::load()
{
   current_page_table = this;
   write_cr3((unsigned long) page_directory);
   Console::puts("Loaded page table\n");
}

void PageTable::enable_paging()
{
   unsigned long cr0 = read_cr0();
   cr0 |= 0x80000000;
   write_cr0(cr0);
   
   paging_enabled = 1;
}

void PageTable::handle_fault(REGS * _r)
{
    unsigned long error = _r->err_code;
    unsigned long fault_addr = read_cr2();

    if (!(error & 0x1))
    {
      unsigned long directory_index = fault_addr >> 22; 
      unsigned long table_index = (fault_addr >> 12) & 0x3FF;

      if ((current_page_table->page_directory[directory_index] & 0x1) == 0) 
      {
         // Address of page table in memory
         unsigned long * new_page_table = (unsigned long *) ((kernel_mem_pool->get_frames(1)) * PAGE_SIZE);
         for (int i = 0; i < ENTRIES_PER_PAGE; i++) {
            new_page_table[i] = 0;
         }

         // Set page directory entry with physical address of page table and set flags
         current_page_table->page_directory[directory_index] = ((unsigned long) new_page_table & 0xFFFFF000) // Physical address of new page table
                                                               | (0x1 << 0)  // Present flag
                                                               | (0x1 << 1); // Read/Write flag
        }

        unsigned long * page_table_addr = (unsigned long *) (current_page_table->page_directory[directory_index] & 0xFFFFF000);
        if ((page_table_addr[table_index] & 0x1) == 0) 
        {
            // Allocate new frame for page
            unsigned long * new_page = (unsigned long *) ((process_mem_pool->get_frames(1)) * PAGE_SIZE);

            // Set page table entry with physical address of the frame and set flags
            page_table_addr[table_index] = ((unsigned long) new_page & 0xFFFFF000)
                                          | (0x1 << 0) // Present flag
                                          | (0x1 << 1); // Read/Write flag 
         }
      }
      Console::puts("Page Fault handled\n");
}


