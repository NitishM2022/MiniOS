/*
     File        : nonblocking_disk.c

     Author      :
     Modified    :

     Description :

*/

/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

#define _BONUS_1_2_
/* This macro is defined when we want to force the disk to be mutual exclusive.
   Otherwise, the disk doesn't guarantee mutual exclusion.
*/

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "assert.H"
#include "utils.H"
#include "console.H"
#include "nonblocking_disk.H"
#include "scheduler.H"

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR */
/*--------------------------------------------------------------------------*/

NonBlockingDisk::NonBlockingDisk(DISK_ID _disk_id, unsigned int _size)
    : SimpleDisk(_disk_id, _size)
{
}

/*--------------------------------------------------------------------------*/
/* SIMPLE_DISK FUNCTIONS */
/*--------------------------------------------------------------------------*/

void NonBlockingDisk::wait_until_ready()
{
  while (!is_ready())
  {
    SYSTEM_SCHEDULER->resume(Thread::CurrentThread());
    SYSTEM_SCHEDULER->yield();
  }
}

void NonBlockingDisk::read(unsigned long _block_no, unsigned char *_buf)
{
#ifdef _BONUS_1_2_

  Thread *current_thread = Thread::CurrentThread();
  Disk_Req *req = new Disk_Req(_block_no, _buf, true, current_thread);

  // Add to the request queue
  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  if (head == nullptr)
  {
    head = req;
  }
  else
  {
    Disk_Req *tmp = head;
    while (tmp->next != nullptr)
    {
      tmp = tmp->next;
    }
    tmp->next = req;
  }

  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();

  // Wait till disk is free and our turn
  while (busy || head != req)
  {
    SYSTEM_SCHEDULER->resume(current_thread);
    SYSTEM_SCHEDULER->yield();
  }

  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  busy = true;
  SimpleDisk::do_read(_block_no);
  Console::puts("issued operation\n");

  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();

  if (!is_ready())
  {
    SYSTEM_SCHEDULER->yield();
  }

  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++)
  {
    tmpw = Machine::inportw(0x1F0);
    _buf[i * 2] = (unsigned char)tmpw;
    _buf[i * 2 + 1] = (unsigned char)(tmpw >> 8);
  }

  head = head->next;
  delete req;

  busy = false;

  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();

#else

  SimpleDisk::read(_block_no, _buf);

#endif
}

void NonBlockingDisk::write(unsigned long _block_no, unsigned char *_buf)
{
#ifndef BONUS_1_2_

  Thread *current_thread = Thread::CurrentThread();
  Disk_Req *req = new Disk_Req(_block_no, _buf, false, current_thread);

  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  if (head == nullptr)
  {
    head = req;
  }
  else
  {
    Disk_Req *tmp = head;
    while (tmp->next != nullptr)
    {
      tmp = tmp->next;
    }
    tmp->next = req;
  }

  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();

  // Wait till disk is free
  while (busy || head != req)
  {
    SYSTEM_SCHEDULER->resume(current_thread);
    SYSTEM_SCHEDULER->yield();
  }

  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  busy = true;
  SimpleDisk::do_write(_block_no);
  Console::puts("issued operation\n");

  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();

  if (!is_ready())
  {
    SYSTEM_SCHEDULER->yield();
  }

  if (Machine::interrupts_enabled())
    Machine::disable_interrupts();

  int i;
  unsigned short tmpw;
  for (i = 0; i < 256; i++)
  {
    tmpw = _buf[2 * i] | (_buf[2 * i + 1] << 8);
    Machine::outportw(0x1F0, tmpw);
  }

  head = head->next;
  delete req;

  busy = false;

  if (!Machine::interrupts_enabled())
    Machine::enable_interrupts();

#else

  SimpleDisk::write(_block_no, _buf);

#endif
}

bool NonBlockingDisk::disk_ready()
{
  return is_ready();
}