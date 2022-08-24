
#include <cstdio>
#include <map>
#include <mutex>
#include <random>
#include <string>

#include <omp.h>

#include "bind/livegraph.hpp"
#include "core/livegraph.hpp"

#include <filesystem>
#include <fstream>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

DEFINE_string(csv, "", "path to graph to load");
DEFINE_int32(num_load_task, 0, "number of load tasks");
DEFINE_int64(num_v, 0, "number of vertices");

using namespace livegraph;

class ThreadSafeDeque
{
public:
    ThreadSafeDeque(const std::string &path)
    {
        std::scoped_lock lock(mu_);
        std::cout << "dir: " << path << std::endl;
        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            files_.push_back(entry.path());
            std::cout << " - " << entry.path() << std::endl;
        }
    }

    std::tuple<bool, std::string> pop()
    {
        std::scoped_lock lock(mu_);
        bool empty = files_.empty();
        std::string file;
        if (!empty)
        {
            file = files_.back();
            files_.pop_back();
        }
        return std::tuple(empty, file);
    }

private:
    std::deque<std::string> files_;
    std::mutex mu_;
};

class CsvFileIterator
{
public:
    using Item = std::tuple<vertex_t, vertex_t>;
    CsvFileIterator(ThreadSafeDeque *files) : files_(files) { std::string file = files->pop(); }

    std::tuple<bool, Item> next() {}

private:
    struct State
    {
        bool empty_;
        std::ifstream fstream_;
        std::string file_;

        State(std::string file)
        {
            file_ = file;
            fstream_ = std::ifstream(file);
        }

        std::tuple<bool, Item> next()
        {
            bool has_next = false;
            Item item;
            std::string line;
            auto res = std::getline(fstream_, line);
            if (res.good())
            {
                has_next = true;
                // parse
                vertex_t from, to;
                std::istringstream iss(line);
                if (!(iss >> from >> to))
                {
                    LOG(FATAL) << "bad parsing line " << line << " file " << file_;
                }
                item = std::move(std::tuple(from, to));
            }
            else if (res.eof())
            {
                has_next = false;
            }
            else if (res.fail())
            {
                LOG(FATAL) << "read file " << file_ << " fail";
            }
            else if (res.bad())
            {
                LOG(FATAL) << "read file " << file_ << " bad";
            }
            else
            {
                LOG(FATAL) << "read file " << file_ << " invalid code ";
            }

            return std::tuple(has_next, std::move(item));
        }
    };

    std::string dir_;
    ThreadSafeDeque *files_;
};

void load_graph(const std::string &data_path, int num_load_task, vertex_t num_v, Graph *graph) {}

int main(int argc, char **argv)
{
    // create graph
    std::string db_path = "./db_block";
    std::string wal_path = "./db_wal";
    size_t max_block_size = 128 * 1024 * 1024; // 128 MB
    std::string csv = FLAGS_csv;
    int num_load_task = FLAGS_num_load_task;
    vertex_t num_v = static_cast<vertex_t>(FLAGS_num_v);

    Graph *g = new Graph(db_path, wal_path, max_block_size);
    load_graph(csv, num_load_task, num_v, g);
    return 0;
}