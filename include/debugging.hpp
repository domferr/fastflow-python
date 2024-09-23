#ifndef LOG_HPP
#define LOG_HPP

#define DEBUG

#ifdef DEBUG
#    define LOG(msg) std::cerr << msg << std::endl
#else
#    define LOG(msg) do { } while(0)
#endif

#ifdef DEBUG
#    define TIMESTART(timename) auto timename = std::chrono::system_clock::now()
#else
#    define TIMESTART(msg) do { } while(0)
#endif

#ifdef DEBUG
#    define LOGELAPSED(msg, timestart) do { auto elapsed_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - timestart).count(); LOG(msg << elapsed_time_ms << "ms"); } while(0)
#else
#    define LOGELAPSED(msg, timestart) do { } while(0)
#endif

#endif // LOG_HPP