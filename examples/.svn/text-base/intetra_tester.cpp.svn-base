// compiple by g++ intetra_tester.cpp ../predicates.o -std=c++0x -ltbb 
#include <iostream>
#include "../predicates.h"
using namespace std; 


ostream& operator<<(ostream& os, REAL v[3])
{
    cout << "point("
         << v[0] << ','
         << v[1] << ','
         << v[2] << ')';
    return os;
}

int main(int argc, const char *argv[])
{

    REAL vs[][3] = {
        { 0,0,0 },
        { 1,0,0 },
        { 0,1,0 },
        { 0,0,1 }
    };


    cout     << "Tetra:"  << endl;
    cout     << "{ 0,0,0 }" << endl;
    cout     << "{ 1,0,0 }" << endl;
    cout     << "{ 0,1,0 }" << endl;
    cout     << "{ 0,0,1 }" << endl;


    REAL e[][3] = {
        { 0,0,0 },
        { 1,0,0 }, 
        { 0,1,0 },
        { 0,0,1 },
        { 0,0,-0.1},
        { 0.01,0.01,0.01 },
        { 0.25,0.25,0.25 }
    };


    exactinit();
    for (int i = 0; i < sizeof(e)/(sizeof(REAL)*3); i++)
    {
        cout << e[i] << " is " << intetra(vs[0], vs[1], vs[2], vs[3], e[i]) << " to the tetra" << endl;
    }


    return 0;
}
