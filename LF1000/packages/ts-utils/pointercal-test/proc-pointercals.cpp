#include <stdio.h>
#include <math.h>

// Input is the 7 "pointercal" numbers, e.g., /flags/pointercal
// Return false if the data is bad
bool cal_test (int a, int b, int c, int d, int e, int f, int g)
{
    // Fixed/Floating point conversion
    const float one = 65536.0;

    float fa = a/one;
    float fb = b/one;
    float fd = d/one;
    float fe = e/one;

    // Thresholds
    const float TDET = 0.01; // Good normalized determinants are really close to 1
    const float TDOT = 0.05; // Good normalized dot products, not so close

    if (g != (int) one)
	return false; // This is just malformed input data.  Last # is 65536

    // Let A be the transform matrix.  Just look at 2D part of affine matrix:
    // A = a b
    //     d e
    //
    // We consider image of unit vectors [1,0] and [0,1] under transform A:
    //   x = A*[1,0] = [a,d]
    //   y = A*[0,1] = [b,e]
    // 
    // We want to check  det(A)/(|x|*|y|) == 1
    //   That is, a parallelogram has same area as determinant,
    //   But only a rectangle has same area as |x|*|y|
    // We want to check (x . y)/(|x|*|y|) == 0
    //   That is, we want a right angle between x and y, which
    //   means cos(angle)=0
    // These tests seem redundant, now that I think of it!
    //
    // This is where the art comes in.
    //
    // Determinant is essentially sin, and Dot is essentially cos.  At
    // 90 deg, sin is not changing, so small errors in determinant
    // won't show up.  At 90 deg, cos is changing fast, so small
    // errors appear big.
    //
    // So why use determinant, if dot is more sensitive?  Because
    // determinant captures the winding direction.  If you hit the
    // targets in backward order, the determinant will be -1 instead
    // of 1.

    float mx = sqrt (fa * fa + fd * fd);
    float my = sqrt (fb * fb + fe * fe);

    float det = fa * fe - fb * fd;
    float dot = fa * fb + fd * fe;
    
    det /= mx*my;
    dot /= mx*my;

    bool baddet = det > 1+TDET || det < 1-TDET;
    bool baddot = dot > 0+TDOT || dot < 0-TDOT;

    bool success = ! (baddet || baddot);

    printf ("Det=%6.2f %s ", det, baddet ? "*" : " ");
    printf ("Dot=%6.2f %s ", dot, baddot ? "*" : " ");
    printf ("%s", success ? "     " : "FAIL ");
    printf ("%d %d %d %d %d %d %d\n", a, b, c, d, e, f, g);

    return success;
}

main ()
{
	int a,b,c,d,e,f,g;
	while (1)
	{
		scanf ("%d %d %d %d %d %d %d", &a, &b, &c, &d, &e, &f, &g);
		bool x = cal_test (a, b, c, d, e, f, g);
	}
}
