#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
    for (unsigned int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }

    return 0;
}
