#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <complex.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
       
#include <unistd.h>

#define NTHREADS 8
#define WIDTH 1750
#define HEIGHT 1000

#define R 2 * 2 // escape radius
#define FOCUS_X -0.743291725 // default -0.75
#define FOCUS_Y -0.131234623 // default 0
#define MAX_ITER 1000
#define ZOOM 0.001 // default 1

struct Color { 
    int r; 
    int g; 
    int b;
};

struct Color map_to_color(int i, int mode) {

    int r = 0;
    int g = 0;
    int b = 0;

    switch (mode){
        default:
        case 0: // b&w
            r = 255.0 / (MAX_ITER) * i;
            g = r;
            b = r;
            break;
        case 1: // red
            if (i < MAX_ITER / 3) {
                r = 255.0 / (MAX_ITER / 3) * i;
            }
            else if (i < 2/3 * MAX_ITER) {
                r = 255;
                g = 255.0 / (2/3 * MAX_ITER - MAX_ITER / 3) * (i - MAX_ITER / 3);
            }
            else {
                r = 255;
                g = 255;
                b = 255.0 / (MAX_ITER - (2/3 * MAX_ITER)) * (i - (2/3 * MAX_ITER));
            }
            break;
    
        case 2: // purple
            if (i < MAX_ITER / 3) {
                b = 255.0 / (MAX_ITER / 3) * i;
                r = 255.0 / (MAX_ITER / 3) * i;
            }
            else if (i < 2/3 * MAX_ITER) {
                b = 255;
                g = 255.0 / (2/3 * MAX_ITER - MAX_ITER / 3) * (i - MAX_ITER / 3);
            }
            else {
                b = 255;
                g = 255;
                r = 255.0 / (MAX_ITER - (2/3 * MAX_ITER)) * (i - (2/3 * MAX_ITER));
            }
            break;
    }
    

    struct Color c = {.r = r, .g = g, .b = b};
    return c;
}

/*                                                                     
    -0.743291725    -0.34853774148008254    -0.34842274148008254    -0.34842274148008254    -0.348513
    -0.131234623    -0.6065922085831237     -0.606515085831237      -0.606515085831237      -0.6065922085831237
    0.001           0.0001                  0.0001                  0.00000004
    spiral          tunnel                  tunel             
*/

struct ThreadArgs {
    int start_y;
    int end_y;
    cairo_t *cr;
    struct Color (*image)[HEIGHT][WIDTH];
};


void *draw(void *vargp) {    
    struct ThreadArgs* args = ((struct ThreadArgs*)vargp);

    const double fromR = FOCUS_X - (3.5 * ZOOM)/2; 
    const double toR = FOCUS_X + (3.5 * ZOOM)/2; 
    const double fromI = FOCUS_Y - (2 * ZOOM)/2; 
    const double toI = FOCUS_Y + (2 * ZOOM)/2; 

    for(int y = args->start_y; y < args->end_y; y++) {
        for(int x = 0; x < WIDTH; x++) {
            double x0 = (((double)x/(double)WIDTH) * (toR - (fromR)) + fromR); 
            double y0 = (((double)y/(double)HEIGHT) * (toI - fromI) + fromI); 

            double complex z = 0.0 + 0.0 * I;  
            int i = 0;

            while (creal(z) + cimag(z) < R && i < MAX_ITER) {
                double complex c = x0 + y0 * I;             
                z = z * z + c;
                i++;
            }
            
            if (i < MAX_ITER) {
                struct Color col = map_to_color(i, 1);
                (*(args->image))[y][x] = col;
            }
        }
        fflush(stdout);
    }
    return NULL;
}


int main() {

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, WIDTH, HEIGHT);
    cairo_t *cr = cairo_create (surface);

    pthread_t threads[NTHREADS];

    static struct Color image[HEIGHT][WIDTH];

    struct ThreadArgs *args;
    for (int i = 0; i < NTHREADS; i++) {    
        args = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));
        args->start_y = i * HEIGHT/NTHREADS;
        args->end_y = i * HEIGHT/NTHREADS + HEIGHT/NTHREADS;
        args->cr = cr;
        args->image = &image;
    
        pthread_create(&threads[i], NULL, draw, args);
        
    }
    free(args);

    for (int i = 0; i < NTHREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // plot 
    for(int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            struct Color col = image[y][x]; 
            cairo_set_source_rgb (cr, col.r/255.0, col.g/255.0, col.b/255.0);
            cairo_rectangle (cr, x, y, 1.0, 1.0);
            cairo_fill (cr);
        }
    }

    // create image
    cairo_destroy (cr);
    cairo_surface_write_to_png (surface, "image.png");
    cairo_surface_destroy (surface);

   
    gettimeofday(&end_time, NULL);
    double exec_time = end_time.tv_sec + end_time.tv_usec / 1e6 -
                        start_time.tv_sec - start_time.tv_usec / 1e6; 
    printf("time: %.2fs\n", exec_time);

    return 0;
}

