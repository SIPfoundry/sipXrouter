/* 
 * File:   SubscribeRecord.h
 * Author: joegen
 *
 * Created on February 2, 2011, 4:12 PM
 */

#ifndef SUBSCRIBERECORD_H
#define	SUBSCRIBERECORD_H


#include <string>
#include <boost/shared_ptr.hpp>
#include "sipdb/MongoDB.h"


class Subscription
{
public:
    Subscription();
    Subscription(const Subscription& subscription);
    Subscription(const MongoDB::BSONObj& bson);
    Subscription(
        const UtlString& component,
        const UtlString& uri,
        const UtlString& callId,
        const UtlString& contact,
        unsigned int expires,
        unsigned int subscribeCseq,
        const UtlString& eventTypeKey,
        const UtlString& eventType,
        const UtlString& id,
        const UtlString& toUri,
        const UtlString& fromUri,
        const UtlString& key,
        const UtlString& recordRoute,
        unsigned int notifyCseq,
        const UtlString& accept,
        unsigned int version);

    ~Subscription();
    Subscription& operator=(const Subscription& subscription);
    Subscription& operator=(const MongoDB::BSONObj& bson);
    void swap(Subscription& subscription);
    std::string& oid();
    std::string& component();
    std::string& uri();
    std::string& callId();
    std::string& contact();
    std::string& eventTypeKey();
    std::string& eventType();
    std::string& id();
    std::string& toUri();
    std::string& fromUri();
    std::string& key();
    std::string& recordRoute();
    std::string& accept();
    std::string& file();
    unsigned int notifyCseq();
    unsigned int subscribeCseq();
    unsigned int version();  
    unsigned int expires();



    static const char* oid_fld();
    static const char* component_fld();
    static const char* uri_fld();
    static const char* callId_fld();
    static const char* contact_fld();
    static const char* notifyCseq_fld();
    static const char* subscribeCseq_fld();
    static const char* eventTypeKey_fld();
    static const char* eventType_fld();
    static const char* id_fld();
    static const char* toUri_fld();
    static const char* fromUri_fld();
    static const char* key_fld();
    static const char* recordRoute_fld();
    static const char* accept_fld();
    static const char* file_fld();
    static const char* version_fld();
    static const char* expires_fld();

private:
    std::string _oid;
    std::string _component;
    std::string _uri;
    std::string _callId;
    std::string _contact;
    std::string _eventTypeKey;
    std::string _eventType;
    std::string _id; // id param from event header
    std::string _toUri;
    std::string _fromUri;
    std::string _key;
    std::string _recordRoute;
    std::string _accept;
    std::string _file;
    unsigned int _notifyCseq;
    unsigned int _subscribeCseq;
    unsigned int _version;              // Version no inside generated XML.
    unsigned int _expires; // Absolute expiration time secs since 1/1/1970
};

//
// Inlines
//

inline std::string& Subscription::oid()
{
    return _oid;
}


inline std::string& Subscription::component()
{
    return _component;
}

inline std::string& Subscription::uri()
{
    return _uri;
}

inline std::string& Subscription::callId()
{
    return _callId;
}

inline std::string& Subscription::contact()
{
    return _contact;
}

inline std::string& Subscription::eventTypeKey()
{
    return _eventTypeKey;
}

inline std::string& Subscription::eventType()
{
    return _eventType;
}

inline std::string& Subscription::id()
{
    return _id;
}

inline std::string& Subscription::toUri()
{
    return _toUri;
}

inline std::string& Subscription::fromUri()
{
    return _fromUri;
}

inline std::string& Subscription::key()
{
    return _key;
}

inline std::string& Subscription::recordRoute()
{
    return _recordRoute;
}

inline std::string& Subscription::accept()
{
    return _accept;
}

inline unsigned int Subscription::version()
{
    return _version;
}

inline unsigned int Subscription::expires()
{
    return _expires;
}

inline std::string& Subscription::file()
{
    return _file;
}

inline unsigned int Subscription::notifyCseq()
{
    return _notifyCseq;
}

inline unsigned int Subscription::subscribeCseq()
{
    return _subscribeCseq;
}

#endif	/* SUBSCRIBERECORD_H */

