#include "fm_radio_s_kernel.cpp"
#include "../drivers/test_driver.hpp"
#include <valgrind/callgrind.h>
#include <iostream>
#include <iomanip>

#define PRINT 1

using namespace std;

struct fm_radio_printer : public fm_radio::state<fm_radio_printer>
{
    fm_radio_printer() { io = this; }

    void output(float * data)
    {
        cout << std::showpoint << std::setprecision(15) << (double) *data << endl;
    }
    void output(double * data)
    {
        cout << std::showpoint << std::setprecision(15) << (double) *data << endl;
    }
};

struct fm_radio_test
{
    typedef fm_radio::state<fm_radio_test> kernel_t;

    kernel_t * kernel;
    volatile float dummy_f;
    volatile double dummy_d;

    void initialize()
    {
        kernel = new kernel_t;
        kernel->io = this;
        kernel->initialize();
    }
    void run()
    {
        CALLGRIND_TOGGLE_COLLECT;
        for (int i = 0; i < 500; ++i)
            kernel->process();
        CALLGRIND_TOGGLE_COLLECT;
    }
    void output(float * data)
    {
        dummy_f = *data;
    }
    void output(double * data)
    {
        dummy_d = *data;
    }
};

int main()
{
#if PRINT

    auto p = new fm_radio_printer;
    cout << "initializing" << endl;
    p->initialize();
    for (int i = 0; i < 10; ++i)
    {
        cout << "processing" << endl;
        p->process();
    }

#else // PRINT

    auto test = new fm_radio_test;

#if 0
    test->initialize();
    test->run();
#endif
#if 1
    for (int i = 0; i < 1000; ++i)
    {
        test->initialize();
        test->run();
    }
#endif
#if 0
    test_driver<fm_radio_test> driver;
    driver.go(*test, 3, 1000);
#endif

#endif // PRINT

}
