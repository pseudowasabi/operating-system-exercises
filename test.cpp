#include <iostream>
#include <string>
using namespace std;

class Test {
public:
    unsigned int something;
};

int main() {
    Test *test0 = new Test;
    size_t test0_ull = (size_t)test0;
    test0->something = 4;
    
    cout << test0_ull << endl;

    Test* test1 = (Test*)test0_ull;
    test1->something = 5;
    cout << test0->something << endl;

    cout << sizeof(size_t) << endl;
    cout << sizeof(unsigned int) << endl;

    cout << sizeof(unsigned long) << endl;

    cout << sizeof(unsigned long long) << endl;
    return 0;
}