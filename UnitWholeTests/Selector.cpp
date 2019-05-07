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
    printf("1. Benchmark\n");
    printf("2. Benchmark2\n");
    printf("3. KeyControllerTests\n");
    printf("4. SyncTests\n");
    printf("5. Falling\n");
    printf("6. InteractionTests\n");

restart:
    int choice = 0;
    scanf_s("%i", &choice);
    printf("\n");

    switch (choice)
    {
    case 1:
        Benchmark();
        break;
    case 2:
        Benchmark2();
        break;
    case 3:
        KeyControllerTests();
        break;
    case 4:
        SyncTests();
        break;
    case 5:
        Falling();
        break;
    case 6:
        InteractionTests();
        break;
    default:
        printf("Incorrect input, try again\n");
        goto restart;
    }

    printf("Test complete\n\n");
    goto restart;
}