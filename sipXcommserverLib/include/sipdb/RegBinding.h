/* 
 * File:   RegBinding.h
 * Author: joegen
 *
 * Created on January 10, 2011, 5:25 PM
 */

#ifndef RegBinding_H
#define	RegBinding_H


#include <string>
#include <boost/shared_ptr.hpp>
#include "sipdb/MongoDB.h"


class RegBinding
{
public:
    typedef boost::shared_ptr<RegBinding> Ptr;
    RegBinding();
    RegBinding(const RegBinding& binding);
    RegBinding(const MongoDB::BSONObj& bson);
    ~RegBinding();
    void swap(RegBinding& binding);
    RegBinding& operator=(const RegBinding& binding);
    RegBinding& operator=(const MongoDB::BSONObj& bson);
    const std::string& getIdentity() const;
    const std::string& getUri() const;
    const std::string& getCallId() const;
    const std::string& getContact() const;
    const std::string& getQvalue() const;
    const std::string& getInstanceId() const;
    const std::string& getGruu() const;
    const std::string& getPath() const;
    unsigned int getCseq() const;
    unsigned int getExpirationTime() const;
    const std::string& getInstrument() const;

    void setIdentity(const std::string& identity);
    void setUri(const std::string& uri);
    void setCallId(const std::string& callId);
    void setContact(const std::string& contact);
    void setQvalue(const std::string& qvalue);
    void setInstanceId(const std::string& instanceId);
    void setGruu(const std::string& gruu);
    void setPath(const std::string& path);
    void setCseq(unsigned int cseq);
    void setExpirationTime(unsigned int expirationTime);
    void setInstrument(const std::string& intrument);

private:
    std::string _identity;
    std::string _uri;
    std::string _callId;
    std::string _contact;
    std::string _qvalue;
    std::string _instanceId;
    std::string _gruu;
    std::string _path;
    unsigned int _cseq;
    unsigned int _expirationTime;
    std::string _instrument;
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

inline const std::string& RegBinding::getPath() const
{
  return _path;
}

inline unsigned int RegBinding::getCseq() const
{
  return _cseq;
}

inline unsigned int RegBinding::getExpirationTime() const
{
  return _expirationTime;
}

inline const std::string& RegBinding::getInstrument() const
{
  return _instrument;
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

inline void RegBinding::setPath(const std::string& path)
{
  _path = path;
}

inline void RegBinding::setCseq(unsigned int cseq)
{
  _cseq = cseq;
}

inline void RegBinding::setExpirationTime(unsigned int expirationTime)
{
  _expirationTime = expirationTime;
}

inline void RegBinding::setInstrument(const std::string& instrument)
{
  _instrument = instrument;
}

#endif	/* RegBinding_H */

