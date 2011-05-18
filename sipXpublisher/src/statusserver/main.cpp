//
//
// Copyright (C) 2007 Pingtel Corp., certain elements licensed under a Contributor Agreement.
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
//
// $$
//////////////////////////////////////////////////////////////////////////////
// StatusServer.cpp : Defines the entry point for the console application.
//

// SYSTEM INCLUDES
#include <iostream>
#include <stdio.h>
#include <signal.h>

// APPLICATION INCLUDES
#include "config.h"
#include "os/OsConfigDb.h"
#include "os/OsFS.h"
#include "os/OsTask.h"

#include "net/NameValueTokenizer.h"
#include "sipXecsService/SipXecsService.h"    // now deregister this process's database references from the IMDB
#include "statusserver/StatusServer.h"

#include <os/OsLogger.h>
#include <os/OsLoggerHelper.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS
// GLOBAL VARIABLE INITIALIZATIONS
OsServerTask* pServerTask   = NULL;
UtlBoolean     gShutdownFlag = FALSE;
UtlBoolean     gClosingIMDB  = FALSE;
OsMutex*       gpLockMutex = new OsMutex(OsMutex::Q_FIFO);

using namespace std;

/* ============================ FUNCTIONS ================================= */


class SignalTask : public OsTask
{
public:
   SignalTask() : OsTask() {}

   int
   run(void *pArg)
   {
       int sig_num ;
       OsStatus res ;

       // Wait for a signal.  This will unblock signals
       // for THIS thread only, so this will be the only thread
       // to catch an async signal directed to the process
       // from the outside.
       res = awaitSignal(sig_num);
       if (res == OS_SUCCESS)
       {
          if (SIGTERM == sig_num)
          {
             Os::Logger::instance().log( LOG_FACILITY, PRI_INFO, "SignalTask: terminate signal received.");
          }
          else
          {
            Os::Logger::instance().log( LOG_FACILITY, PRI_CRIT, "SignalTask: caught signal: %d", sig_num );
          }
       }
       else
       {
            Os::Logger::instance().log( LOG_FACILITY, PRI_CRIT, "SignalTask: awaitSignal() failed");
       }
       // set the global shutdown flag
       gShutdownFlag = TRUE ;
       return 0 ;
   }
} ;

// Initialize the OsSysLog
void initSysLog(OsConfigDb* pConfig)
{
   UtlString logLevel;               // Controls Log Verbosity
   UtlString consoleLogging;         // Enable console logging by default?
   UtlString fileTarget;             // Path to store log file.
   UtlBoolean bSpecifiedDirError ;   // Set if the specified log dir does not
                                    // exist

   Os::LoggerHelper::instance().processName = "SipStatus";

   //
   // Get/Apply Log Filename
   //
   fileTarget.remove(0) ;
   if ((pConfig->get(CONFIG_SETTING_LOG_DIR, fileTarget) != OS_SUCCESS) ||
      fileTarget.isNull() || !OsFileSystem::exists(fileTarget))
   {
      bSpecifiedDirError = !fileTarget.isNull() ;

      // If the log file directory exists use that, otherwise place the log
      // in the current directory
      OsPath workingDirectory;
      if (OsFileSystem::exists(CONFIG_LOG_DIR))
      {
         fileTarget = CONFIG_LOG_DIR;
         OsPath path(fileTarget);
         path.getNativePath(workingDirectory);

         Os::Logger::instance().log(LOG_FACILITY, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_DIR, workingDirectory.data()) ;
      }
      else
      {
         OsPath path;
         OsFileSystem::getWorkingDirectory(path);
         path.getNativePath(workingDirectory);

         Os::Logger::instance().log(LOG_FACILITY, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_DIR, workingDirectory.data()) ;
      }

      fileTarget = workingDirectory +
         OsPathBase::separator +
         CONFIG_LOG_FILE;
   }
   else
   {
      bSpecifiedDirError = false ;
      Os::Logger::instance().log(LOG_FACILITY, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_DIR, fileTarget.data()) ;

      fileTarget = fileTarget +
         OsPathBase::separator +
         CONFIG_LOG_FILE;
   }


   //
   // Get/Apply Log Level
   //
   SipXecsService::setLogPriority(*pConfig, CONFIG_SETTING_PREFIX);
   Os::Logger::instance().setLoggingPriorityForFacility(FAC_SIP_INCOMING_PARSED, PRI_ERR);
   Os::LoggerHelper::instance().initialize(fileTarget);
   //
   // Get/Apply console logging
   //
   UtlBoolean bConsoleLoggingEnabled = false ;
   if ((pConfig->get(CONFIG_SETTING_LOG_CONSOLE, consoleLogging) ==
         OS_SUCCESS))
   {
      consoleLogging.toUpper();
      if (consoleLogging == "ENABLE")
      {
         Os::Logger::instance().enableConsoleOutput(true);
         bConsoleLoggingEnabled = true ;
      }
   }

   Os::Logger::instance().log(LOG_FACILITY, PRI_INFO, "%s : %s", CONFIG_SETTING_LOG_CONSOLE, bConsoleLoggingEnabled ? "ENABLE" : "DISABLE") ;

   if (bSpecifiedDirError)
   {
      Os::Logger::instance().log(LOG_FACILITY, PRI_CRIT, "Cannot access %s directory; please check configuration.", CONFIG_SETTING_LOG_DIR);
   }
}




