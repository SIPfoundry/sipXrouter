/* 
 * File:   CallerIdMasking.h
 * Author: root
 *
 * Created on July 4, 2016, 4:14 PM
 */

#ifndef CALLERIDMASKING_H
#define	CALLERIDMASKING_H

#include <string>
#include <sstream>

#include "digitmaps/EmergencyRulesUrlMapping.h"
#include "sipdb/MongoDB.h"
#include "sipdb/MongoMod.h"
#include "net/SipBidirectionalProcessorPlugin.h"
#include "os/OsConfigDb.h"

extern "C" SipBidirectionalProcessorPlugin* getTransactionPlugin(const UtlString& pluginName);

class CallerIdMasking : 
  public MongoDB::BaseDB, 
  public SipBidirectionalProcessorPlugin
{
public:
  CallerIdMasking(const UtlString& name);
  ~CallerIdMasking();
  void readConfig(OsConfigDb& config);
  void handleIncoming(SipMessage& msg, const char* addr, int port);
  void handleOutgoing(SipMessage& msg, const char* addr, int port);
  void initialize();
  EmergencyRulesUrlMapping _emergencyRules;
  bool _hasEmergencyRules;
};

//
// Inlines
//
inline void CallerIdMasking::initialize() {};
#endif	/* CALLERIDMASKING_H */

