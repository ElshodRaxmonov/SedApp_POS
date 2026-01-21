#include <bits/stdc++.h>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int q;
    cin >> q;

    while (q--) {
        int n;
        cin >> n;

        if (n < 4) {
            cout << -1 << '\n'; 
        } 
        else if (n == 4 || n == 6) {
            cout << 1 << '\n'; 
        } 
        else if (n == 5 || n == 7 || n == 9 || n == 11) {
            cout << -1 << '\n';
        } 
        else {
            
            int count;
            if (n % 2 == 0) {
                count = n / 4; 
            } else {
               
                count = (n - 9) / 4 + 1; 
            }
            cout << count << '\n';
        }
    }

    return 0;
}
