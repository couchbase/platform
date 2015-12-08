#include <iostream>
#include <platform/platform.h>
#ifdef _MSC_VER
#include <time.h>
#else
#include <sys/time.h>
#endif

int main(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        std::cerr << "gettimeofday returned != 0" << std::endl;
        return 1;
    }

        time_t now = time(NULL);
        if (tv.tv_sec > now) {
            std::cerr << "gettimeofday returned a date in the future " << std::endl
                << "time returns " << now << " and tv_sec " << tv.tv_sec << " tv_usec " << tv.tv_usec
                << std::endl;
            return 1;
        }

        if (now != tv.tv_sec && now != (tv.tv_sec + 1)) {
            // the test shouldn't take more than a sec to run, so it should be
            std::cerr << "gettimeofday returned a date too long ago " << std::endl
                << "time returns " << now << " and tv_sec " << tv.tv_sec << " tv_usec " << tv.tv_usec
                << std::endl;
            return 1;
        }

   return 0;
}
