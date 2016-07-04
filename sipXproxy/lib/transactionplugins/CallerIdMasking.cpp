
#include "CallerIdMasking.h"


#define SIPX_PLUGIN_PRIORITY 995
#define MONGO_COLLECTION "imdb.entity"
#define MCID_FROM "mcid_from"
#define MCID_EXT "mcid_ext"
#define MCID_NAME "mcid_name"

extern "C" SipBidirectionalProcessorPlugin* getTransactionPlugin(const UtlString& name)
{
  return new CallerIdMasking(name);
}

CallerIdMasking::CallerIdMasking(const UtlString& name) :
  MongoDB::BaseDB(MongoDB::ConnectionInfo::globalInfo(), MONGO_COLLECTION),
  SipBidirectionalProcessorPlugin(name, SIPX_PLUGIN_PRIORITY),
  _hasEmergencyRules(false)
{
}

CallerIdMasking::~CallerIdMasking()
{
}

void CallerIdMasking::readConfig(OsConfigDb& config)
{
  UtlString emergencyRules;
  if (config.get("EMERGRULES", emergencyRules) == OS_SUCCESS)
  {
    _hasEmergencyRules = (_emergencyRules.loadMappings(emergencyRules) == OS_SUCCESS);
  }
}

void CallerIdMasking::handleOutgoing(SipMessage& msg, const char* addr, int port)
{
  if (msg.isResponse())
  {
    return;
  }
  
  UtlString emergencyRuleName;
  UtlString emergencyRuleDescription;
  UtlString requestUri;
  msg.getRequestUri(&requestUri);
  Url requestUrl(requestUri.data(), TRUE);
  if (_hasEmergencyRules && _emergencyRules.getMatchedRule(requestUrl, emergencyRuleName, emergencyRuleDescription))
  {
    return;
  }
  
  Url fromUrl;
  msg.getFromUrl(fromUrl);
  UtlString fromUser;
	fromUrl.getUserId(fromUser);
  if (fromUser.isNull())
  {
    return;
  }
  
  UtlString mcid_ext;
  if (fromUrl.getUrlParameter(MCID_EXT, mcid_ext))
  {
    return;
  }
  
  std::string callerId;
  std::string callerIdDisplay;
  mongo::BSONObj query = BSON(MCID_FROM << fromUser);
  MongoDB::ScopedDbConnectionPtr conn(mongoMod::ScopedDbConnection::getScopedDbConnection(_info.getConnectionString().toString(), getReadQueryTimeout()));
  mongo::BSONObjBuilder builder;
  BaseDB::nearest(builder, query);
  mongo::BSONObj obj = conn->get()->findOne(_ns, builder.obj());

  if (!obj.isEmpty() && obj.hasField(MCID_EXT))
  {
    callerId = obj.getStringField(MCID_EXT);
    if (!callerId.empty())
    {
      fromUrl.setUrlParameter(MCID_EXT, fromUser.data());
      fromUrl.setUserId(callerId.c_str());
    }
  }

  if (!obj.isEmpty() && obj.hasField(MCID_NAME))
  {
    callerIdDisplay = obj.getStringField(MCID_NAME);
    if (!callerIdDisplay.empty())
    {
      std::ostringstream strm;
      strm << "\"" << callerIdDisplay << "\"";
      fromUrl.setDisplayName(strm.str().c_str());
      UtlString displayName;
      fromUrl.getDisplayName(displayName);
      if (!displayName.isNull())
      {
        fromUrl.setUrlParameter(MCID_NAME, displayName.data());
      }
    }
  }
  conn->done();
  
  if (!callerId.empty() || !callerIdDisplay.empty())
  {
    UtlString maskedHeader;
    fromUrl.toString(maskedHeader);
    msg.setRawFromField(maskedHeader.data());
  }
}

void CallerIdMasking::handleIncoming(SipMessage& msg, const char* addr, int port)
{
  Url fromUrl;
  msg.getFromUrl(fromUrl);
  UtlString mcid_ext;
  if (!fromUrl.getUrlParameter(MCID_EXT, mcid_ext))
  {
    return;
  }
  fromUrl.setUserId(mcid_ext.data());
  fromUrl.removeUrlParameter(MCID_EXT);
  
  UtlString mcid_name;
  if (fromUrl.getUrlParameter(MCID_NAME, mcid_name))
  {
    fromUrl.setDisplayName(mcid_name.data());
    fromUrl.removeUrlParameter(MCID_NAME);
  }
  
  UtlString maskedHeader;
  fromUrl.toString(maskedHeader);
  msg.setRawFromField(maskedHeader.data());
}
