/*
 File: scheduler.C

 Author:
 Date  :

 */

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "scheduler.H"
#include "thread.H"
#include "console.H"
#include "utils.H"
#include "assert.H"
#include "nonblocking_disk.H"

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

extern NonBlockingDisk * SYSTEM_DISK;

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   S c h e d u l e r  */
/*--------------------------------------------------------------------------*/

Scheduler::Scheduler()
{
  head.thread = nullptr;
  head.next = nullptr;

  zombie_head.thread = nullptr;
  Console::puts("Constructed Scheduler.\n");
}

void Scheduler::yield()
{
  Thread *current = Thread::CurrentThread();
  if (current == nullptr)
  {
    return;
  }

  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  if (head.next != nullptr)
  {
    Thread *next = head.next->thread;
    node *temp = head.next;
    head.next = temp->next;

    if (!Machine::interrupts_enabled())
      Machine::enable_interrupts();

    delete temp;
    Thread::dispatch_to(next);
  }
  else
  {
    if (!Machine::interrupts_enabled())
      Machine::enable_interrupts();
  }
}

void Scheduler::resume(Thread *_thread)
{
  if (_thread == nullptr)
  {
    return;
  }

  node *new_node = new node;
  new_node->thread = _thread;
  new_node->next = nullptr;

  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  // Routine Kill Dead Threads 
  while (zombie_head.next != nullptr)
  {
    node *temp = zombie_head.next;
    zombie_head.next = temp->next;

    delete temp->thread;
    delete temp;
  }

  // Add to end of list
  node *current = &head;
  while (current->next != nullptr)
  {
    current = current->next;
  }

  current->next = new_node;


  // Add thread to resume if disk is ready
  if (SYSTEM_DISK->head != nullptr && SYSTEM_DISK->head->resumed == false && SYSTEM_DISK->disk_ready())
  {
    SYSTEM_DISK->head->resumed = true;
    Console::puts("Resuming disk operation Thread no"); Console::puti(SYSTEM_DISK->head->req_thread->ThreadId()); Console::puts("\n");
    resume(SYSTEM_DISK->head->req_thread);
  }

  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}

void Scheduler::add(Thread *_thread)
{
  resume(_thread);
}

void Scheduler::terminate(Thread *_thread)
{
  if (_thread == nullptr)
  {
    return;
  }

  bool is_self = (_thread == Thread::CurrentThread());

  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  node *current = &head;
  while (current->next != nullptr)
  {
    if (current->next->thread == _thread)
    {
      node *temp = current->next;
      current->next = temp->next;

      if (!Machine::interrupts_enabled())
        Machine::enable_interrupts();

      delete temp;
      break;
    }
    current = current->next;
  }

  if (current->next == nullptr && !Machine::interrupts_enabled())
  {
    Machine::enable_interrupts();
  }

  if (is_self)
  {
    yield();
  }
}

void Scheduler::add_zombie(Thread *_thread)
{
  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  node *new_zombie = new node;
  new_zombie->thread = _thread;
  new_zombie->next = zombie_head.next;

  zombie_head.next = new_zombie;

  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();
}
