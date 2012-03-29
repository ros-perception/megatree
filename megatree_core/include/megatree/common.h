// Useful utility functions that don't belong anywhere else
#ifndef MEGATREE_COMMON_H
#define MEGATREE_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <boost/date_time/posix_time/posix_time_types.hpp>

namespace megatree {

int parseNumberSuffixed(const char* s);

  class Tictoc
  {
  public:
    Tictoc() : started(boost::posix_time::microsec_clock::universal_time()) {}

    float tic()
    {
      boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
      long m = (now - started).total_milliseconds();
      started = now;
      return float(m) / 1000.0f;
    }

    float toc()
    {
      boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
      long m = (now - started).total_milliseconds();
      return float(m) / 1000.0f;
    }
    boost::posix_time::ptime started;
  };

  class PausingTimer
  {
  public:
    PausingTimer() : started(boost::posix_time::microsec_clock::universal_time()), elapsed(0,0,0,0) {}

    void pause()
    {
      boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
      elapsed += (now - started);
    }

    void start()  // Continues timing
    {
      started = boost::posix_time::microsec_clock::universal_time();
    }

    float getElapsed() const
    {
      return elapsed.total_milliseconds() / 1000.0f;
    }
    void reset()
    {
      elapsed = boost::posix_time::time_duration(0,0,0,0);
    }

    boost::posix_time::ptime started;
    boost::posix_time::time_duration elapsed;
  };

}

class AssertionException : std::exception
{
public:
  AssertionException(const char* _file, long _line, const char* _pretty_function, const char* _message)
    : file(_file), line(_line), pretty_function(_pretty_function), message(_message)
  {
    fprintf(stderr, "%s:%ld  %s: %s\n", file, line, pretty_function, message);
  }
  AssertionException(const AssertionException &o)
    : file(o.file), line(o.line), message(o.message)  {}
  virtual ~AssertionException() throw() {}

  virtual const char* what() const throw()
  {
    try {
      std::stringstream ss;
      ss << file << ":" << line << "  " << pretty_function << ": " << message;
      __internal = ss.str();
      return __internal.c_str();
    }
    catch (...)
    {
      return message;
    }
  }

  const char* file;
  long line;
  const char* pretty_function;
  const char* message;
  mutable std::string __internal;
};

#define Assert(cond)                                                   \
  ((cond)                                                              \
   ? (void)(0)                                                         \
   : throw AssertionException(__FILE__, __LINE__, __PRETTY_FUNCTION__, \
                              "Assertion failed: " #cond))

#endif
