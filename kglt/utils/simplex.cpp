#include <cmath>
#include <cstdlib>
#include <kazmath/kazmath.h>

#include "simplex.h"

int fastfloor(double x) {
    return x > 0 ? (int)x : (int)x-1;
}

double dot(int* g, double x, double y, double z, double w) {
    return g[0] * x + g[1] * y + g[2] * z + g[3] * w;
}

bool Simplex::init() {
    srand(seed_);

    p.clear();
    for(int i = 0; i < 256; ++i) {
        p.push_back(i);
    }

    for(int i = 0; i < 256; ++i) {
        int j = rand() % 256;
        std::swap(p[i], p[j]);
    }

    perm.resize(512);

    for(int i = 0; i < 512; ++i) {
        perm[i] = p[i & 255];
    }

    float range_x0 = -1;
    float range_x1 = 1;
    float range_y0 = -1;
    float range_y1 = 1;

    for(int x = 0; x < buffer_width_; ++x) {
        for(int y = 0; y < buffer_height_; ++y) {
            float s = float(x) / float(buffer_width_);
            float t = float(y) / float(buffer_height_);

            float dx = range_x1 - range_x0;
            float dy = range_y1 - range_y0;

            float nx = range_x1 + cos(s * 2 * kmPI) * dx / (2 * kmPI);
            float ny = range_y1 + cos(t * 2 * kmPI) * dy / (2 * kmPI);
            float nz = range_x1 + sin(s * 2 * kmPI) * dx / (2 * kmPI);
            float nw = range_y1 + sin(t * 2 * kmPI) * dy / (2 * kmPI);

            buffer[(y * buffer_width_) + x] = noise(nx, ny, nz, nw);
        }
    }

    return true;
}

Simplex::Simplex(int width, int height, int seed):
    buffer_width_(width),
    buffer_height_(height),
    seed_(seed) {

    buffer.resize(width * height);
}

double Simplex::get(float x, float y) {
    return buffer[(y * buffer_width_) + x];
}

