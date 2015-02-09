#ifndef LOGGER_H
#define	LOGGER_H

#include <sys/time.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <execinfo.h> // for backtrace
#include <dlfcn.h> // for dladdr
#include <cxxabi.h> // for __cxa_demangle

#include <iostream>
#include <sstream>
#include <iosfwd>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <boost/version.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <boost/interprocess/detail/atomic.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>

typedef boost::function<bool(int /*facility*/, int /*level*/, const std::ostringstream& /*headers*/, std::string& /*message*/)> OsExternalLogger;

#define LOG_FORMAT_DELIMITERS           ", "
#define LOG_FILTER_VERBOSE              "verbose"
#define LOG_FILTER_DATE                 "date"
#define LOG_FILTER_COUNTER              "counter"
#define LOG_FILTER_FACILITY             "facility"
#define LOG_FILTER_PRIORITY             "priority"
#define LOG_FILTER_HOST_NAME            "hostName"
#define LOG_FILTER_THREAD_ID            "thread_id"
#define LOG_FILTER_PROCESS_NAME         "processName"
#define LOG_FILTER_TASK_NAME            "taskName"
#define LOG_FILTER_ESCAPE               "escape"
#define LOG_FILTER_NULL                 "nullFilter"
#define DEFAULT_LOG_FORMAT              LOG_FILTER_DATE LOG_FORMAT_DELIMITERS \
                                        LOG_FILTER_COUNTER LOG_FORMAT_DELIMITERS \
                                        LOG_FILTER_FACILITY LOG_FORMAT_DELIMITERS \
                                        LOG_FILTER_PRIORITY LOG_FORMAT_DELIMITERS \
                                        LOG_FILTER_HOST_NAME LOG_FORMAT_DELIMITERS \
                                        LOG_FILTER_THREAD_ID LOG_FORMAT_DELIMITERS \
                                        LOG_FILTER_PROCESS_NAME LOG_FORMAT_DELIMITERS

enum tagOsSysLogFacility
{
   FAC_PERF=0,             ///< performance related
   FAC_KERNEL,             ///< kernel/os related
   FAC_AUTH,               ///< authentication/security related
   FAC_NET,                ///< networking related
   FAC_RTP,                ///< RTP/RTCP related
   FAC_PHONESET,           ///< phoneset related
   FAC_HTTP,               ///< http sever related
   FAC_SIP,                ///< sip related
   FAC_CP,                 ///< call processing related
   FAC_MP,                 ///< media processing related
   FAC_TAO,                ///< TAO related
   FAC_JNI,                ///< JNI Layer related
   FAC_JAVA,               ///< Java related
   FAC_LOG,                ///< OsSysLog related
   FAC_SUPERVISOR,         ///< sipXsupervisor
   FAC_SIP_OUTGOING,       ///< Outgoing SIP messages
   FAC_SIP_INCOMING,       ///< Incoming SIP messages
   FAC_SIP_INCOMING_PARSED,///< Incoming SIP messages after being parsed
   FAC_MEDIASERVER_CGI,    ///< Mediaserver CGIs
   FAC_MEDIASERVER_VXI,    ///< Mediaserver VXI engine
   FAC_ACD,                ///< ACD related
   FAC_PARK,               ///< Park Server related
   FAC_APACHE_AUTH,        ///< Apache Authentication Module
   FAC_UPGRADE,            ///< Update/Upgrade related
   FAC_LINE_MGR,           ///< SIP line manager related
   FAC_REFRESH_MGR,        ///< SIP refresh manager related
   FAC_UNIT_TEST,          ///< Available for re-use.
   FAC_STREAMING,          ///< Stream Media related message
   FAC_REPLICATION_CGI,    ///< replication cgi( replicates databases across components )
   FAC_DB,                 ///< Database related (sipdb)
   FAC_PROCESSMGR,         ///< OsProcessMgr
   FAC_PROCESS,            ///< process related
   FAC_SIPXTAPI,           ///< sipXtapi related
   FAC_AUDIO,              ///< audio related
   FAC_CONFERENCE,         ///< Conference bridge
   FAC_ODBC,               ///< ODBC related
   FAC_CDR,                ///< CDR generating related
   FAC_RLS,                ///< Resource list server
   FAC_XMLRPC,             ///< XML RPC related
   FAC_FSM,                ///< Finite State Machine tracking
   FAC_NAT,                ///< NAT Traversal related
   FAC_ALARM,              ///< Alarms
   FAC_SAA,                ///< Shared Appearance Agent
   FAC_MONGO_CLIENT,       ///< Mongo C++ Client Driver logs
   FAC_MAX_FACILITY        ///< Last Facility (used to for length)

