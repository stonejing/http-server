#include <stdio.h>
#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <unistd.h>
#include <sys/time.h>

struct Data {
    double x;
    double y;
    int    a;
    Data(double x_, double y_, int a):x(x_), y(y_), a(a){};
};

std::vector<Data*> MakeData(int num) {
    std::vector<Data*> vec;
    for (int i=0; i< num; i++) {
        Data data(i + 0.5, i + 1.5, i);
        // data->x = static_cast<double>(i) + 0.5;
        // data->y = static_cast<double>(i) + 1.5;
        // data->a = i;
        vec.push_back(&data);
    }
    return vec;
}

double Cal(std::vector<Data*>& data, int idx, int num) {
    double sum1 = 0.0;
    double sum2 = 0.0;
    for (int i = idx - num + 1; i <= idx; i++) {
        if (i < 0) {
            continue;
        }
        auto & elem = data[i];
        sum1 += elem->x;
        sum2 += elem->y;
    }

    auto avg1 = sum1 / num;
    auto avg2 = sum2 / num;

    return (avg1 + avg2) / 2;
}

void Make(std::vector<Data*> & data) {
    std::vector<double> target;
    // target.reserve(data.size());
    for (int i = 0; i < data.size(); i++) {
        auto v = Cal(data, i, 1000);
        if (v > 1000) {
            target.push_back(v);
        }
    }
    std::cout << "target size:" << target.size() << std::endl;
}
            
int main(int argc,char** argv)
{
    struct timeval t1;
    struct timeval t2;
    gettimeofday(&t1, NULL);
    auto data = MakeData(300*365*5);
    Make(data);
    gettimeofday(&t2, NULL);
    auto usetime = double((t2.tv_sec*1000000 + t2.tv_usec) - (t1.tv_sec*1000000 + t1.tv_usec))/1000000;
    std::cout <<"use time: " << usetime << std::endl;
}