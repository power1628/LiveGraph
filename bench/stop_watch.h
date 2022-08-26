#include <chrono>
#include <sys/time.h>
class StopWatch
{
public:
    StopWatch() { start_ = std::chrono::system_clock::now(); }
    float elapsed_sec()
    {
        auto end = std::chrono::system_clock::now();
        return static_cast<float>(std::chrono::duration_cast<std::chrono::seconds>(end - start_).count());
    }

    float elapsed_milli_sec()
    {
        auto end = std::chrono::system_clock::now();
        return static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count());
    }

    float elapsed_micro_sec()
    {
        auto end = std::chrono::system_clock::now();
        return static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count());
    }

    void reset() { start_ = std::chrono::system_clock::now(); }

    void record_sec()
    {

        auto end = std::chrono::system_clock::now();
        auto elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::seconds>(end - start_).count());
        records_.push_back(elapsed);
    }

    void record_micro_sec()
    {

        auto end = std::chrono::system_clock::now();
        auto elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count());
        records_.push_back(elapsed);
    }

    void record_milli_sec()
    {

        auto end = std::chrono::system_clock::now();
        auto elapsed = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count());
        records_.push_back(elapsed);
    }

    std::string report()
    {
        std::string res = "[";
        for (int i = 0; i < records_.size(); ++i)
        {
            res += std::to_string(records_[i]);
            if (i != records_.size() - 1)
            {
                res += ", ";
            }
        }
        res += "]";
        return res;
    }

private:
    std::chrono::time_point<std::chrono::system_clock> start_;
    std::vector<float> records_;
};

class StopWatchLite
{
public:
    StopWatchLite() { reset(); }

    double elapsed_milli_sec()
    {
        struct timeval end;
        assert(gettimeofday(&end, NULL));
        return double(end.tv_sec - start_.tv_sec) * 1000.0 + double(end.tv_usec - start_.tv_usec) / 1000.0;
    }

    double elapsed_micro_sec()
    {
        struct timeval end;
        assert(gettimeofday(&end, NULL));
        return double(end.tv_sec - start_.tv_sec) * 1000000.0 + double(end.tv_usec - start_.tv_usec);
    }

    void reset() { assert(gettimeofday(&start_, NULL)); }

private:
    struct timeval start_;
};