   //
   // *** READ THIS ***
   //
   // NOTE:  If adding a facility, please:
   //        1) Insert it before FAC_MAX_FACILITY.
   //        2) Update OsSysLogFacilities.cpp to include the
   //           string name.
   //        3) Update the !enum comments above.
   //
   // *** READ THIS ***
   //
   //
} ;

typedef enum tagOsSysLogFacility OsSysLogFacility ;

/// The priority of log messages ordered from least to most severe
typedef enum tagOsSysLogPriority
{
   PRI_DEBUG,     /**< Developer message needed only when debugging code.
                   *   May include recording of such things as entry and exit from methods
                   *   or internal branch tracking.
                   *   Should never be required by end users.
                   */
   PRI_INFO,      /**< Informational message used to trace system inputs and actions.
                   *   Significant actions in the execution of some activity; for example, the
                   *   receipt of a message or an important decision in its disposition.
                   *   This level should be sufficient for an administrator or support person
                   *   to determine what occured in the system when debugging a configuration
                   *   or interoperability problem in the field.
                   */
   PRI_NOTICE,    /**< Normal but significant events.
                   *   Events that are expected but provide important context, such as service
                   *   restarts and reloading configuration files.
                   *   This is the default logging level, and generally logging should
                   *   always include at least these messages.
                   */
   PRI_WARNING,   /**< Conditions that imply that some failure is possible but not certain.
                   *   Generally, external inputs that are not as expected and possibly
                   *   invalid.  Especially useful in low level routines that are going to
                   *   return an error that may be recoverable by the caller.
                   */
   PRI_ERR,       /**< An unexpected condition likely to cause an end-user visibile failure.
                   *   This level should be used whenever an error response is being sent
                   *   outside the system to provide a record of the internal data that
                   *   are important to understanding it.
                   */
   PRI_CRIT,      /**< Endangers service operation beyond the current operation.
                   *   MUST be logged prior to any 'assert', or when exiting for any abnormal
                   *   reason.
                   */
   PRI_ALERT,     /**< Fault to be communicated to operations.
                   *   Should be replaced by usage of the new Alarm subsystem.
                   */
   PRI_EMERG,     /**< System is unusable.
                   */

   // NOTE: If adding/removing priorities, you MUST also adjust static name
   // initializer in OsSysLog.cpp

   SYSLOG_NUM_PRIORITIES ///< MUST BE LAST

} OsSysLogPriority;

namespace Os
{

  template <typename T>
  class LogFileChannelBase : boost::noncopyable
  {
  public:
    #define default_mode std::fstream::out | std::fstream::binary | std::fstream::app | std::fstream::ate

    LogFileChannelBase() :
      _mode(default_mode)
    {
    }

    LogFileChannelBase(const char* path, std::ios_base::openmode mode = default_mode)
    {
      open(path, mode);
    }

    LogFileChannelBase(const std::string& path, std::ios_base::openmode mode = default_mode)
    {
      open(path.c_str(), mode);
    }

    ~LogFileChannelBase()
    {
      _fstream.close();
    }

    std::streamsize read(char* s, std::streamsize n)
    {
        // Read up to n characters from the underlying data source
        // into the buffer s, returning the number of characters
        // read; return -1 to indicate EOF
      _fstream.get(s, n);
      return _fstream.gcount();
    }

    std::streamsize write(const char* s, std::streamsize n)
    {
     _fstream.write(s, n);
      return n;
    }

    void flush()
    {
      _fstream.flush();
    }

    std::streamsize seek(std::streamsize off, std::ios_base::seekdir way)
    {
      _fstream.seekg(off, way);
      return _fstream.tellg();
    }

    bool open(const char* path_, std::ios_base::openmode mode = default_mode)
    {
      _fstream.close();
      _path = std::string(path_);
      _mode = mode;
      _fstream.open(_path.c_str(), _mode);
      return _fstream.is_open();
    }

    std::streamsize size()
    {
      // get length of file:
      std::streamsize offset = _fstream.tellg();
      _fstream.seekg(0, std::ios::end);
      std::streamsize size = _fstream.tellg();
      _fstream.seekg(offset);
      return size;
    }

    void open(const std::string& path, std::ios_base::openmode mode = default_mode)
    {
      open(path.c_str(), mode);
    }

    bool is_open()
    {
      return _fstream.is_open();
    }

    void close()
    {
      _fstream.close();
    }

    bool auto_close()
    {
      return false;
    }

