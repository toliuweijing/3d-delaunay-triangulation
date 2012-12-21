#ifndef PREDICATES_H_
#define PREDICATES_H_

#include <array>

// Must be consistant with predicates.c.
#define REAL double

extern "C" {
	extern void exactinit();

	/*Return a positive value if the point pd lies below the plane */
	/*passing through pa, pb, and pc; "below" is defined so */
	/*that pa, pb, and pc appear in counterclockwise order when   */
	/*viewed from above the plane.  Returns a negative value if   */
	/*pd lies above the plane.  Returns zero if the points are coplanar. */
	extern REAL orient3d(REAL *pa, REAL *pb, REAL *pc, REAL *pd);
	extern REAL orient3dfast(REAL *pa, REAL *pb, REAL *pc, REAL *pd);

	/*Return a positive value if the point pe lies inside the     */
	/*sphere passing through pa, pb, pc, and pd; a negative value */
	/*if it lies outside; and zero if the five points are         */
	/*cospherical.  The points pa, pb, pc, and pd must be ordered */
	/*so that they have a positive orientation (as defined by     */
	/*orient3d()), or the sign of the result will be reversed.    */
	extern REAL inspherefast(REAL *pa, REAL *pb, REAL *pc, REAL *pd, REAL *pe);
	extern REAL insphere(REAL *pa, REAL *pb, REAL *pc, REAL *pd, REAL *pe);
}


// REAL3 store 3 double numbers
typedef std::array<REAL, 3> REAL3;

// function to test if a point inside a tetraheron or not 
// pa, pb, pc, pd are four vertex of a tetraheron, pe is the test point
// return -1 outside the tetra, return 0 on tetra, return 1 inside tetra.
inline int intetra(REAL *pa, REAL *pb, REAL *pc, REAL *pd, REAL *pe) {
#define ORIENT3DFAST
#ifdef ORIENT3DFAST
    /*
    REAL res[4][2] = {
        { orient3dfast(pa, pb, pc, pd), orient3dfast(pa, pb, pc, pe) },
        { orient3dfast(pa, pb, pd, pc), orient3dfast(pa, pb, pd, pe) }, 
        { orient3dfast(pa, pc, pd, pb), orient3dfast(pa, pc, pd, pe) },
        { orient3dfast(pb, pc, pd, pa), orient3dfast(pb, pc, pd, pe) }
    };
    */
    REAL res[5] = {orient3dfast(pa, pb, pc, pd),orient3dfast(pe, pb, pc, pd),orient3dfast(pa, pe, pc, pd),orient3dfast(pa, pb, pe, pd),orient3dfast(pa, pb, pc, pe)};
#else
    /*
    REAL res[4][2] = {
        { orient3d(pa, pb, pc, pd), orient3d(pa, pb, pc, pe) },
        { orient3d(pa, pb, pd, pc), orient3d(pa, pb, pd, pe) }, 
        { orient3d(pa, pc, pd, pb), orient3d(pa, pc, pd, pe) },
        { orient3d(pb, pc, pd, pa), orient3d(pb, pc, pd, pe) }
    };
    */
    REAL res[5] = {orient3d(pa, pb, pc, pd),orient3d(pe, pb, pc, pd),orient3d(pa, pe, pc, pd),orient3d(pa, pb, pe, pd),orient3d(pa, pb, pc, pe)};
#endif
   
     for (int i=0;i<5;i++) {
            if(res[i]==0)
                return 0;
            if( ((res[0]<0)&&(res[i]>0)) || ((res[0]>0)&&(res[i]<0)) )
                return -1;
     }
     return 1;
/*
    bool coplanar = false;  // should be set true if pe is at least on one plane
    bool inplane = true;    // should be set false if pe stay by the opposite side
                            // to at least one plane
                           
    for (int i = 0; i < 4; i++)
    {
        if (res[i][1] == 0)
        {// pe is at least on one plane 
            coplanar = true;
            continue;
        }
        if ((res[i][0] > 0) != (res[i][1] > 0))
        {// pe is at least on the opposite side to one plane
            inplane = false;
            break;
        }
    }
    
    if (inplane == false)
        return -1;
    if (inplane == true && coplanar == true)
        return 0;
    return 1;
    */
};

inline REAL insphere_with_adjust(REAL *pa, REAL *pb, REAL *pc, REAL *pd, REAL *pe) {
     return ( orient3dfast(pa, pb, pc, pd) < 0 ? inspherefast(pa, pc, pb, pd, pe) : inspherefast(pa, pb, pc, pd, pe) );
};

#endif
