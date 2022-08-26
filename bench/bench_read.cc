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

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

DEFINE_int32(num_task, 1, "number of read task");
DEFINE_int64(num_v, 0, "number of vertices to read");

using namespace livegraph;

struct Metric
{
    prometheus::Histogram &latency_histo;
    prometheus::Gauge &edge_counter;
    prometheus::Counter &query_counter;
    Metric(prometheus::Histogram &_latency_histo, prometheus::Gauge &_edge_counter, prometheus::Counter &_query_counter)
        : latency_histo(_latency_histo), edge_counter(_edge_counter), query_counter(_query_counter)
    {
    }
};

static prometheus::Histogram::BucketBoundaries
CreateLinearBuckets(std::int64_t start, std::int64_t end, std::int64_t step)
{
    auto bucket_boundaries = prometheus::Histogram::BucketBoundaries{};
    for (auto i = start; i < end; i += step)
    {
        bucket_boundaries.push_back(i);
    }
    return bucket_boundaries;
}

void read_graph(Graph *g, label_t label, int num_task, vertex_t num_v, Metric *metric)
{
    std::atomic_uint64_t query_cnt;
    StopWatch watch;
    {
        std::thread tasks[num_task];
        vertex_t avg = (num_v + num_task - 1) / num_task;
        for (int i = 0; i < num_task; ++i)
        {
            vertex_t begin = avg * i;
            vertex_t end = std::min(avg * (i + 1), num_v);
            tasks[i] = std::thread(
                [g, label, metric](int task_id, vertex_t begin, vertex_t end)
                {
                    LOG(INFO) << " task " << task_id << " [" << begin << ", " << end << " )";
                    for (vertex_t vid = begin; vid < end; ++vid)
                    {
                        vertex_t edge_cnt = 0;
                        StopWatchLite watch;

                        auto txn = g->begin_read_only_transaction();
                        auto iter = txn.get_edges(vid, label, false);
                        while (iter.valid())
                        {
                            vertex_t dst = iter.dst_id();
                            iter.next();
                            edge_cnt += 1;
                        }
                        double elapsed = watch.elapsed_micro_sec();
                        // report
                        metric->edge_counter.Increment(double(edge_cnt));
                        metric->latency_histo.Observe(elapsed);
                        metric->query_counter.Increment();
                        // LOG(INFO) << " task " << task_id << " vid " << vid << " elapsed " << elapsed << " edge " <<
                        // edge_cnt;
                    }
                },
                i, begin, end);
        }

        for (int i = 0; i < num_task; ++i)
        {
            tasks[i].join();
        }
    }

    float cost = watch.elapsed_sec();
    LOG(INFO) << "qps: " << static_cast<float>(query_cnt.load()) / cost;
}

int main(int argc, char **argv)
{
    SetupStacktraceSignal();
    FLAGS_logtostderr = 1;
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    gflags::ShowUsageWithFlagsRestrict(argv[0], "bench_read");

    // setup prometheus
    prometheus::Exposer exposer{"0.0.0.0:9100"};
    auto registry = std::make_shared<prometheus::Registry>();

    auto &query_counter =
        prometheus::BuildCounter().Name("query").Help("queries processed").Register(*registry).Add({});

    auto &latency_histo = prometheus::BuildHistogram()
                              .Name("latency")
                              .Help("query latency in us")
                              .Register(*registry)
                              .Add({}, CreateLinearBuckets(0, 1000000, 10));

    auto &edge_counter =
        prometheus::BuildGauge().Name("edge_processed").Help("edges processed").Register(*registry).Add({});
    exposer.RegisterCollectable(registry);

    Metric metric(latency_histo, edge_counter, query_counter);

    // bench read
    std::string db_path = "./db_block";
    std::string wal_path = "./db_wal";
    label_t label = 1;
    int num_task = FLAGS_num_task;
    vertex_t num_v = FLAGS_num_v;

    Graph *g = new Graph(db_path, wal_path);
    {
        StopWatch watch;
        read_graph(g, label, num_task, num_v, &metric);
        LOG(INFO) << "read_graph cost(sec) : " << watch.elapsed_sec();
    }

    delete g;
    sleep(100);
    return 0;
}
