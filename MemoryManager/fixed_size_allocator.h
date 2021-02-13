#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_set>
#include <vector>

#include <windows.h>

#include "definitions.h"

template<size_t block_size, size_t page_size = 4*KB>
class fixed_size_allocator final
{
public:
	static_assert(page_size > block_size, "page should contain at least two blocks");
	static_assert(page_size % block_size == 0, "page should contain an integer number of blocks");

	fixed_size_allocator(std::string debug_name);
	~fixed_size_allocator();

	void* alloc(size_t size);
	void free(void* address);

#ifndef NDEBUG
	void dumpStat() const;
	void dumpBlocks() const;
#endif

private:
	union block
	{
		uint8_t data[block_size];
		block* next_free_block;
	};
	static_assert(block_size > sizeof(block*), "block_size is too small to contain all necessary service information");
	
	block* allocate_new_page();
	block* get_page_for_address(void* address) const;

	block* free_block = nullptr;
	std::vector<block*> allocated_pages;

#ifndef NDEBUG
	std::string name;
	std::unordered_set<block*> allocated_blocks;
#endif
};

template<size_t block_size, size_t page_size>
inline fixed_size_allocator<block_size, page_size>::fixed_size_allocator(std::string debug_name)
{
#ifndef NDEBUG
	name = debug_name;
#endif
	free_block = allocate_new_page();
}

template<size_t block_size, size_t page_size>
inline fixed_size_allocator<block_size, page_size>::~fixed_size_allocator()
{
#ifndef NDEBUG
	// You have a memory leak if this assert is triggered
	if(allocated_blocks.size() > 0)
	{
		std::cerr << name << " allocator detected a memory leak! Following adresses are not free at allocated destruction:"<< std::endl;
		for(block* allocated_block : allocated_blocks)
		{
			std::cerr << "\t" << (void*)allocated_block << std::endl;
		}
	}
	assert(allocated_blocks.size() == 0);
#endif

	for(block* page : allocated_pages)
	{
		VirtualFree(page, 0, MEM_RELEASE);
	}
}

template<size_t block_size, size_t page_size>
inline void* fixed_size_allocator<block_size, page_size>::alloc(size_t size)
{
	assert(size <= block_size);
	if(size > block_size)
		return nullptr;

	// we have no free blocks, need to allocate new page
	if(free_block == nullptr)
		free_block = allocate_new_page();

	block* allocated_block = free_block;
	free_block = free_block->next_free_block;

	// unmarkered data ahead, need to update pointers
	if(free_block != nullptr && free_block->next_free_block == nullptr)
	{
		block* page_start = get_page_for_address(free_block);

		assert(page_start != nullptr);

		block* page_end = page_start + (page_size/block_size);
		if(free_block + 1 != page_end)
		{
			free_block->next_free_block = free_block + 1;
			free_block->next_free_block->next_free_block = nullptr;
		}
	}

#ifndef NDEBUG
	allocated_blocks.insert(allocated_block);
#endif

	return allocated_block;
}

template<size_t block_size, size_t page_size>
inline void fixed_size_allocator<block_size, page_size>::free(void* address)
{
	block* address_block = (block*) address;

#ifndef NDEBUG
	block* address_page = get_page_for_address(address);
	assert(address_page != nullptr);
	
	const auto removed = allocated_blocks.erase(address_block);
	assert(removed == 1);
#endif

	address_block->next_free_block = free_block;
	free_block = address_block;
}


#ifndef NDEBUG

template<size_t block_size, size_t page_size>
inline void fixed_size_allocator<block_size, page_size>::dumpStat() const
{
	const size_t total_blocks_amount = allocated_pages.size() * (page_size/block_size);
	size_t free_blocks_amount = 0;
	if(free_block != nullptr)
	{
		// Add all free blocks from the list
		block* cur_free_block = free_block;
		while(cur_free_block->next_free_block != nullptr)
		{
			++free_blocks_amount;
			cur_free_block = cur_free_block->next_free_block;
		}

		// Add current and unmarkered free blocks
		const block* page_start = get_page_for_address(cur_free_block);
		const block* page_end = page_start + (page_size/block_size);
		free_blocks_amount += page_end - cur_free_block;
	}
	const size_t allocated_blocks_amount = total_blocks_amount - free_blocks_amount;
	assert(allocated_blocks_amount == allocated_blocks.size());

	std::cout << name << " stats:" << std::endl;
	std::cout << "\tAllocated " << allocated_pages.size() << " page(s), page size is " << page_size << " bytes (" << allocated_pages.size() * page_size << " bytes total)" << std::endl;
	std::cout << "\tBlock size is " << block_size << " bytes" << std::endl;
	std::cout << "\tHas " << free_blocks_amount << " free block(s) (" << free_blocks_amount * block_size << " bytes, " << (float)free_blocks_amount/(float)total_blocks_amount * 100.f << "% of the allocated memory)" << std::endl;
	std::cout << "\tHas " << allocated_blocks_amount << " allocated block(s) (" << allocated_blocks_amount * block_size << " bytes, " << (float)allocated_blocks_amount/(float)total_blocks_amount * 100.f<< "% of the allocated memory)" << std::endl;
}

template<size_t block_size, size_t page_size>
inline void fixed_size_allocator<block_size, page_size>::dumpBlocks() const
{
	std::cout << "Blocks stats for " << name << ":" << std::endl;
	std::cout << "\tAllocated " << allocated_pages.size() << " page(s), page size is " << page_size << " bytes (" << allocated_pages.size() * page_size << " bytes total)" << std::endl;

	if(allocated_blocks.size() == 0)
	{
		std::cout << "\tNo allocated blocks" << std::endl;
	}

	// Can get all allocated blocks without this container, but it's easier and faster just to track them in debug version
	for(block* allocated_block : allocated_blocks)
		std::cout << "\tAllocated block at " << (void*)allocated_block << " (" << block_size << " bytes)" << std::endl;
}

#endif

template<size_t block_size, size_t page_size>
inline typename fixed_size_allocator<block_size, page_size>::block* fixed_size_allocator<block_size, page_size>::allocate_new_page()
{
	void* page = VirtualAlloc(NULL, page_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	assert(page != NULL);

	block* first_block = (block*)page;
	allocated_pages.push_back(first_block);
	first_block->next_free_block = first_block + 1;
	first_block->next_free_block->next_free_block = nullptr;

	return first_block;
}

template<size_t block_size, size_t page_size>
inline typename fixed_size_allocator<block_size, page_size>::block* fixed_size_allocator<block_size, page_size>::get_page_for_address(void* address) const
{
	for(block* page : allocated_pages)
	{
		block* page_end = page + (page_size/block_size);
		if(address >= page && address < page_end)
			return page;
	}

	return nullptr;
}
