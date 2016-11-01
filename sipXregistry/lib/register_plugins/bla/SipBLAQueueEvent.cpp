//
//
// Copyright (C) 2016 Sipfoundry, Org., certain elements licensed under a Contributor Agreement.
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
//
// $$
//////////////////////////////////////////////////////////////////////////////

#include "SipBLAQueueEvent.h"

#include "os/OsLock.h"
#include "os/OsDateTime.h"
#include "os/OsLogger.h"
#include "os/OsConfigDb.h"
#include "utl/UtlString.h"
#include "utl/UtlRegex.h"
#include "net/NetMd5Codec.h"
#include "registry/SipRegistrar.h"
#include "sipdb/EntityDB.h"
#include <net/CallId.h>

#define DEFAULT_REDIS_PORT 6379
#define DEFAULT_CHANNEL_NAME "SIPX.BLA.REGISTER"

extern "C" RegisterPlugin* getRegisterPlugin(const UtlString& name)
{
   OsLock lock(*SipBLAQueueEvent::mpSingletonLock);

   RegisterPlugin* thePlugin;

   if (!SipBLAQueueEvent::mpSingleton)
   {
      SipBLAQueueEvent::mpSingleton = new SipBLAQueueEvent(name);
      thePlugin = dynamic_cast<RegisterPlugin*>(SipBLAQueueEvent::mpSingleton);
   }
   else
   {
      Os::Logger::instance().log(FAC_SIP, PRI_ERR,
                    "SipBLAQueueEvent plugin may not be configured twice.\n"
                    "   First configured instance is %s.\n"
                    "   Second instance [%s] not created.",
                    SipBLAQueueEvent::mpSingleton->mLogName.data(), name.data()
                    );

      thePlugin = NULL;
   }

   return thePlugin;
}

OsBSem* SipBLAQueueEvent::mpSingletonLock = new OsBSem( OsBSem::Q_PRIORITY
                                                              ,OsBSem::FULL
                                                              );
SipBLAQueueEvent* SipBLAQueueEvent::mpSingleton;

// the 'SIP_REGISTRAR.<instancename>.' has been stripped by the time we see it...
const char SipBLAQueueEvent::ConfigPrefix[] = "REDIS.";

boost::format SipBLAQueueEvent::MessageFormat("{\"aor\":\"%s\", \"contact\":\"%s\", \"duration\":%i}");

SipBLAQueueEvent::SipBLAQueueEvent(const UtlString& name) 
  : RegisterPlugin(name)
  , mChannelName(DEFAULT_CHANNEL_NAME)
  , mRedisDb(0)
  , mRedisPort(DEFAULT_REDIS_PORT)
{
   mLogName.append("[");
   mLogName.append(name);
   mLogName.append("] SipBLAQueueEvent");
}

SipBLAQueueEvent::~SipBLAQueueEvent()
{
  mRedisClient.disconnect();
}

void SipBLAQueueEvent::readConfig( OsConfigDb& configDb )
{
  OsConfigDb impliedRedisConfig;
  UtlString channelName;
  UtlString password;
  int redisDb;
  int redisPort;

  Os::Logger::instance().log(FAC_SIP, PRI_DEBUG, "SipBLAQueueEvent[%s]::readConfig",
                 mInstanceName.data()
                 );

  // extract the database of implied message waiting subscriptions
  configDb.getSubHash( ConfigPrefix
                      ,impliedRedisConfig
                      );
  impliedRedisConfig.get("ADDRESS", mRedisAddress);
  if(impliedRedisConfig.get("PORT", redisPort) == OS_SUCCESS) 
  {
    mRedisPort = redisPort;
  }

  if(impliedRedisConfig.get("PASSWORD", password) == OS_SUCCESS)
  {
    mRedisPassword = password;
  }

  if(impliedRedisConfig.get("DB", redisDb) == OS_SUCCESS)
  {
    mRedisDb = redisDb;
  }

  if(impliedRedisConfig.get("CHANNEL", channelName) == OS_SUCCESS) 
  {
    mChannelName = channelName;
  }

  Os::Logger::instance().log( FAC_SIP, PRI_INFO
                ,"%s::readConfig REDIS=\"%s:%d\""
                ,mLogName.data(), mRedisAddress.data(), mRedisPort
                );

  bool connected = mRedisClient.connect(mRedisAddress.str(), mRedisPort, mRedisPassword.str(), mRedisDb);
  if(!connected)
  {
    Os::Logger::instance().log( FAC_SIP, PRI_ERR
                ,"%s::readConfig Unable to establish connection to REDIS=\"%s:%d\""
                ,mLogName.data(), mRedisAddress.data(), mRedisPort
      );
  }
}

void SipBLAQueueEvent::takeAction(
    const SipMessage&   registerMessage  ///< the successful registration
   ,const unsigned int  registrationDuration /**< the actual allowed
                                              * registration time (note
                                              * that this may be < the
                                              * requested time). */
   ,SipUserAgent*       sipUserAgent     /**< to be used if the plugin
                                          *   wants to send any SIP msg */
                                         )
{  
  if(registrationDuration > 0)
  {
    Url fromUrl;
    UtlString identity;
    
    registerMessage.getFromUrl(fromUrl);
    fromUrl.getIdentity(identity);

    EntityRecord entity;
    EntityDB* entityDb = SipRegistrar::getInstance(NULL)->getEntityDB();
    if (entityDb && entityDb->findByIdentity(identity.str(), entity))
    {
      std::string sharedId = entity.shared();
      if(!sharedId.empty())
      {
        Os::Logger::instance().log( FAC_SIP, PRI_INFO
                    ,"%s requesting BLA event subscription for %s duration=%d"
                    ,mLogName.data(), sharedId.c_str(), registrationDuration
                    );

        UtlString contactUri;
        registerMessage.getContactUri(0, &contactUri);

        UtlString message = boost::str(MessageFormat % sharedId % contactUri % registrationDuration);

        mRedisClient.publish(mChannelName.str(), message.str());
      }
    }
  }
}
