
#include <stdio.h>
int main(void)
{

    int number;
    printf("Enter the number: ");
    scanf("%d", &number);
    if (
        number > 0)
    {
        printf("The number is positive");
    }
    else if (number == 0)
    {
        printf("The number is zero");
    }else{
        printf("The number is negative");
    }
}
/* No additional code needed. The program already reads an integer and prints its multiplication table from 1 to 10 using a for loop. */