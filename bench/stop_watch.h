#include <chrono>
class StopWatch
{
public:
    StopWatch() { start_ = std::chrono::system_clock::now(); }
    float elapsed_sec()
    {
        auto end = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::seconds>(end - start_).count();
    }

    float elapsed_milli_sec()
    {
        auto end = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
    }

    float elapsed_micro_sec()
    {
        auto end = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
    }

    void reset() { start_ = std::chrono::system_clock::now(); }

    void record_sec()
    {

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(end - start_).count();
        records_.push_back(elapsed);
    }

    void record_micro_sec()
    {

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
        records_.push_back(elapsed);
    }

    void record_milli_sec()
    {

        auto end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start_).count();
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