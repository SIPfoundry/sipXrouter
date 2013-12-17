
/*
 * Copyright (c) 2011 eZuce, Inc. All rights reserved.
 * Contributed to SIPfoundry under a Contributor Agreement
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#include "sipdb/MongoOpLog.h"
#include <mongo/client/connpool.h>

#include <os/OsLogger.h>

const string MongoOpLog::NS("local.oplog.rs");

const char* MongoOpLog::ts_fld(){static std::string name = "ts"; return name.c_str();}
const char* MongoOpLog::op_fld(){static std::string name = "op"; return name.c_str();}
void MongoOpLog::createOpLogDataMap()
{
  _opLogDataMap.insert(std::pair<std::string, OpLogType>("u", Update));
  _opLogDataMap.insert(std::pair<std::string, OpLogType>("i", Insert));
  _opLogDataMap.insert(std::pair<std::string, OpLogType>("d", Delete));
}

MongoOpLog::MongoOpLog(const MongoDB::ConnectionInfo& info,
                       const mongo::BSONObj& customQuery,
                       const int querySleepTime,
                       const unsigned long startFromTimestamp) :
	MongoDB::BaseDB(info),
  _isRunning(false),
  _pThread(0),
  _querySleepTime(querySleepTime),
  _startFromTimestamp(startFromTimestamp),
  _customQuery(customQuery),
  _ns(NS)
{
  OS_LOG_INFO(FAC_SIP, "MongoOpLog::MongoOpLog:" <<
                        " entering" <<
                        " querySleepTime=" << querySleepTime <<
                        " startFromTimestamp=" << startFromTimestamp);

  _customQuery.getOwned();
  createOpLogDataMap();
}

MongoOpLog::~MongoOpLog()
{
  stop();
}

void MongoOpLog::registerCallback(OpLogType type, OpLogCallBack cb)
{
  if (type < Insert ||
      type > OpLogTypeNumber)
  {
    return;
  }

  OS_LOG_INFO(FAC_SIP, "MongoOpLog::registerCallback:" <<
                        " entering" <<
                        " type=" << type);

  _opLogCbVectors[type].push_back(cb);
}

void MongoOpLog::run()
{
  OS_LOG_INFO(FAC_SIP, "MongoOpLog::run:" <<
                        " starting MongoOpLog thread");

  _isRunning = true;
  _pThread = new boost::thread(boost::bind(&MongoOpLog::internal_run, this));
}

void MongoOpLog::stop()
{
  if (false == _isRunning)
  {
    return;
  }

  OS_LOG_INFO(FAC_SIP, "MongoOpLog::stop:" <<
                       " stopping MongoOpLog thread");

  _isRunning = false;

  if (_pThread)
  {
    _pThread->join();

    delete _pThread;
    _pThread = 0;
  }

  OS_LOG_INFO(FAC_SIP, "MongoOpLog::stop:" <<
                       " exiting");
}

// The monitor thread stays in this function as long as the cursor is not dead
// If the cursor already processed lastEntryObj, it gets blocked in a while in
// cursor->more() for 1 or 2 seconds until new entries are added to this collection.
// After that the entries are processed and the thread gets blocked again in
// cursor->more function
bool MongoOpLog::processQuery(mongo::DBClientCursor* cursor,
                              mongo::BSONObj& lastEntry)
{
  OS_LOG_INFO(FAC_SIP, "MongoOpLog::processQuery:" <<
                        " entering");

  if (!cursor)
  {
    OS_LOG_ERROR(FAC_SIP, "MongoOpLog:processQuery - Cursor is NULL");
    return false;
  }

  while(_isRunning)
  {
    if(!cursor->more())
    {
      if (_querySleepTime)
      {
        sleep(_querySleepTime);
      }

      if(cursor->isDead())
      {
        break;
      }

      continue; // we will try more() again
    }

    // WARNING: BSONObj must stay in scope for the life of the BSONElement
    lastEntry = cursor->next().getOwned();
    runCallBacks(lastEntry);
  }

  return true;
}

void MongoOpLog::createQuery(const mongo::BSONObj& lastEntry, mongo::BSONObj& query)
{
  OS_LOG_INFO(FAC_SIP, "MongoOpLog::createQuery:" <<
                        " entering");

  mongo::BSONObjBuilder queryBSONObjBuilder;

  const mongo::BSONElement lastEntryElem = lastEntry[ts_fld()];

  queryBSONObjBuilder.appendElements(BSON(ts_fld() << mongo::GT << lastEntryElem));
  if (!_customQuery.isEmpty())
  {
    queryBSONObjBuilder.appendElements(_customQuery);
  }
  query = queryBSONObjBuilder.obj();
}

bool MongoOpLog::createFirstQuery(mongo::BSONObj& lastEntry,
                                  mongo::BSONObj& query)
{
  OS_LOG_INFO(FAC_SIP, "MongoOpLog::createFirstQuery:" <<
                        " entering");

  if (_startFromTimestamp == 0)
  {
    lastEntry = mongo::minKey;
  }
  else
  {
    mongo::BSONObjBuilder builder;
    unsigned long long timeStamp = (unsigned long long)_startFromTimestamp << 32;
    builder.appendTimestamp(ts_fld(), timeStamp);

    lastEntry = builder.obj();

    // Check that the created BSONElement has the correct timeStamp
    unsigned long long lastEntryTimeStamp = lastEntry[ts_fld()].timestampTime();
    if (_startFromTimestamp * 1000 != lastEntryTimeStamp)
    {
      OS_LOG_ERROR(FAC_SIP, "MongoOpLog::createFirstQuery" <<
                            " time stamps are different " <<
                            _startFromTimestamp * 1000 << lastEntryTimeStamp);
      return false;
    }
  }

  // WARNING: BSONObj must stay in scope for the life of the BSONElement
  createQuery(lastEntry, query);

  return true;
}

void MongoOpLog::internal_run()
{
  mongo::BSONObj lastEntry;
  mongo::BSONObj query;

  OS_LOG_INFO(FAC_SIP, "MongoOpLog::internal_run:"
              << " entering");

  bool rc = createFirstQuery(lastEntry, query);
  if (false == rc)
  {
    OS_LOG_ERROR(FAC_SIP, "MongoOpLog::createFirstQuery" <<
                 " exited with error");
    return;
  }

  MongoDB::ScopedDbConnectionPtr pConn(mongoMod::ScopedDbConnection::getScopedDbConnection(_info.getConnectionString().toString()));

  while (_isRunning)
  {
    // If we are at the end of the data, block for a while rather
    // than returning no data. After a timeout period, we do return as normal
    std::auto_ptr<mongo::DBClientCursor> cursor = pConn->get()->query(_ns, query, 0, 0, 0,
                 mongo::QueryOption_CursorTailable | mongo::QueryOption_AwaitData );

    bool rc = processQuery(cursor.get(), lastEntry);
    if (false == rc)
    {
      break;
    }

    createQuery(lastEntry, query);
  }

  pConn->done();

  OS_LOG_INFO(FAC_SIP, "MongoOpLog::internal_run:"
              << " exiting");
}

bool MongoOpLog::getOpLogType(const std::string& operationType,
                              OpLogType& opLogType)
{
  OpLogDataMap::iterator it;

  it = _opLogDataMap.find(operationType);
  if (_opLogDataMap.end() != it)
  {
    opLogType = it->second;

    return true;
  }

  return false;
}

void MongoOpLog::runCallBacks(const mongo::BSONObj& bSONObj)
{
  std::string type = bSONObj.getStringField(op_fld());
  std::string opLog = bSONObj.toString();

  OS_LOG_DEBUG(FAC_SIP, "MongoOpLog::notifyCallBacks:" <<
                " type=" << type <<
                " opLog=" << opLog);

  if (type.empty())
  {
    OS_LOG_WARNING(FAC_SIP, "MongoOpLog::notifyCallBacks:"
                << " type is empty");
    return;
  }

  // Notify all subscribers
  for(OpLogCbVector::iterator iter = _opLogCbVectors[All].begin(); iter != _opLogCbVectors[All].end(); iter++)
  {
    (*iter)(bSONObj);
  }

  OpLogType opLogType;
  bool found = false;
  found = getOpLogType(type, opLogType);
  if (false == found)
  {
    return;
  }
  
  // Notify specific subscribers
  for(OpLogCbVector::iterator iter = _opLogCbVectors[opLogType].begin();
      iter != _opLogCbVectors[opLogType].end(); iter++)
  {
    (*iter)(bSONObj);
  }
}

