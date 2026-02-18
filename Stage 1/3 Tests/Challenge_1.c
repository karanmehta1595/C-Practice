#include <stdio.h>

int main() {
        int a,b,c,d;
        float x,y;

        a = 63;
        b = 23;
        c = 42;
        d = 96;
        x = 23.42;
        y = 12.6287;

    int Result = (a+b%c)*(d-a/c);
    float Result2  = (a/y*x)+a%d;
    /*float Result2  = (a/y*x)+(d-y);*/ /*here i was not be able to use % i don't know the exact reason
                                will you help me out with this confussion */

    printf("Result For intiger will be %d\n", Result);
    printf("Result For float will be %.2f\n", Result2);

    return 0;
}