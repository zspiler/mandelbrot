#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <complex.h>
#include <sys/time.h>
#include <math.h>
#include <pthread.h>
       
#include <unistd.h>


const int width = 1750;
const int height = 1000;
const int R = 2 * 2; // escape radius

const double focus_x = -0.743291725;
const double focus_y = -0.131234623;

const int max_iterations = 1000;
const double zoom = 0.001;


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
            r = 255.0 / (max_iterations) * i;
            g = r;
            b = r;
            break;
        case 1: // red
            if (i < max_iterations / 3) {
                r = 255.0 / (max_iterations / 3) * i;
            }
            else if (i < 2/3 * max_iterations) {
                r = 255;
                g = 255.0 / (2/3 * max_iterations - max_iterations / 3) * (i - max_iterations / 3);
            }
            else {
                r = 255;
                g = 255;
                b = 255.0 / (max_iterations - (2/3 * max_iterations)) * (i - (2/3 * max_iterations));
            }
            break;
    
        case 2: // purple
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


// default focus (-0.75, 0), zoom 1

void draw(char *filename) { 

    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);
    cairo_t *cr = cairo_create (surface);

    const double from_re = focus_x - (3.5 * zoom)/2; 
    const double to_re = focus_x + (3.5 * zoom)/2; 
    const double from_im = focus_y - (2 * zoom)/2; 
    const double to_im = focus_y + (2 * zoom)/2; 

    for(int y = 0; y <= height; y++) {
        for(int x = 0; x <= width; x++) {
            double x0 = (((double)x/(double)width) * (to_re - (from_re)) + from_re); 
            double y0 = (((double)y/(double)height) * (to_im - from_im) + to_im); 

            double complex z = 0.0 + 0.0 * I;  
            int i = 0;
            while (creal(z) + cimag(z) < R && i < max_iterations) {
                double complex c = x0 + y0 * I;             
                z = z * z + c;
                i++;
            }

            if (i < max_iterations) {
                struct Color col = map_to_color(i, 1);
                // plot
                cairo_set_source_rgb (cr, col.r/255.0, col.g/255.0, col.b/255.0);
                cairo_rectangle (cr, x, y, 1.0, 1.0);
                cairo_fill (cr);
            }
        }
        
        printf("\rrow %d/%d", y, height);
        fflush(stdout);
    }

    cairo_destroy (cr);
    cairo_surface_write_to_png (surface, strlen(filename) > 0 ? filename : "image.png");
    cairo_surface_destroy (surface);
}


struct ThreadArgs {
    int start_y;
    int end_y;
    double zoom;
    cairo_t *cr;
    struct Color (*image)[height][width];
};


void *draw_multithread(void *vargp) {    
    struct ThreadArgs* args = ((struct ThreadArgs*)vargp);

    const double fromR = focus_x - (3.5 * zoom)/2; 
    const double toR = focus_x + (3.5 * zoom)/2; 
    const double fromI = focus_y - (2 * zoom)/2; 
    const double toI = focus_y + (2 * zoom)/2; 

    for(int y = args->start_y; y < args->end_y; y++) {
        for(int x = 0; x < width; x++) {
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


    // draw(0.0000000000001, "");

    // int i = 0;
    // for (double zoom = 1; zoom >= 0.0001; zoom *= 0.9) { // 0.99
    //     char file[100];
    //     sprintf(file, "./sequence/%d.png", i);    
    //     i += 1;
    //     draw(-0.34853774148008254, -0.6065922085831237, zoom, file);
    // }
    

    // Multithread
    cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, width, height);
    cairo_t *cr = cairo_create (surface);

    const int num_of_threads = 4;
    pthread_t threads[num_of_threads];

    static struct Color image[height][width];

    struct ThreadArgs *args;
    for (int i = 0; i < num_of_threads; i++) {    
        args = (struct ThreadArgs *) malloc(sizeof(struct ThreadArgs));
        args->start_y = i * height/num_of_threads;
        args->end_y = i * height/num_of_threads + height/num_of_threads;
        args->cr = cr;
        args->image = &image;
    
        pthread_create(&threads[i], NULL, draw_multithread, args);
    }
    free(args);

    for (int i = 0; i < num_of_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // plot 
    for(int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            struct Color col = image[y][x]; // To je sam en args retard
            cairo_set_source_rgb (cr, col.r/255.0, col.g/255.0, col.b/255.0);
            cairo_rectangle (cr, x, y, 1.0, 1.0);
            cairo_fill (cr);
        }
    }

    cairo_destroy (cr);
    cairo_surface_write_to_png (surface, "image.png");
    cairo_surface_destroy (surface);


    
   
    gettimeofday(&end_time, NULL);
    double exec_time = end_time.tv_sec + end_time.tv_usec / 1e6 -
                        start_time.tv_sec - start_time.tv_usec / 1e6; 
    printf("\ntime: %.2f\n", exec_time);

    return 0;
}

