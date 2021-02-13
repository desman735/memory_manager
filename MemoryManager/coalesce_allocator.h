#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include <windows.h>

#include "definitions.h"

template<size_t data_block_size = 12, size_t page_size = 4*KB>
class coalesce_allocator
{
public:

	coalesce_allocator(std::string debug_name);
	~coalesce_allocator();

	void* alloc(size_t size);
	void free(void* address);

#ifndef NDEBUG
	void dumpStat() const;
	void dumpBlocks() const;
#endif

public:

	union data
	{
		uint8_t data[data_block_size];
		struct block* next_free_block;
	};

	struct block
	{
		uint32_t block_length;
		data block_data;
	};

	static constexpr size_t serivice_info_size = sizeof(block::block_length);
	static constexpr size_t real_block_size = sizeof(block);

	//static_assert(page_size > real_block_size, "page should contain at least two blocks (with 4 bytes of service info)");
	//static_assert(page_size % real_block_size == 0, "page should contain an integer number of blocks (taking 4 bytes of service info into account)");
	
	block* allocate_new_page();
	block* get_page_for_address(void* address) const;

	std::vector<block*> allocated_pages;

#ifndef NDEBUG
	std::string name;
#endif
};

template<size_t data_block_size, size_t page_size>
inline coalesce_allocator<data_block_size, page_size>::coalesce_allocator(std::string debug_name)
{
}

template<size_t data_block_size, size_t page_size>
inline coalesce_allocator<data_block_size, page_size>::~coalesce_allocator()
{
}

template<size_t data_block_size, size_t page_size>
inline void* coalesce_allocator<data_block_size, page_size>::alloc(size_t size)
{
	return NULL;
}

template<size_t data_block_size, size_t page_size>
inline void coalesce_allocator<data_block_size, page_size>::free(void* address)
{
}

template<size_t data_block_size, size_t page_size>
inline void coalesce_allocator<data_block_size, page_size>::dumpStat() const
{
}

template<size_t data_block_size, size_t page_size>
inline void coalesce_allocator<data_block_size, page_size>::dumpBlocks() const
{
}

template<size_t data_block_size, size_t page_size>
inline typename coalesce_allocator<data_block_size, page_size>::block* coalesce_allocator<data_block_size, page_size>::allocate_new_page()
{
	return NULL;
}

template<size_t data_block_size, size_t page_size>
inline typename coalesce_allocator<data_block_size, page_size>::block* coalesce_allocator<data_block_size, page_size>::get_page_for_address(void* address) const
{
	return NULL;
}
