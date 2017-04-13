#ifndef SMPI_SHARED_HPP
#define SMPI_SHARED_HPP
#include <xbt/function_types.h>
#include <xbt/misc.h>
#include <vector>


/*
 * We cannot put this declaration in smpi.h, since we use C++ features.
 */


XBT_PUBLIC(int) smpi_is_shared(void* ptr, std::vector<std::pair<size_t, size_t>> &private_blocks, size_t *offset);

std::vector<std::pair<size_t, size_t>> shift_and_frame_private_blocks(const std::vector<std::pair<size_t, size_t>> vec, size_t offset, size_t buff_size);
std::vector<std::pair<size_t, size_t>> merge_private_blocks(std::vector<std::pair<size_t, size_t>> src, std::vector<std::pair<size_t, size_t>> dst);

#endif
