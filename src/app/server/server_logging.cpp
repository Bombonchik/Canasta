#include "server/server_logging.hpp"

#include <cstdlib>
#include <filesystem>
#include <spdlog/async.h>
#include <spdlog/async_logger.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/details/log_msg.h>

namespace spdlog {
    namespace sinks {
    
    template<typename WrappedSink, typename Mutex = std::mutex>
    class level_range_filter_sink final : public base_sink<Mutex> {
    public:
        level_range_filter_sink(
            std::shared_ptr<WrappedSink> wrapped,
            level::level_enum           min_level,
            level::level_enum           max_level
        )
        : wrapped_(std::move(wrapped)), min_(min_level), max_(max_level) {}
    
    protected:
        void sink_it_(const details::log_msg &msg) override {
            if (msg.level >= min_ && msg.level <= max_)
                wrapped_->log(msg);
        }
        void flush_() override {
            wrapped_->flush();
        }
    
    private:
        std::shared_ptr<WrappedSink> wrapped_;
        level::level_enum            min_, max_;
    };
    
    } // namespace sinks
} // namespace spdlog

void initLogger()
{
    namespace fs = std::filesystem;
    namespace lvl = spdlog::level;
    
    // Create a directory for logs if it doesn't exist
    fs::path logDir;
    if (auto* env = std::getenv("LOG_DIR")) logDir = env;
    else                                   logDir = fs::current_path().parent_path().parent_path() / "logs";
    fs::create_directories(logDir);

    spdlog::init_thread_pool(8192, 1);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    //console_sink->set_level(lvl::info);
    console_sink->set_level(lvl::debug);

    auto raw_info = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        (logDir / "server.log").string(), false);
    auto raw_err  = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            (logDir / "error.log").string(), false);

    auto info_sink = std::make_shared<
        spdlog::sinks::level_range_filter_sink<
            spdlog::sinks::basic_file_sink_mt
        >
    //>(raw_info, lvl::info, lvl::warn);
    >(raw_info, lvl::debug, lvl::warn);

    auto err_sink = std::make_shared<
        spdlog::sinks::level_range_filter_sink<
            spdlog::sinks::basic_file_sink_mt
        >
    >(raw_err, lvl::err, lvl::critical);

    // one async logger that fans out to all sinks
    auto logger = std::make_shared<spdlog::async_logger>(
        "server", spdlog::sinks_init_list{console_sink, info_sink, err_sink},
        spdlog::thread_pool(), spdlog::async_overflow_policy::block
    );
    logger->set_level(lvl::debug);
    spdlog::set_default_logger(logger);
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] %v");
    spdlog::flush_every(std::chrono::seconds(2));
}