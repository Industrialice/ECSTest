#include <PreHeader.hpp>
#include <stdio.h>

void Benchmark();
void Benchmark2();
void KeyControllerTests();
void SyncTests();
void Falling();
void InteractionTests();

int main()
{
    printf("1. KeyControllerTests\n");
    printf("2. InteractionTests\n");
    printf("3. SyncTests\n");
    printf("4. Benchmark\n");
    printf("5. Falling\n");
    printf("6. Benchmark2\n");

restart:
    int choice = 0;
    scanf_s("%i", &choice);
    printf("\n");

    switch (choice)
    {
    case 1:
        KeyControllerTests();
        break;
    case 2:
        InteractionTests();
        break;
    case 3:
        SyncTests();
        break;
    case 4:
        Benchmark();
        break;
    case 5:
        Falling();
        break;
    case 6:
        Benchmark2();
        break;
    default:
        printf("Incorrect input, try again\n");
        goto restart;
    }

    printf("Test complete\n\n");
    goto restart;
}