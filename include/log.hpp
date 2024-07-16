#ifndef LOG_HPP
#define LOG_HPP

//#define DEBUG

#ifdef DEBUG
#    define LOG(msg) std::cerr << msg << std::endl
#else
#    define LOG(msg) do { } while(0);
#endif

#endif // LOG_HPP