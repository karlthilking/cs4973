#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>

void
swap(int *a, int *b)
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

int
partition(int arr[], int low, int high)
{
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; ++j) {
        if (arr[j] < pivot) {
            ++i;
            swap(arr + i, arr + j);
        }
    }
    swap(arr + i + 1, arr + high);
    return i + 1;
}

void
quicksort(int arr[], int low, int high)
{
    if (low < high) {
        int ix = partition(arr, low, high);
        quicksort(arr, low, ix - 1);
        quicksort(arr, ix + 1, high);
    }
}

int
main(void)
{
    srand(time(NULL));
    const int len = 512;
    int input[len];

    for (int i = 0; i < len; ++i)
        input[i] = rand() % 1024;

    quicksort(input, 0, len - 1);
    
    for (int i = 0; i < len - 1; ++i)
        assert(input[i] <= input[i + 1]);
    
    exit(0);
}
