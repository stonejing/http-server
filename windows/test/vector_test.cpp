#include <iostream>
#include <vector>

using namespace std;

int main(void)
{
    std::vector<int> c(1000);

    std::cout << c.size() << std::endl;

    while(true)
    {
        if(!c.empty())
        {
            continue;
        }
    }

    return 0;
}