/** The main entry point to the StatusServer */
int
main(int argc, char* argv[] )
{
   // Block all signals in this the main thread.
   // Any threads created after this will have all signals masked.
   OsTask::blockSignals();

   // Create a new task to wait for signals.  Only that task
   // will ever see a signal from the outside.
   SignalTask* signalTask = new SignalTask();
   signalTask->start();

   OsConfigDb  configDb ;  // Params for OsSysLog init

   UtlBoolean interactiveSet = false;
   UtlString argString;
   for(int argIndex = 1; argIndex < argc; argIndex++)
   {
      Os::Logger::instance().log(LOG_FACILITY, PRI_INFO, "arg[%d]: %s\n", argIndex, argv[argIndex]) ;
      argString = argv[argIndex];
      NameValueTokenizer::frontBackTrim(&argString, "\t ");
      if(argString.compareTo("-v") == 0)
      {
         Os::Logger::instance().log(LOG_FACILITY, PRI_INFO, "Version: %s %s\n", VERSION, PACKAGE_REVISION);
         return(1);
      }
      else if( argString.compareTo("-i") == 0)
      {
         interactiveSet = true;
         Os::Logger::instance().log(LOG_FACILITY, PRI_INFO, "Entering Interactive Mode\n");
      }
      else
      {
         Os::Logger::instance().log(LOG_FACILITY, PRI_INFO, "usage: %s [-v] [-i]\nwhere:\n -v provides the software version\n"
            " -i start the server in an interactive made\n",
         argv[0]);
         return(1);
      }
   }

    // initialize log file
   OsPath workingDirectory;
   if (OsFileSystem::exists(CONFIG_ETC_DIR))
   {
      workingDirectory = CONFIG_ETC_DIR;
      OsPath path(workingDirectory);
      path.getNativePath(workingDirectory);
   }
   else
   {
      OsPath path;
      OsFileSystem::getWorkingDirectory(path);
      path.getNativePath(workingDirectory);
   }

    UtlString fileName =  workingDirectory +
      OsPathBase::separator +
      CONFIG_SETTINGS_FILE;

    if (configDb.loadFromFile(fileName) != OS_SUCCESS)
    {
       exit(1);
    }
    initSysLog(&configDb) ;

    // Fetch Pointer to the OsServer task object, note that
    // object uses the IMDB so it is important to shut this thread
    // cleanly before the signal handler exits
    StatusServer* pStatusServer = StatusServer::getInstance();

    pServerTask = static_cast<OsServerTask*>(pStatusServer);

    // Do not exit, let the proxy do its stuff
    while( !gShutdownFlag )
    {
        if( interactiveSet)
        {
            int charCode = getchar();

            if(charCode != '\n' && charCode != '\r')
            {
                if( charCode == 'e')
                {
                    Os::Logger::instance().enableConsoleOutput(true);
                } else if( charCode == 'd')
                {
                    Os::Logger::instance().enableConsoleOutput(false);
                } else
                {
                    // pStatusServer->printMessageLog();
                }
            }
        } else
        {
            OsTask::delay(2000);
        }
    }

    // Remove the current process's row from the IMDB
    // Persisting the database if necessary
    cout << "Cleaning Up..Start." << endl;

    // This is a server task so gracefully shutdown the
    // server task using the waitForShutdown method, this
    // will implicitly request a shutdown for us if one is
    // not already in progress
    if ( pServerTask != NULL )
    {
        // Deleting a server task is the only way of
        // waiting for shutdown to complete cleanly
        delete pServerTask;
        pServerTask = NULL;
    }


    // Flush the log file
    Os::Logger::instance().flush();

    cout << "Cleanup...Finished" << endl;

    return 0;
}
