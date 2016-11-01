//
//
// Copyright (C) 2016 Sipfoundry, Org., certain elements licensed under a Contributor Agreement.
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
//
// $$
//////////////////////////////////////////////////////////////////////////////

#ifndef SIP_BLA_QUEUE_EVENT_H_
#define SIP_BLA_QUEUE_EVENT_H_

/**
 * SIP Registrar Bla Queue Event
 *
 */

// APPLICATION INCLUDES
#include "os/OsBSem.h"
#include "utl/UtlString.h"
#include "sipXecsService/SipNonceDb.h"
#include "net/SipUserAgent.h"
#include "registry/RegisterPlugin.h"
#include <OSS/Persistent/RedisClient.h>
#include <boost/format.hpp>

/// Plugin::Factory method to get the instance of this plugin.
extern "C" RegisterPlugin* getRegisterPlugin(const UtlString& name);

/**
 * This plugin PUBLISH contact info to REDIS for all user who enable SLA/BLA.
 */
class SipBLAQueueEvent : public RegisterPlugin
{
public:

    /** dtor */
    virtual ~SipBLAQueueEvent();

    virtual void readConfig( OsConfigDb& configDb );

    virtual void takeAction( const SipMessage&   registerMessage  ///< the successful registration
                            ,const unsigned int  registrationDuration /**< the actual allowed
                                                                       * registration time (note
                                                                       * that this may be < the
                                                                       * requested time). */
                            ,SipUserAgent*       sipUserAgent     /**< to be used if the plugin
                                                                   *   wants to send any SIP msg */
                            );


private:
    static OsBSem*                  mpSingletonLock;
    static SipBLAQueueEvent*        mpSingleton;

    OSS::Persistent::RedisClient    mRedisClient;

    // String to use in place of class name in log messages:
    // "[instance] class".
    UtlString mLogName;
    UtlString mRedisAddress;
    UtlString mRedisPassword;
    UtlString mChannelName;
    int mRedisDb;
    int mRedisPort;


    /**
     * Constructor is hidden so that only the factory can instantiate it.
     */
    SipBLAQueueEvent(const UtlString& name);
    friend RegisterPlugin* getRegisterPlugin(const UtlString& name);

    /**  ConfigPrefix -
     *     The configuration may contain any number of directives
     *     that begin with this prefix - the value of each is a regular
     *     expression that matches a User-Agent header value.  When a matching
     *     REGISTER request is received,
     */
    static const char ConfigPrefix[];

    /**
     *
     *
     */
     static boost::format MessageFormat;

};

#endif // SIPIMPLSUB_H
