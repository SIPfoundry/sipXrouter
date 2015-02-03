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

#ifndef RegBinding_H
#define	RegBinding_H


#include <string>
#include <boost/shared_ptr.hpp>
#include "sipdb/MongoDB.h"
#include "sipdb/MongoMod.h"

class RegBinding
{
public:
    typedef boost::shared_ptr<RegBinding> Ptr;
    RegBinding();
    RegBinding(const RegBinding& binding);
    RegBinding(const mongo::BSONObj& bson);
    ~RegBinding();
    void swap(RegBinding& binding);
    RegBinding& operator=(const RegBinding& binding);
    RegBinding& operator=(const mongo::BSONObj& bson);
    const std::string& getIdentity() const;
    const std::string& getUri() const;
    const std::string& getCallId() const;
    const std::string& getContact() const;
    const std::string& getBinding() const;
    const std::string& getQvalue() const;
    const std::string& getInstanceId() const;
    const std::string& getGruu() const;
    unsigned int getShardId() const;
    const std::string& getPath() const;
    unsigned int getCseq() const;
    unsigned long getExpirationTime() const;
    const std::string& getInstrument() const;
    const std::string& getLocalAddress()const;
    unsigned long getTimestamp() const;
    bool getExpired() const;

    void setIdentity(const std::string& identity);
    void setUri(const std::string& uri);
    void setCallId(const std::string& callId);
    void setContact(const std::string& contact);
    void setBinding(const std::string& binding);
    void setQvalue(const std::string& qvalue);
    void setInstanceId(const std::string& instanceId);
    void setGruu(const std::string& gruu);
    void setShardId(unsigned int shardId);
    void setPath(const std::string& path);
    void setCseq(unsigned int cseq);
    void setExpirationTime(unsigned long expirationTime);
    void setInstrument(const std::string& intrument);
    void setLocalAddress(const std::string& localAddress);
    void setTimestamp(unsigned long timestamp);
    void setExpired(bool expired);

    static const char* identity_fld();
    static const char* uri_fld();
    static const char* callId_fld();
    static const char* contact_fld();
    static const char* binding_fld();
    static const char* qvalue_fld();
    static const char* instanceId_fld();
    static const char* gruu_fld();
    static const char* path_fld();
    static const char* cseq_fld();
    static const char* expirationTime_fld();
    static const char* instrument_fld();
    static const char* localAddress_fld();
    static const char* timestamp_fld();
    static const char* expired_fld();
    static const char* shardId_fld();

private:
    std::string _identity;
    std::string _uri;
    std::string _callId;
    std::string _contact;
    std::string _binding;
    std::string _qvalue;
    std::string _instanceId;
    std::string _gruu;
    unsigned int _shardId;
    std::string _path;
    unsigned int _cseq;
    unsigned long _expirationTime;
    std::string _instrument;
    std::string _localAddress;
    unsigned long _timestamp;
    bool _expired;
};

//
// Inlines
//


inline const std::string& RegBinding::getIdentity() const
{
  return _identity;
}

inline const std::string& RegBinding::getUri() const
{
  return _uri;
}

inline const std::string& RegBinding::getCallId() const
{
  return _callId;
}

inline const std::string& RegBinding::getContact() const
{
  return _contact;
}

inline const std::string& RegBinding::getBinding() const
{
  return _binding;
}

inline const std::string& RegBinding::getQvalue() const
{
  return _qvalue;
}

inline const std::string& RegBinding::getInstanceId() const
{
  return _instanceId;
}

inline const std::string& RegBinding::getGruu() const
{
  return _gruu;
}

inline unsigned int RegBinding::getShardId() const
{
  return _shardId;
}

inline const std::string& RegBinding::getPath() const
{
  return _path;
}

inline unsigned int RegBinding::getCseq() const
{
  return _cseq;
}

inline unsigned long RegBinding::getExpirationTime() const
{
  return _expirationTime;
}

inline const std::string& RegBinding::getInstrument() const
{
  return _instrument;
}

inline const std::string& RegBinding::getLocalAddress()const
{
    return _localAddress;
}

inline unsigned long RegBinding::getTimestamp() const
{
    return _timestamp;
}

inline bool RegBinding::getExpired() const
{
    return _expired;
}

inline void RegBinding::setIdentity(const std::string& identity)
{
  _identity = identity;
}

inline void RegBinding::setUri(const std::string& uri)
{
  _uri = uri;
}

inline void RegBinding::setCallId(const std::string& callId)
{
  _callId = callId;
}

inline void RegBinding::setContact(const std::string& contact)
{
  _contact = contact;
}

inline void RegBinding::setBinding(const std::string& binding)
{
  _binding = binding;
}

inline void RegBinding::setQvalue(const std::string& qvalue)
{
  _qvalue = qvalue;
}

inline void RegBinding::setInstanceId(const std::string& instanceId)
{
  _instanceId = instanceId;
}

inline void RegBinding::setGruu(const std::string& gruu)
{
  _gruu = gruu;
}

inline void RegBinding::setShardId(unsigned int shardId)
{
  _shardId = shardId;
}

inline void RegBinding::setPath(const std::string& path)
{
  _path = path;
}

inline void RegBinding::setCseq(unsigned int cseq)
{
  _cseq = cseq;
}

inline void RegBinding::setExpirationTime(unsigned long expirationTime)
{
  _expirationTime = expirationTime;
}

inline void RegBinding::setInstrument(const std::string& instrument)
{
  _instrument = instrument;
}

inline void RegBinding::setLocalAddress(const std::string& localAddress)
{
    _localAddress = localAddress;
}

inline void RegBinding::setTimestamp(unsigned long timestamp)
{
    _timestamp = timestamp;
}

inline void RegBinding::setExpired(bool expired)
{
    _expired = expired;
}

#endif	/* RegBinding_H */

