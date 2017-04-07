#ifndef SMPI_SHARED_HPP
#define SMPI_SHARED_HPP
#include <xbt/function_types.h>
#include <xbt/misc.h>
#include <vector>


/*
 * We cannot put this declaration in smpi.h, since we use C++ features.
 */


XBT_PUBLIC(int) smpi_is_shared(void* ptr, std::vector<std::pair<int, int>> &private_blocks, int *offset);

std::vector<std::pair<int, int>> shift_and_frame_private_blocks(const std::vector<std::pair<int, int>> vec, int offset, int buff_size);
std::vector<std::pair<int, int>> merge_private_blocks(std::vector<std::pair<int, int>> src, std::vector<std::pair<int, int>> dst);

#endif
