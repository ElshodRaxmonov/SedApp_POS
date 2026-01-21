#include <iostream>
#include <vector>
#include <string>
#include <algorithm> // For std::max and std::search

// Size of the alphabet (ASCII characters)
#define NO_OF_CHARS 256

// Function to create the bad character table
void badCharHeuristic(const std::string& pattern, int patternLength, std::vector<int>& badchar) {
    // Initialize all occurrences as -1
    for (int i = 0; i < NO_OF_CHARS; i++) {
        badchar[i] = -1;
    }

    // Fill the actual value of last occurrence of a character
    for (int i = 0; i < patternLength; i++) {
        badchar[(int)pattern[i]] = i;
    }
}

// Function to search for the pattern in the text
void search(const std::string& text, const std::string& pattern) {
    int textLength = text.length();
    int patternLength = pattern.length();
    std::vector<int> badchar(NO_OF_CHARS);

    badCharHeuristic(pattern, patternLength, badchar);

    int s = 0; // s is shift of the pattern with respect to text
    while (s <= (textLength - patternLength)) {
        int j = patternLength - 1;

        // Keep reducing index j of pattern while characters of pattern and text are matching
        while (j >= 0 && pattern[j] == text[s + j]) {
            j--;
        }

        // If the pattern is present at current shift, then index j will become -1
        if (j < 0) {
            std::cout << "Pattern found at index " << s << std::endl;

            // Shift the pattern so that the next character in text aligns with its last occurrence in pattern
            s += (s + patternLength < textLength) ? patternLength - badchar[(int)text[s + patternLength]] : 1;
        } else {
            // Mismatch occurred. Use bad character heuristic to find the next shift.
            // The maximum of 1 and the shift suggested by the bad character rule is used
            // to ensure a positive shift.
            s += std::max(1, j - badchar[(int)text[s + j]]);
        }
    }
}

int main() {
    std::string text = "ABAAABCD";
    std::string pattern = "ABC";
    search(text, pattern);
    return 0;
}
