#include <iostream>
using namespace std;

int main()
{

    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    int first, second;
    cin >> first >> second;

    bool firstLine[10] = {}, secondLine[10] = {};
    for (int i = 0, x; i < first; i++)
        cin >> x, firstLine[x] = true;
    for (int i = 0, x; i < second; i++)
        cin >> x, secondLine[x] = true;

    for (int d = 1; d <= 9; d++)
    {
        if (firstLine[d] && secondLine[d])
        {
            cout << d << '\n';
            return 0;
        }
    }

    int minA = 10, minB = 10;
    for (int d = 1; d <= 9; d++)
    {
        if (firstLine[d])
            minA = min(minA, d);
        if (secondLine[d])
            minB = min(minB, d);
    }

    cout << min(minA * 10 + minB, minB * 10 + minA) << '\n';
    return 0;
}
