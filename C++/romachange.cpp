#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, k;
    cin >> n >> k;

    vector<int> a(n);
    for (int i = 0; i < n; i++) {
        cin >> a[i];
    }

    for (int i = 0; i < n && k > 0 && a[i] < 0; i++) {
        a[i] = -a[i];
        k--;
    }

    if (k % 2 == 1) {
        int idx = min_element(a.begin(), a.end()) - a.begin();
        a[idx] = -a[idx];
    }

    long long sum = 0;
    for (int x : a) sum += x;

    cout << sum << '\n';
    return 0;
}