#include <iostream>
#include <fstream>
#include <cassert>
#include <chrono>
#include "../spatialsort.h"
#include "types.h"
using namespace std; 



vector<xyz> xyzs, a1, a2;
void OutputToMatlab(vector<xyz>& xyzs, char *filename)
{
    ofstream os(filename);
    
    os << "X=[\n";
    for (auto&& p : xyzs)
    {
        os << p[0] << ",";
        os << p[1] << ",";
        os << p[2] << ";\n";
    }
    os << "]";
    os.close();
}
int main(int argc, const char *argv[])
{
    for (int i = 0; i < 100000; i++)
    {
        xyz p = { 
            rand()*1.0/RAND_MAX, 
            rand()*1.0/RAND_MAX, 
            rand()*1.0/RAND_MAX 
        };
        xyzs.push_back(p);
    }

    a1=a2=xyzs;

    //OutputToMatlab(xyzs, "xyzs_unsorted.m"); 
	auto start_time = chrono::steady_clock::now();
    spatial_sort(xyzs,1);
	auto end_time = chrono::steady_clock::now();
	double time = 0.001 * chrono::duration_cast<chrono::milliseconds>
			(end_time - start_time).count();
	cout << "Execution Time: " << time << endl;

    //OutputToMatlab(xyzs, "xyzs_sorted.m"); 


    return 0;
}