    const std::string& path() const
    {
      return _path;
    }

    bool erase()
    {
      _fstream.close();
      try
      {
        std::remove(_path.c_str());
      }
      catch(std::exception& e)
      {
        return false;
      }
      return true;
    }

    T& stream()
    {
      return _fstream;
    }
  private:
    std::string _path;
    T _fstream;
    std::ios_base::openmode _mode;
  };

  typedef LogFileChannelBase<std::fstream> LogFileChannel;

  //
  // filter from which all other filters derive
  //
  class LogBaseFilter
  {
  public:
    LogBaseFilter(const std::string& filterName) :
      _filterName(filterName),
      _enabled(false)
    {
    }

    ~LogBaseFilter()
    {
    }

    void enable(const std::set<std::string>& filters)
    {
      if(filters.find(_filterName) != filters.end())
      {
        _enabled = true;
      }
    }

  public:
    std::string _hostName;
    std::string _processName;
    std::string _filterName;
    bool _enabled;
  };

//
// This filter adds an atomic counter to the log header
//
  class LogCounterFilter : public LogBaseFilter
  {
  public:
    LogCounterFilter() :
      LogBaseFilter(LOG_FILTER_COUNTER),
      _counter(0)
    {
    }

    ~LogCounterFilter()
    {
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      if(_enabled)
      {
#if (BOOST_VERSION < 104800)
        // CentOS 6
        headers << boost::interprocess::detail::atomic_inc32(&_counter) + 1 << ":";
#else
        //Fedora 17 or greater
        headers << boost::interprocess::ipcdetail::atomic_inc32(&_counter) + 1 << ":";
#endif
        return true;
      }
      return true;
    }

  private:
    volatile boost::uint32_t _counter;

  };

  class LogTimeFilter : public LogBaseFilter
  {
  public:
    LogTimeFilter() :
      LogBaseFilter(LOG_FILTER_DATE)
    {
        _enabled = true;
    }

    ~LogTimeFilter()
    {
    }

    void enable(const std::set<std::string>& filters)
    {
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      if(_enabled)
      {
        struct timeval tv;
        struct timezone tz;
        struct tm *tm;
        ::gettimeofday(&tv, &tz);
        tm=::gmtime(&tv.tv_sec);
        char strTime[28];
        ::memset(strTime, '\0', 28);
        ::sprintf(strTime, "%4d-%02d-%02dT%02d:%02d:%02d.%06dZ",
          tm->tm_year + 1900, tm->tm_mon+1, tm->tm_mday,
          tm->tm_hour, tm->tm_min, tm->tm_sec, (int)tv.tv_usec);

        headers << "\"" << strTime << "\"" << ":";

        return true;
      }
      return true;
    }
  };

  class LogFacilityFilter : public LogBaseFilter
  {
  public:
    LogFacilityFilter() :
      LogBaseFilter(LOG_FILTER_FACILITY)
    {
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      if(_enabled)
      {
        static const char* facilityList[45] =
        {
           "PERF",
           "KERNEL",
           "AUTH",
           "NET",
           "RTP",
           "PHONESET",
           "HTTP",
           "SIP",
           "CP",
           "MP",
           "TAO",
           "JNI",
           "JAVA",
           "LOG",
           "SUPERVISOR",
           "OUTGOING",
           "INCOMING",
           "INCOMING_PARSED",
           "MEDIASERVER_CGI",
           "MEDIASERVER_VXI",
           "ACD",
           "PARK",
           "APACHE_AUTH",
           "UPGRADE",
           "LINE_MGR",
           "REFRESH_MGR",
           "UNIT_TEST",
           "STREAMING",
           "REPLICATION_CGI",
           "SIPDB",
           "PROCESSMGR",
           "OS",
           "SIPXTAPI",
           "AUDIO",
           "CONFERENCE",
           "ODBC",
           "CDR",
           "RLS",
           "XMLRPC",
           "FSM",
           "NAT",
           "ALARM",
           "SAA",
           "MONGO_CLIENT",
           "UNKNOWN",
        } ;

        headers <<  facilityList[facility] << ":";

        return true;
      }
      return true;
    }
  };

  class LogPriorityFilter : public LogBaseFilter
  {
  public:
    LogPriorityFilter() :
      LogBaseFilter(LOG_FILTER_PRIORITY)
    {
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      if(_enabled)
      {
        static const char* _priorityNames[8] =
        {
           "DEBUG",
           "INFO",
           "NOTICE",
           "WARNING",
           "ERR",
           "CRIT",
           "ALERT",
           "EMERG"
        };

        headers << _priorityNames[priority] << ":";
        return true;
      }
      return true;
    }
  };

