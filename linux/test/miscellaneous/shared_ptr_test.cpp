#include <string>
#include <memory>
#include <iostream>
#include <unistd.h>
#include <map>

using namespace std;

class HTTP
{
public:
    HTTP()
    {
        cout << "HTTP constructor" << endl;
    }

    ~HTTP()
    {
        cout << "HTTP destructor" << endl;
    }
};

int main(void)
{
    int fd = 5;
    map<int, shared_ptr<HTTP>> test;
    test[fd] = make_shared<HTTP>();
    cout << "test size: " << test.size() << endl;
    // cout << close(fd) << endl;
    test[fd] = make_shared<HTTP>();
    // test.erase(fd);
    cout << "test size: " << test.size() << endl;
    return 0;
}