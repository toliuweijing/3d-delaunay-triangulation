
#define REORDER_INPLACE
#define PARALLEL


#include <iostream>
#include <fstream>
#include <cassert>
#include <omp.h>
#include "types.h"
using namespace std; 



// find which of the x,y,z axes has the greatest 
// diameter, find its median and partition the points. 
// axe is [0,1,2] representing [x,y.z]
inline void find_greatest_diameter(
    vector<xyz> &xyzs, int pos, int size, int &axe, REAL &median)
{
    //cout << "find_greatest_diameter(){";
    //Output(xyzs, pos, size);

    pair<REAL,REAL> axes[3];
    int to_init = 2;
    //for (auto&& p : xyzs)
    for (int i = pos; i < pos + size; i++)
    {
        xyz p = xyzs[i];

        if (to_init >>=1)
        {
            for (int i = 0; i < 3; i++)
            {
                axes[i] = make_pair(p[i], p[i]);
            }
            continue;
        }
        for (int i = 0; i < 3; i++)
        {
            axes[i].first = min(axes[i].first, p[i]);
            axes[i].second = max(axes[i].second, p[i]);
        }
    }


    double diameter = -1;
    for (int i = 0; i < 3; i++)
    {
        if (axes[i].second - axes[i].first > diameter)
        {
            axe = i;
            diameter = axes[i].second - axes[i].first;
        }
    }
    median = axes[axe].first + diameter/2;
    
    //cout << "axe=" << axe << endl;
    //cout << "median=" << median << endl;
}

// return the num_points on the left size
inline int reorder_points(
    vector<xyz> &xyzs, int pos, int size, int axe, REAL median)
{
    //cout << "reorder_points(){";
    //Output(xyzs, pos, size);

    vector<xyz> left;
    vector<xyz> right;

    //for (auto&& p: xyzs)
    for (int i = pos; i < pos+size; i++)
    {
        xyz p = xyzs[i];

        if (p[axe] <= median)
        {
            left.push_back(p);
        }
        else
        {
            right.push_back(p);
        }
    }

    int k = pos;
    for (auto&& p : left)
    {
        xyzs[k++] = p;
    }
    for (auto&& p: right)
    {
        xyzs[k++] = p;
    }
    
    return left.size();
}

// return the num_points on the left size
inline int reorder_points_inplace(
    vector<xyz> &xyzs, int pos, int size, int axe, REAL median)
{
    int l = pos;
    int r = pos + size - 1;
    while (l <= r)
    {
        if (xyzs[l][axe] > median && 
            xyzs[r][axe] <= median)
        {
            swap(xyzs[l], xyzs[r]);
        }
        if (xyzs[l][axe] <= median)
            l++;
        if (xyzs[r][axe] > median)
            r--;
    }
    //cout << "median=" << median << endl;
    //Output(xyzs, pos, size);
    return l - pos;
}

inline void spatial_sort_kernel(vector<xyz> &xyzs, int pos, int size)
{
    //cout << "thread_num=" << omp_get_thread_num() << endl;
    assert(pos >= 0 && pos + size <= xyzs.size());
    if (size < 2)
        return;

    int axe;
    REAL median;

    find_greatest_diameter(xyzs, pos, size, axe, median);

#ifdef REORDER_INPLACE
    int left_size = reorder_points_inplace(xyzs, pos, size, axe, median);
#else
    int left_size = reorder_points(xyzs, pos, size, axe, median);
#endif

#pragma omp task shared(xyzs)
    spatial_sort_kernel(xyzs, pos, left_size);
#pragma omp task shared(xyzs)
    spatial_sort_kernel(xyzs, pos + left_size, size - left_size);
}

void spatial_sort(std::vector<xyz> &xyzs)
{
#ifndef PARALLEL
    omp_set_num_threads(1);
#endif

    #pragma omp parallel //num_threads
    {
        //cout << "run_spatial_sort_with_num_threads=" << omp_get_num_threads() << endl;
        #pragma omp single
        spatial_sort_kernel(xyzs, 0, xyzs.size());
    }
}

void spatial_sort(vector<xyz> &xyzs, int num_threads)
{
    omp_set_num_threads(num_threads);

    #pragma omp parallel //num_threads
    {
        //cout << "run_spatial_sort_with_num_threads=" << omp_get_num_threads() << endl;
        #pragma omp single
        spatial_sort_kernel(xyzs, 0, xyzs.size());
    }
}


