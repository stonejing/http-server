#include <iostream>

class Test
{
public:
    Test()
    {
        std::cout << "print test()." << std::endl;
    }
};

int main(void)
{
    auto a = new Test[3];
    std::cout << "init completed." << std::endl;
    return 0;
}