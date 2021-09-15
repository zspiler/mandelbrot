#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <complex.h>
#include <time.h>

const int width = 1750;
const int height = 1000;
const int R = 2 * 2; // escape radius
const int max_iterations = 6000;

struct Color {
    int r;
    int g;
    int b;
};

// struct Color map_to_color(int i) {
//     int r = 0;
//     int g = 0;
//     int b = 0;

//     if (i < max_iterations / 3) {
//         r = 255.0 / (max_iterations / 3) * i;
//     }
//     else if (i < 2/3 * max_iterations) {
//         r = 255;
//         g = 255.0 / (2/3 * max_iterations - max_iterations / 3) * (i - max_iterations / 3);
//     }
//     else {
//         r = 255;
//         g = 255;
//         b = 255.0 / (max_iterations - (2/3 * max_iterations)) * (i - (2/3 * max_iterations));
//     }
//     struct Color c = {.r = r, .g = g, .b = b};
//     return c;

// }

struct Color map_to_color(int i) {

    // pink

    int r = 0;
    int g = 0;
    int b = 0;

    if (i < max_iterations / 3) {
        b = 255.0 / (max_iterations / 3) * i;
        r = 255.0 / (max_iterations / 3) * i;
    }
    else if (i < 2/3 * max_iterations) {
        b = 255;
        g = 255.0 / (2/3 * max_iterations - max_iterations / 3) * (i - max_iterations / 3);
    }
    else {
        b = 255;
        g = 255;
        r = 255.0 / (max_iterations - (2/3 * max_iterations)) * (i - (2/3 * max_iterations));
    }

    // BW
    // r = 255.0 / (max_iterations) * i;
    // g = r;
    // b = r;

    struct Color c = {.r = r, .g = g, .b = b};
    return c;
}

/*                                                                     
    -0.743291725    -0.34853774148008254    -0.34842274148008254    -0.34842274148008254
    -0.131234623    -0.6065922085831237     -0.606515085831237      -0.606515085831237
    0.001           0.0001                  0.0001                  0.00000004
    spiral          tunnel                  tunel             
*/


int main() {

    clock_t begin = clock();

    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);
    cairo_t *cr = cairo_create (surface);

    double zoom = 0.00001; 

    double size_x = 3.5 * zoom; // default X range (-2.5, 1)
    double size_y = 2 * zoom; // default Y range (-1, 1)


    // double focus_x = -0.34842274148008254; // default -0.75
    // double focus_y = -0.606515085831237; // default 0  

    double focus_x = -0.348513; // default -0.75
    double focus_y = -0.6065922085831237; // default 0  

    double fromR = focus_x - size_x/2; 
    double toR = focus_x + size_x/2; 
    double fromI = focus_y - size_y/2; 
    double toI = focus_y + size_y/2; 

    for(int y = 0; y <= height; y++) {
        for(int x = 0; x <= width; x++) {
            double x0 = (((double)x/(double)width) * (toR - (fromR)) + fromR); 
            double y0 = (((double)y/(double)height) * (toI - fromI) + fromI); 

            double complex z = 0.0 + 0.0 * I;  
            int i = 0;
            while (creal(z) + cimag(z) < R && i < max_iterations) {
                double complex c = x0 + y0 * I;             
                z = z * z + c;
                i++;
            }

            if (i < max_iterations) {
                struct Color col = map_to_color(i);

                cairo_set_source_rgb (cr, col.r/255.0, col.g/255.0, col.b/255.0);
                cairo_rectangle (cr, x, y, 1.0, 1.0);
                cairo_fill (cr);
            }
        }
        
        printf("\rrow %d/%d", y, height);
        fflush(stdout);
    }

    cairo_destroy (cr);
    cairo_surface_write_to_png (surface, "image.png");

    cairo_surface_destroy (surface);
    
    printf("\ntime: %f\n", (double)(clock() - begin) / CLOCKS_PER_SEC);

    return 0;
}

