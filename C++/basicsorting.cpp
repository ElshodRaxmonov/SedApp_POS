#include <iostream>
#include <bits/stdc++.h>
using namespace std;

int main()
{

    int a[] = {23,34,45,24,25,12,67};
    int n = sizeof(a) / sizeof(a[0]);
    for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j)
            if (a[i] < a[j])
                swap(a[i], a[j]);

    return 0;
}