double Simplex::noise(double x, double y, double z, double w) {
    // The skewing and unskewing factors are hairy again for the 4D case
    const double F4 = (sqrt(5.0) - 1.0) / 4.0;
    const double G4 = (5.0 - sqrt(5.0)) / 20.0;
    double n0, n1, n2, n3, n4;

    // Noise contributions from the five corners
    // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
    double s = (x + y + z + w) * F4;

    // Factor for 4D skewing
    int i = fastfloor(x + s);
    int j = fastfloor(y + s);
    int k = fastfloor(z + s);
    int l = fastfloor(w + s);

    double t = (i + j + k + l) * G4;

    // Factor for 4D unskewing
    double X0 = i - t;

    // Unskew the cell origin back to (x,y,z,w) space
    double Y0 = j - t;
    double Z0 = k - t;
    double W0 = l - t;
    double x0 = x - X0;

    // The x,y,z,w distances from the cell origin
    double y0 = y - Y0;
    double z0 = z - Z0;
    double w0 = w - W0;

    // For the 4D case, the simplex is a 4D shape I won't even try to describe.
    // To find out which of the 24 possible simplices we're in, we need to
    // determine the magnitude ordering of x0, y0, z0 and w0.
    // The method below is a good way of finding the ordering of x,y,z,w and
    // then find the correct traversal order for the simplex we’re in.
    // First, six pair-wise comparisons are performed between each possible pair
    // of the four coordinates, and the results are used to add up binary bits
    // for an integer index.
    int c1 = (x0 > y0) ? 32 : 0;
    int c2 = (x0 > z0) ? 16 : 0;
    int c3 = (y0 > z0) ? 8 : 0;
    int c4 = (x0 > w0) ? 4 : 0;
    int c5 = (y0 > w0) ? 2 : 0;
    int c6 = (z0 > w0) ? 1 : 0;
    int c = c1 + c2 + c3 + c4 + c5 + c6;
    int i1, j1, k1, l1;
    // The integer offsets for the second simplex corner
    int i2, j2, k2, l2;
    // The integer offsets for the third simplex corner
    int i3, j3, k3, l3;
    // The integer offsets for the fourth simplex corner
    // simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
    // Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
    // impossible. Only the 24 indices which have non-zero entries make any sense.
    // We use a thresholding to set the coordinates in turn from the largest magnitude.
    // The number 3 in the "simplex" array is at the position of the largest coordinate.
    i1 = simplex[c][0]>=3 ? 1 : 0;
    j1 = simplex[c][1]>=3 ? 1 : 0;
    k1 = simplex[c][2]>=3 ? 1 : 0;
    l1 = simplex[c][3]>=3 ? 1 : 0;
    // The number 2 in the "simplex" array is at the second largest coordinate.
    i2 = simplex[c][0]>=2 ? 1 : 0;
    j2 = simplex[c][1]>=2 ? 1 : 0;
    k2 = simplex[c][2]>=2 ? 1 : 0;
    l2 = simplex[c][3]>=2 ? 1 : 0;
    // The number 1 in the "simplex" array is at the second smallest coordinate.
    i3 = simplex[c][0]>=1 ? 1 : 0;
    j3 = simplex[c][1]>=1 ? 1 : 0;
    k3 = simplex[c][2]>=1 ? 1 : 0;
    l3 = simplex[c][3]>=1 ? 1 : 0;
    // The fifth corner has all coordinate offsets = 1, so no need to look that up.
    double x1 = x0 - i1 + G4;
    // Offsets for second corner in (x,y,z,w) coords
    double y1 = y0 - j1 + G4;
    double z1 = z0 - k1 + G4;
    double w1 = w0 - l1 + G4;
    double x2 = x0 - i2 + 2.0*G4;
    // Offsets for third corner in (x,y,z,w) coords
    double y2 = y0 - j2 + 2.0*G4;
    double z2 = z0 - k2 + 2.0*G4;
    double w2 = w0 - l2 + 2.0*G4;
    double x3 = x0 - i3 + 3.0*G4;
    // Offsets for fourth corner in (x,y,z,w) coords
    double y3 = y0 - j3 + 3.0*G4;
    double z3 = z0 - k3 + 3.0*G4;
    double w3 = w0 - l3 + 3.0*G4;
    double x4 = x0 - 1.0 + 4.0*G4;
    // Offsets for last corner in (x,y,z,w) coords
    double y4 = y0 - 1.0 + 4.0*G4;
    double z4 = z0 - 1.0 + 4.0*G4;
    double w4 = w0 - 1.0 + 4.0*G4;
    // Work out the hashed gradient indices of the five simplex corners
    int ii = i & 255;
    int jj = j & 255;
    int kk = k & 255;
    int ll = l & 255;
    int gi0 = perm[ii+perm[jj+perm[kk+perm[ll]]]] % 32;
    int gi1 = perm[ii+i1+perm[jj+j1+perm[kk+k1+perm[ll+l1]]]] % 32;
    int gi2 = perm[ii+i2+perm[jj+j2+perm[kk+k2+perm[ll+l2]]]] % 32;
    int gi3 = perm[ii+i3+perm[jj+j3+perm[kk+k3+perm[ll+l3]]]] % 32;
    int gi4 = perm[ii+1+perm[jj+1+perm[kk+1+perm[ll+1]]]] % 32;

    // Calculate the contribution from the five corners
    double t0 = 0.6 - x0*x0 - y0*y0 - z0*z0 - w0*w0;
    if(t0 < 0) {
        n0 = 0.0;
    } else {
        t0 *= t0;
        n0 = t0 * t0 * dot(grad4[gi0], x0, y0, z0, w0);
    }

    double t1 = 0.6 - x1*x1 - y1*y1 - z1*z1 - w1*w1;

    if(t1 < 0) {
        n1 = 0.0;
    } else {
        t1 *= t1;
        n1 = t1 * t1 * dot(grad4[gi1], x1, y1, z1, w1);
    }

    double t2 = 0.6 - x2*x2 - y2*y2 - z2*z2 - w2*w2;

    if(t2 < 0) {
        n2 = 0.0;
    } else {
        t2 *= t2;
        n2 = t2 * t2 * dot(grad4[gi2], x2, y2, z2, w2);
    }

    double t3 = 0.6 - x3*x3 - y3*y3 - z3*z3 - w3*w3;

    if(t3 < 0) {
        n3 = 0.0;
    } else {
        t3 *= t3;
        n3 = t3 * t3 * dot(grad4[gi3], x3, y3, z3, w3);
    }

    double t4 = 0.6 - x4*x4 - y4*y4 - z4*z4 - w4*w4;

    if(t4 < 0) {
        n4 = 0.0;
    } else {
        t4 *= t4;
        n4 = t4 * t4 * dot(grad4[gi4], x4, y4, z4, w4);
    }

    // Sum up and scale the result to cover the range [-1,1]
    return 27.0 * (n0 + n1 + n2 + n3 + n4);
}
