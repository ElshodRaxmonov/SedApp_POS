#include <iostream>
#include <algorithm>
using namespace std;

int firstOccurrence(int a[], int size, bool first, int theNumber)
{
    int left = 0, right = size - 1, mid;
    int result;
    while (left <= right)
    {
        mid = (left + right) / 2;
        if (theNumber == a[mid])
        {
            result = mid;
            (first) ? right = mid - 1 : left + 1;
        }
        else if (theNumber < a[mid])
        {
            right = mid - 1;
        }
        else
        {
            left = mid + 1;
        }
    }
    return result;
}
int main(int argc, char const *argv[])
{
    int a[] = {1, 1, 1, 3, 3, 3, 3, 3, 4, 4, 4, 2, 2, 2, 4, 4, 5, 5, 6, 6};
    ios_base::sync_with_stdio(false);

    int size = sizeof(a) / sizeof(a[0]);
    sort(a, a + size);
    cout << firstOccurrence(a, size, 1, 4);
    return 0;
}