  class LogHostFilter : public LogBaseFilter
  {
  public:
    LogHostFilter() :
      LogBaseFilter(LOG_FILTER_HOST_NAME)
    {
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      if(_enabled)
      {
        headers << _hostName << ":";
        return true;
      }
      return true;
    }
   };

  class LogTaskNameFilter : public LogBaseFilter
  {
  public:
    LogTaskNameFilter() :
      LogBaseFilter(LOG_FILTER_TASK_NAME)
    {
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      if(_enabled)
      {
        headers << task << ":";
        return true;
      }
      return true;
    }
   };

  class LogThreadIdFilter : public LogBaseFilter
  {
  public:
    LogThreadIdFilter() :
      LogBaseFilter(LOG_FILTER_THREAD_ID)
    {
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      if(_enabled)
      {
        headers << std::hex << pthread_self() << ":";
        return true;
      }
      return true;
    }
  };

  class LogProcessNameFilter : public LogBaseFilter
  {
  public:
    LogProcessNameFilter() :
      LogBaseFilter(LOG_FILTER_PROCESS_NAME)
    {
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      if(_enabled)
      {
        headers << _processName << ":";
        return true;
      }
      return true;
    }
   };

  class LogEscapeFilter : public LogBaseFilter
  {
  public:
    LogEscapeFilter() :
      LogBaseFilter(LOG_FILTER_ESCAPE)
    {
    }
    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      if(_enabled)
      {
        std::string escaped;
        escaped.reserve(message.size());
        for (std::string::const_iterator iter = message.begin(); iter != message.end(); iter++)
        {
          if (*iter == '\\')
            escaped.append("\\\\");
          else if (*iter == '\r')
            escaped.append("\\r");
          else if (*iter == '\n')
            escaped.append("\\n");
          else if (*iter == '\"')
            escaped.append("\\\"");
          else
            escaped.append(1, *iter);
        }
        message = escaped;
        return true;
      }
      return true;
    }
  };

  class NullFilter : public LogBaseFilter
  {
  public:
    NullFilter():
      LogBaseFilter(LOG_FILTER_NULL)
    {
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      return false;
    }
  };

  template <
    typename TFilter_1 = NullFilter,
    typename TFilter_2 = NullFilter,
    typename TFilter_3 = NullFilter,
    typename TFilter_4 = NullFilter,
    typename TFilter_5 = NullFilter,
    typename TFilter_6 = NullFilter,
    typename TFilter_7 = NullFilter,
    typename TFilter_8 = NullFilter,
    typename TFilter_9 = NullFilter,
    typename TFilter_10 = NullFilter,
    typename TFilter_11 = NullFilter,
    typename TFilter_12 = NullFilter,
    typename TFilter_13 = NullFilter,
    typename TFilter_14 = NullFilter,
    typename TFilter_15 = NullFilter,
    typename TFilter_16 = NullFilter,
    typename TFilter_17 = NullFilter,
    typename TFilter_18 = NullFilter,
    typename TFilter_19 = NullFilter,
    typename TFilter_20 = NullFilter
  >
  class LogLevelFilter
  {
  public:
    enum LogLevel{ debug, information, notice, warning, error, critical, alert, emergency };
    
    LogLevelFilter(LogLevel level = notice) :
      _level(level)
    {
    }

    ~LogLevelFilter()
    {
    }

    bool willLog(int priority)
    {
      return priority >= _level;
    }
    
    bool willLog(int facility, int priority)
    {
      std::map<int, int>::const_iterator facIter = _facilityLevel.find(facility);
      return facIter != _facilityLevel.end() && priority >= facIter->second;
    }

    bool filter(int facility, int priority, const std::string& task, std::ostringstream& headers, std::string& message)
    {
      std::map<int, int>::const_iterator facIter = _facilityLevel.find(facility);
      if (facIter != _facilityLevel.end() && priority < facIter->second)
        return false;
      else if (priority < _level)
        return false;

      if (_f1.filter(facility, priority,  task, headers, message))
        if (_f2.filter(facility, priority,  task, headers, message))
          if (_f3.filter(facility, priority,  task, headers, message))
            if (_f4.filter(facility, priority,  task, headers, message))
              if (_f5.filter(facility, priority,  task, headers, message))
                if (_f6.filter(facility, priority,  task, headers, message))
                  if (_f7.filter(facility, priority,  task, headers, message))
                    if (_f8.filter(facility, priority,  task, headers, message))
                      if (_f9.filter(facility, priority,  task, headers, message))
                        if (_f10.filter(facility, priority,  task, headers, message))
                          if (_f11.filter(facility, priority,  task, headers, message))
                            if (_f12.filter(facility, priority,  task, headers, message))
                              if (_f13.filter(facility, priority,  task, headers, message))
                                if (_f14.filter(facility, priority,  task, headers, message))
                                  if (_f15.filter(facility, priority,  task, headers, message))
                                    if (_f16.filter(facility, priority,  task, headers, message))
                                      if (_f17.filter(facility, priority,  task, headers, message))
                                        if (_f18.filter(facility, priority,  task, headers, message))
                                          if (_f19.filter(facility, priority,  task, headers, message))
                                            if (_f20.filter(facility, priority,  task, headers, message))
                                              return true;

      return true;
    }

    void setLevel(int level)
    {
      _level = (LogLevel)level;
    }

    void setLoggingPriorityForFacility(int facility, int level)
    {
      _facilityLevel[facility] = level;
    }

    int getLevel()
    {
      return _level;
    }

    bool getLevelFromString(const char* priority, int& level)
    {
      static const char* _priorityNames[8] =
      {
         "DEBUG",
         "INFO",
         "NOTICE",
         "WARNING",
         "ERR",
         "CRIT",
         "ALERT",
         "EMERG"
      };

      bool found = false;
      for ( int entry = debug; !found && entry < 8; entry++)
      {
        if (::strcasecmp(_priorityNames[entry], priority) == 0)
        {
           level = entry;
           found=true;
        }
      }
      return found;
    }

    const char* priorityName(int level)
    {
      static const char* _priorityNames[8] =
      {
         "DEBUG",
         "INFO",
         "NOTICE",
         "WARNING",
         "ERR",
         "CRIT",
         "ALERT",
         "EMERG"
      };

      return _priorityNames[level];
    }

    bool mustFlush(int level)
    {
      return level == debug || level >= warning;
    }

    void enableFilters(const std::set<std::string>& filters)
    {
      _filters = filters;
      _f1.enable(filters);
      _f2.enable(filters);
      _f3.enable(filters);
      _f4.enable(filters);
      _f5.enable(filters);
      _f6.enable(filters);
      _f7.enable(filters);
      _f8.enable(filters);
      _f9.enable(filters);
      _f10.enable(filters);
      _f11.enable(filters);
      _f12.enable(filters);
      _f13.enable(filters);
      _f14.enable(filters);
      _f15.enable(filters);
      _f16.enable(filters);
      _f17.enable(filters);
      _f18.enable(filters);
      _f19.enable(filters);
      _f20.enable(filters);
    }

    void setHostName(const char* hostName)
    {
      _hostName = hostName;
      _f1._hostName = _hostName;
      _f2._hostName = _hostName;
      _f3._hostName = _hostName;
      _f4._hostName = _hostName;
      _f5._hostName = _hostName;
      _f6._hostName = _hostName;
      _f7._hostName = _hostName;
      _f8._hostName = _hostName;
      _f9._hostName = _hostName;
      _f10._hostName = _hostName;
      _f11._hostName = _hostName;
      _f12._hostName = _hostName;
      _f13._hostName = _hostName;
      _f14._hostName = _hostName;
      _f15._hostName = _hostName;
      _f16._hostName = _hostName;
      _f17._hostName = _hostName;
      _f18._hostName = _hostName;
      _f19._hostName = _hostName;
      _f20._hostName = _hostName;
    }

    void setProcessName(const char* processName)
    {
      _processName = processName;
      _f1._processName = _processName;
      _f2._processName = _processName;
      _f3._processName = _processName;
      _f4._processName = _processName;
      _f5._processName = _processName;
      _f6._processName = _processName;
      _f7._processName = _processName;
      _f8._processName = _processName;
      _f9._processName = _processName;
      _f10._processName = _processName;
      _f11._processName = _processName;
      _f12._processName = _processName;
      _f13._processName = _processName;
      _f14._processName = _processName;
      _f15._processName = _processName;
      _f16._processName = _processName;
      _f17._processName = _processName;
      _f18._processName = _processName;
      _f19._processName = _processName;
      _f20._processName = _processName;
    }

    public:
      std::string _hostName;
      std::string _processName;
      std::set<std::string> _filters;
      std::map<int, int> _facilityLevel;

  private:
    int _level;
    TFilter_1 _f1;
    TFilter_2 _f2;
    TFilter_3 _f3;
    TFilter_4 _f4;
    TFilter_5 _f5;
    TFilter_6 _f6;
    TFilter_7 _f7;
    TFilter_8 _f8;
    TFilter_9 _f9;
    TFilter_10 _f10;
    TFilter_11 _f11;
    TFilter_12 _f12;
    TFilter_13 _f13;
    TFilter_14 _f14;
    TFilter_15 _f15;
    TFilter_16 _f16;
    TFilter_17 _f17;
    TFilter_18 _f18;
    TFilter_19 _f19;
    TFilter_20 _f20;
  };

  template <typename T>
  class LoggerSingleton : boost::noncopyable
  {
  public:
    static T& instance()
    {
      static T singleton;
      return singleton;
    }
  };

  template <typename T>
  class LogRotateStrategy
  {
  public:
    T* _pChannel;
    LogRotateStrategy() : _pChannel(0), _started(false)
    {
      _now = ::time(0);
    }

    ~LogRotateStrategy()
    {
      stop();
    }

    void start(T* pChannel)
    {
      _pChannel = pChannel;
      _now = ::time(0);
      _started = true;
    }

    void stop()
    {
      _started = false;
    }

    void wakeup()
    {
      if (!_started || !_pChannel)
        return;

      time_t now = ::time(0);

      if (now >= _now + 5)
      {
        //
        // Recreate the channel every 5 seconds
        //
        _now = now;
        _pChannel->close();
        _pChannel->open(_pChannel->path());
      }
    }

    time_t _now;
    bool _started;
  };

  template <typename TFilter, typename TChannel, typename TLogRotate = LogRotateStrategy<TChannel> >
  class LoggerBase : public LoggerSingleton<LoggerBase<TFilter, TChannel> >
  {
  public:
    typedef boost::shared_mutex mutex_read_write;
    typedef boost::shared_lock<boost::shared_mutex> mutex_read_lock;
    typedef boost::lock_guard<boost::shared_mutex> mutex_write_lock;
    typedef boost::function<std::string()> TaskCallBack;

    LoggerBase() :
      _flushRate(1),
      _enableConsoleOutput(false),
      _enableVerbose(false),
      _lineNumber(0)
    {
      _pChannel = new TChannel();
      _pFilter = new TFilter();
    }

    ~LoggerBase()
    {
      delete _pChannel;
      delete _pFilter;
    }

    bool open(const char* logFile)
    {
      mutex_write_lock lock(_mutex);
      return _pChannel->open(logFile);
    }

    bool reopen()
    {
      mutex_write_lock lock(_mutex);
      _pChannel->close();
      return _pChannel->open(_pChannel->path());
    }

    void flush()
    {
      _pChannel->flush();
    }

    void setLevel(int level)
    {
      _pFilter->setLevel(level);
    }

    int getLevel()
    {
      return _pFilter->getLevel();
    }

    void setLogPriority(int priority)
    {
      _pFilter->setLevel(priority);
    }

    bool priority(const char* priority, int& level)
    {
      return _pFilter->getLevelFromString(priority, level);
    }

    const char* priorityName(int level)
    {
      return _pFilter->priorityName(level);
    }

    bool willLog(int priority)
    {
      return _pFilter->willLog(priority);
    }

    bool willLog(int facility, int priority)
    {
      return _pFilter->willLog(facility, priority);
    }

    void setLoggingPriorityForFacility(int facility, int level)
    {
      _pFilter->setLoggingPriorityForFacility(facility, level);
    }

    void setHostName(const char* hostName)
    {
      _pFilter->setHostName(hostName);
    }

    void setProcessName(const char* processName)
    {
      _pFilter->setProcessName(processName);
    }

    void setVerbose(const std::string& sourceName, const std::string& functionName, int lineNumber)
    {
      _sourceName = sourceName;
      _functionName = functionName;
      _lineNumber = lineNumber;
    }

    void getSetFromString(const std::string& strFilters, std::set<std::string>& filters)
    {
      const std::string& delimiters = LOG_FORMAT_DELIMITERS;
      boost::split(filters, strFilters, boost::is_any_of(delimiters));
    }

    void enableFilters(const std::string& stringFilters)
    {
      getSetFromString(stringFilters, _filters);
      enableFilters(_filters);
    }

    void enableFilters(const std::set<std::string>& filters)
    {
      _pFilter->enableFilters(filters);
    }

    bool isVerboseEnabled()
    {
      return _enableVerbose;
    }

    //
    // This is the OsSysLog compatibility function
    //
    void log(int facility, int level, const char* format, ...)
    {
      if (!willLog(level))
        return;
      
      char* buff;
      size_t needed = 1024;
      bool formatted = false;
      std::string message;

      if(_enableVerbose && !_functionName.empty(), !_sourceName.empty() && _lineNumber != 0)
      {
        buff = (char*)::malloc(needed);
        snprintf(buff, needed, "%s:%d %s: ", _sourceName.c_str(), _lineNumber, _functionName.c_str());
        message = buff;
        ::free(buff);

        // reset additional data
        _functionName.erase(_functionName.begin(), _functionName.end());
        _sourceName.erase(_sourceName.begin(), _sourceName.end());
        _lineNumber = 0;
      }

      for (int i = 0; !formatted && i < 3; i++)
      {
        va_list args;
        va_start(args, format);
        buff = (char*)::malloc(needed); /// Create a big enough buffer
        ::memset(buff, '\0', needed);
        size_t oldSize = needed;
        needed = vsnprintf(buff, needed, format, args) + 1;
        formatted = needed <= oldSize;
        if (formatted)
          message += buff;
        ::free(buff);
        va_end(args);
      }

      if (formatted)
        log_(facility, level, message.c_str());
    }

    void printf(int facility, int level, const char* format, ...)
    {
      char* buff;
      size_t needed = 1024;

      bool formatted = false;
      std::string message;
      for (int i = 0; !formatted && i < 3; i++)
      {
        va_list args;
        va_start(args, format);
        buff = (char*)::malloc(needed); /// Create a big enough buffer
        ::memset(buff, '\0', needed);
        size_t oldSize = needed;
        needed = vsnprintf(buff, needed, format, args) + 1;
        formatted = needed <= oldSize;
        if (formatted)
          message = buff;
        ::free(buff);
        va_end(args);
      }

      if (formatted)
        log_(facility, level, message.c_str(), &std::cerr);
    }

    void log_(int facility, int level, const char* msg, std::ostream* pAlternateChannel = 0)
    {
      std::string task;
      if (getCurrentTask)
        task = getCurrentTask();
      log_(facility, level, task.c_str(), msg, pAlternateChannel);
    }

    void cerr(int facility, int level, const std::string& taskName, const std::string& msg)
    {
      log_(facility, level, taskName, msg, &std::cerr);
    }

    void cerr(int facility, int level, const char* msg)
    {
      std::string task;
      if (getCurrentTask)
        task = getCurrentTask();
      log_(facility, level, task.c_str(), msg, &std::cerr);
    }

    void cout(int facility, int level, const std::string& taskName, const std::string& msg)
    {
      log_(facility, level, taskName, msg, &std::cout);
    }
    
    void cout(int facility, int level, const char* msg)
    {
      std::string task;
      if (getCurrentTask)
        task = getCurrentTask();
      log_(facility, level, task.c_str(), msg, &std::cout);
    }

    void log_(int facility, int level, const std::string& taskName, const std::string& msg, std::ostream* pAlternateChannel = 0)
    {
      mutex_write_lock lock(_mutex);
      //
      // Create the header
      //
      std::ostringstream headers;
      std::string message(msg);
      if (_pFilter->filter(facility, level, taskName, headers, message))
      {
        //
        // Check if an external logger is set and has consumed the log
        //
        if (_externalLogger && _externalLogger(facility, level, headers, message))
          return;
        //
        // Flush the log
        //
        headers << "\"" << message << "\"" << std::endl;
        std::string hdr(headers.str());
        _pChannel->write(hdr.c_str(), hdr.length());
        if (pAlternateChannel)
          pAlternateChannel->write(hdr.c_str(), hdr.length());
        else if (_enableConsoleOutput)
          std::cerr.write(hdr.c_str(), hdr.length());

        static unsigned long flushCount = 0;
        
        if (_pFilter->mustFlush(level))
        {
          _pChannel->flush();
        }
        else if (_flushRate == 1 || (_flushRate && !(++flushCount % _flushRate)))
        {
          _pChannel->flush();
          if (pAlternateChannel)
            pAlternateChannel->flush();
        }

        _logRotateStrategy.wakeup();
      }
    }

    void setFlushRate(unsigned flushRate)
    {
      _flushRate = flushRate;
    }

    void setCurrentTaskCallBack(TaskCallBack taskCallBack)
    {
      getCurrentTask = taskCallBack;
    }

    template <typename Helper>
    bool initialize(int priorityLevel, const char* path, Helper& helper)
    {
      setLevel(priorityLevel);
      setHostName(helper.getHostName().c_str());
      setProcessName(helper.getProcessName().c_str());
      enableFilters(helper.getFilterNames());
      setCurrentTaskCallBack(boost::bind(&Helper::getCurrentTask, &helper));

      if (open(path))
      {
        _logRotateStrategy.start(_pChannel);
        return true;
      }
      return false;
    }

    template <typename Helper>
    bool initialize(const char* path, Helper& helper)
    {
      setHostName(helper.getHostName().c_str());
      setProcessName(helper.getProcessName().c_str());
      enableFilters(helper.getFilterNames());
      setCurrentTaskCallBack(boost::bind(&Helper::getCurrentTask, &helper));
      if (open(path))
      {
        _logRotateStrategy.start(_pChannel);
        return true;
      }
      else
      {
        std::cerr << "Unable to create log file " << path << "!" << std::endl;
      }
      return false;
    }

    void enableConsoleOutput(bool enableConsoleOutput = true)
    {
      _enableConsoleOutput = enableConsoleOutput;
    }
    
    void setExternalLogger(const OsExternalLogger& externalLogger)
    {
      _externalLogger = externalLogger;
    }

    void enableVerbose(bool enableVerbose = true)
    {
      _enableVerbose = enableVerbose;
    }

    void enableFilter(const std::string& filter)
    {
      _filters.insert(filter);
    }

  private:
    TChannel* _pChannel;
    TFilter* _pFilter;
    TLogRotate _logRotateStrategy;
    mutex_read_write _mutex;
    unsigned _flushRate;
    TaskCallBack getCurrentTask;
    bool _enableConsoleOutput;
    OsExternalLogger _externalLogger;
    bool _enableVerbose;
    std::set<std::string> _filters;
    std::string _sourceName;
    std::string _functionName;
    int _lineNumber;
  };

  typedef LogLevelFilter<
    LogTimeFilter,
    LogCounterFilter,
    LogFacilityFilter,
    LogPriorityFilter,
    LogHostFilter,
    LogTaskNameFilter,
    LogThreadIdFilter,
    LogProcessNameFilter,
    LogEscapeFilter> LogFilter;

  typedef LoggerBase<LogFilter, LogFileChannel> Logger;

  
  //
  // Macros
  //
  #define OS_LOG_PUSH(priority, facility,  source, function, line, data) \
  { \
    if (Os::Logger::instance().willLog(priority)) { \
      std::ostringstream strm; \
      strm << data; \
      if(Os::Logger::instance().isVerboseEnabled()) \
      { \
        Os::Logger::instance().setVerbose(source, function, line); \
      } \
      Os::Logger::instance().log(facility, priority, "%s", strm.str().c_str()); \
    }\
  }

  #define OS_LOG_DEBUG(facility, data) OS_LOG_PUSH(Os::LogFilter::debug, facility, __FILE__, __func__, __LINE__, data)
  #define OS_LOG_INFO(facility, data) OS_LOG_PUSH(Os::LogFilter::information, facility, __FILE__, __func__, __LINE__, data)
  #define OS_LOG_NOTICE(facility, data) OS_LOG_PUSH(Os::LogFilter::notice, facility, __FILE__, __func__, __LINE__, data)
  #define OS_LOG_WARNING(facility, data) OS_LOG_PUSH(Os::LogFilter::warning, facility, __FILE__, __func__, __LINE__, data)
  #define OS_LOG_ERROR(facility, data) OS_LOG_PUSH(Os::LogFilter::error, facility, __FILE__, __func__, __LINE__, data)
  #define OS_LOG_CRITICAL(facility, data) OS_LOG_PUSH(Os::LogFilter::critical, facility, __FILE__, __func__, __LINE__, data)
  #define OS_LOG_ALERT(facility, data) OS_LOG_PUSH(Os::LogFilter::alert, facility, __FILE__, __func__, __LINE__, data)
  #define OS_LOG_EMERGENCY(facility, data) OS_LOG_PUSH(Os::LogFilter::emergency, facility, __FILE__, __func__, __LINE__, data)


  #define OS_LOG_AND_ASSERT(condition, facility, data)                 \
  {                                                                    \
      if (!(condition))                                                \
      {                                                                \
          OS_LOG_PUSH(PRI_CRIT, facility, __FILE__, __func__, __LINE__, data);\
          assert(condition);                                           \
      }                                                                \
  }

  #define CLASS_INFO() className() << "::" << __FUNCTION__ << "::" << this << " "
  #define CLASS_INFO_STATIC() className() << "::" << __FUNCTION__ << " "
  #define CLASS_STATS() "STATS >>>"

} // Utl


#endif	/* LOGGER_H */

