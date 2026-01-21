#include <vector>
#include <iostream>
#include <algorithm>
using namespace std;
int majorityElement(const std::vector<int> &nums)
{
    int count = 0;
    int candidate = 0;

    for (int num : nums)
    {
        if (count == 0)
        {
            candidate = num;
            count = 1;
        }
        else if (num == candidate)
        {
            count++;
        }
        else
        {
            count--;
        }
    }

    return candidate;
}

int majorityElementSecond(vector<int> nums)
{
    int n = nums.size();
    sort(nums.begin(), nums.end());

    int candidate = nums[n / 2];

    int count = 0;
    for (int num : nums)
    {
        if (num == candidate)
        {
            count++;
        }
    }

    if (count > n / 2)
    {
        return candidate;
    }

   
    return -1;
}
int main()
{
    // std::vector<int> arr1 = {3, 2, 3};
    // std::cout << "Majority element in arr1: " << majorityElement(arr1) << std::endl;

    // std::vector<int> arr2 = {2, 2, 1, 1, 1, 2, 2};
    // std::cout << "Majority element in arr2: " << majorityElement(arr2) << std::endl; // Output: 2

    std::vector<int> arr3 = {2, 2, 2, 2, 4, 4, 4, 4, 5, 6, 7, 8, 8, 8, 8, 1, 1, 1, 1, 1, 1, 1};
    std::cout << "Majority element in arr3: " << majorityElementSecond(arr3) << std::endl;

    return 0;
}
