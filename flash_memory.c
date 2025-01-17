/*
 * Copyright 2022 Jarrod A. Smith (MakerMatrix)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.* Testing/examples for reading/writing flash on the RP2040.
 * 
 * Resources I drew from:
 * https://raspberrypi.github.io/pico-sdk-doxygen/group__hardware__flash.html
 * https://kevinboone.me/picoflash.html
 * https://www.makermatrix.com/blog/read-and-write-data-with-the-pi-pico-onboard-flash/
 * 
 * You must disable interrupts while programming flash.  This requires you to
 * include hardware/sync.h and use save_and_disable_interrupts(), then restore_interrupts()
 * 
 * 
 * FLASH_PAGE_SIZE       // 256 bytes (minimum you can write)
 * FLASH_SECTOR_SIZE     // 4096 bytes (minimum you can erase)
 * FLASH_BLOCK_SIZE      // 64k bytes
 * PICO_FLASH_SIZE_BYTES - the total size of the RP2040 flash, in bytes 2MB
 * 
 * functions:
 * flash_range_erase()   // Must be aligned to a 4096-byte flash sector
 * flash_range_program() // Must be aligned to a 256-byte flash page
 * 
 * 
 * and from addresses.h:
 * XIP_BASE - the ARM address of the end of the onboard 256KB RAM.
 * On RP2040 this is 0x10000000 and the first byte of flash is the next address.  
 * 
 * Note that the flash_range_erase() and flash_range_program() functions from flash.h
 * do not use ARM address space, only offsets into flash (i.e. don't count the
 * first 0x10000000 bytes of address space representing the RAM.
 * 
 * But to read, point a pointer directly to a memory-mapped address somewhere in the
 * flash (so, use XIP_BASE to get past RAM) and read directly from it.
*/
#include <stdio.h>
#include <string.h>

#include "hardware/sync.h"
#include "hardware/flash.h"
#include "debug.h"

// Set the target offest to the last sector of flash
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

char page_buffer[FLASH_PAGE_SIZE];  // page to store in flash
int addr, page;
int16_t *p;

void flash_erase() {
   DEBUG_printf("Full sector, erasing...\n");
    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    restore_interrupts (ints);
}

bool flash_write(char * buf, uint len) {
  DEBUG_printf("Data Size = %d\n" ,len);
  DEBUG_printf("FLASH_PAGE_SIZE = %d\n", FLASH_PAGE_SIZE);
  DEBUG_printf("FLASH_SECTOR_SIZE = %d\n" , FLASH_SECTOR_SIZE);
  DEBUG_printf("FLASH_BLOCK_SIZE = %d\n", FLASH_BLOCK_SIZE);
  DEBUG_printf("PICO_FLASH_SIZE_BYTES = %d\n" , PICO_FLASH_SIZE_BYTES);
  DEBUG_printf("XIP_BASE = 0x%4x \n" ,XIP_BASE);
  
  // 
  // Writes must be done as multipes of PAGE_SIZE (256 bytes)
  // Just allow a single page write for now TBD 
  //
  if (len > FLASH_PAGE_SIZE-2) { //
    DEBUG_printf("Data size: %d exceeds page size: %d \n" ,len, FLASH_PAGE_SIZE);
    return false;
  }

  // Look for an unwritten page by scanning the flash via memory-mapped addresses
  // For that we must skip over the XIP_BASE worth of RAM and point to the start of last sector
  // Addrress = XIP_BASE + FLASH_TARGET_OFFSET;
  // Erased flash has all bits=1
  int first_empty_page = -1;
  for(page = 0; page < FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE; page++){
    addr = XIP_BASE + FLASH_TARGET_OFFSET + (page * FLASH_PAGE_SIZE);
    p = (int16_t *)addr;
    DEBUG_printf("Page %d ", page );
    DEBUG_printf("(at 0x%4x)", addr);
    DEBUG_printf(" value = %d \n",*p);
    // 0xFFFF cast as an int is -1 so this is how we detect an empty page
    if( *p == -1 && first_empty_page < 0){
      first_empty_page = page;
    }
  }
  if (first_empty_page < 0){
    // no empty pages found, so erase the whole sector
    flash_erase();
    first_empty_page = 0;
  }

  memset(page_buffer,0,sizeof(page_buffer)); // 
  memcpy(&page_buffer[2], buf, len);  
  DEBUG_printf("Writing to page %d\n", first_empty_page);
  uint32_t ints = save_and_disable_interrupts();
  flash_range_program(FLASH_TARGET_OFFSET + (first_empty_page*FLASH_PAGE_SIZE), (uint8_t *)page_buffer, FLASH_PAGE_SIZE);
  restore_interrupts (ints);
  return true;
}

char * flash_read() {
  int last_used_page = -1;
  for(page = 0; page < FLASH_SECTOR_SIZE/FLASH_PAGE_SIZE; page++){
    addr = XIP_BASE + FLASH_TARGET_OFFSET + (page * FLASH_PAGE_SIZE);
    p = (int16_t *)addr;
    // 0x0000 cast as an int means page has been written to
    if( *p == 0 ){
      last_used_page = page;
    }
  }
  if (last_used_page < 0) {
    // didnt find any written pages
    return NULL;
  } else {
    addr = XIP_BASE + FLASH_TARGET_OFFSET + (last_used_page * FLASH_PAGE_SIZE);
    // return the address of page bytes for reading
    DEBUG_printf("Flash page %d contains data\n",last_used_page);
    return (char *)addr+2;
  }
}