#include <iostream>

using namespace std;

int main(void)
{
    int test[100];
    cout << sizeof(int) << endl;
    cout << sizeof(double) << endl;
    cout << sizeof(test) << endl;
    cout << (test == &test[0]) << endl;
    return 0;
}