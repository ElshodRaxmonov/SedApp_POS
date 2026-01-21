#include <bits/stdc++.h>
using namespace std;


int makePalindrome(int h) {
    int tens = h / 10;
    int ones = h % 10;
    return ones * 10 + tens;   
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(NULL);

    string clock;
    if (!(cin >> clock)) return 0;

    int hours, minutes;
    sscanf(clock.c_str(), "%d:%d", &hours, &minutes);

    int minutesSleep = 0;
    while (true) {
        int pal = makePalindrome(hours);
        if (pal == minutes) break;

      
        minutes++;
        minutesSleep++;
        if (minutes == 60) {
            minutes = 0;
            hours++;
            if (hours == 24) hours = 0;
        }
    }

    cout << minutesSleep << '\n';
    return 0;
}
