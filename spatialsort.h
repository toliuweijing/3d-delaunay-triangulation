#ifndef SPATIALSORT_H

#define SPATIALSORT_H

#include <vector>
#include "types.h"

void spatial_sort(std::vector<xyz> &xyzs);
void spatial_sort(std::vector<xyz> &xyzs, int num_threads);


#endif /* end of include guard: SPATIALSORT_H */
