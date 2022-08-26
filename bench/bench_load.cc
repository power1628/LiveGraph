
#include <cstdio>
#include <map>
#include <mutex>
#include <random>
#include <string>

#include <omp.h>

#include "bind/livegraph.hpp"
#include "core/livegraph.hpp"

#include "./stacktrace.h"
#include "./stop_watch.h"
#include <filesystem>
#include <fstream>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
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
    CsvFileIterator(ThreadSafeDeque *files) : files_(files)
    {
        state_ = State();
        state_.next_file(files_);
    }

    std::tuple<bool, Item> next()
    {
        if (state_.empty())
        {
            LOG(INFO) << "iter done" << std::endl;
            return std::tuple(false, std::tuple(0, 0));
        }
        else
        {
            bool has_next;
            Item item;
            tie(has_next, item) = state_.next();
            if (has_next)
            {
                return std::tuple(has_next, item);
            }
            else
            {
                // next_state
                state_.next_file(files_);
                if (state_.empty())
                {
                    // eof
                    return std::tuple(false, item);
                }
                else
                {
                    return state_.next();
                }
            }
        }
    }

private:
    struct State
    {
        bool empty_;
        std::ifstream fstream_;
        std::string file_;

        State() { empty_ = true; }

        bool next_file(ThreadSafeDeque *files)
        {
            tie(empty_, file_) = files->pop();
            if (!empty_)
            {
                fstream_ = std::ifstream(file_);
            }
            return empty_;
        }

        bool empty() { return empty_; }

        std::tuple<bool, Item> next()
        {
            bool has_next = false;
            Item item;
            std::string line;
            auto &res = std::getline(fstream_, line);
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
    State state_;
};

void load_graph(const std::string &data_path, int num_load_task, vertex_t num_v, label_t label, Graph *graph)
{
    // create vertex
    {
        StopWatch watch;
        std::thread tasks[num_load_task];

        vertex_t num_avg = (num_v + num_load_task - 1) / num_load_task;
        auto txn = graph->begin_batch_loader();

        for (int i = 0; i < num_load_task; ++i)
        {
            vertex_t begin = num_avg * (i);
            vertex_t end = std::min((num_avg) * (i + 1), num_v + 1);
            tasks[i] = std::thread(
                [&](int task_id, vertex_t begin, vertex_t end)
                {
                    LOG(INFO) << "[" << begin << ", " << end << ")";
                    for (vertex_t vid = begin; vid < end; ++vid)
                    {
                        vertex_t id = txn.new_vertex();
                        std::string data = "";
                        txn.put_vertex(id, data);
                    }
                },
                i, begin, end);
        }

        for (int i = 0; i < num_load_task; ++i)
        {
            tasks[i].join();
        }
        watch.record_sec();

        txn.commit();
        watch.record_sec();
        LOG(INFO) << "create vertex : " << num_v << " cost(sec) : " << watch.report();
    }

    // load edge
    {
        StopWatch watch;
        std::atomic_uint64_t num_edges;
        ThreadSafeDeque files(data_path);
        std::thread tasks[num_load_task];
        auto txn = graph->begin_batch_loader();
        for (int i = 0; i < num_load_task; ++i)
        {
            CsvFileIterator file_iter(&files);
            tasks[i] = std::thread(
                [file_iter = std::move(file_iter), &txn, &label, &num_edges](int task_id) mutable
                {
                    bool has_next;
                    CsvFileIterator::Item item;
                    while (true)
                    {
                        std::tie(has_next, item) = file_iter.next();
                        if (!has_next)
                        {
                            LOG(INFO) << "task-" << task_id << " done";
                            break;
                        }
                        else
                        {
                            vertex_t from, to;
                            std::tie(from, to) = item;
                            std::string data = "";
                            txn.put_edge(from, label, to, data);
                            num_edges.fetch_add(1);
                        }
                    }
                },
                i);
        }

        for (int i = 0; i < num_load_task; ++i)
        {
            tasks[i].join();
        }

        watch.record_sec();

        txn.commit();
        watch.record_sec();
        LOG(INFO) << "load edge : " << num_edges.load() << " cost(sec) : " << watch.report();

    } // end of load edge
}

int main(int argc, char **argv)
{
    SetupStacktraceSignal();
    FLAGS_logtostderr = 1;
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    gflags::ShowUsageWithFlagsRestrict(argv[0], "bench_load");

    // create graph
    std::string db_path = "./db_block";
    std::string wal_path = "./db_wal";
    label_t label = 1;
    std::string csv = FLAGS_csv;
    int num_load_task = FLAGS_num_load_task;
    vertex_t num_v = static_cast<vertex_t>(FLAGS_num_v);

    Graph *g = new Graph(db_path, wal_path);
    {
        StopWatch watch;
        load_graph(csv, num_load_task, num_v, label, g);
        LOG(INFO) << "load_graph cost(sec) : " << watch.elapsed_sec();
    }
    // compact
    {
        StopWatch watch;
        g->compact();
        LOG(INFO) << "compact cost(sec) : " << watch.elapsed_sec();
    }

    delete g;
    return 0;
}