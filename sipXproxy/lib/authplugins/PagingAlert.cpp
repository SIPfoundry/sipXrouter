// 
// Copyright (C) 2009 Nortel, certain elements licensed under a Contributor Agreement.  
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
// 
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
// APPLICATION INCLUDES
#include <sipxproxy/SipRouter.h>
#include "os/OsLogger.h"
#include "net/SipXauthIdentity.h"
#include "PagingAlert.h"

// DEFINES
#define X_ALERT_INFO_HEADER "X-Alert-Info"
#define ALERT_INFO_HEADER "Alert-Info"

// CONSTANTS
// TYPEDEFS
// FORWARD DECLARATIONS

/// Factory used by PluginHooks to dynamically link the plugin instance
extern "C" AuthPlugin* getAuthPlugin(const UtlString& pluginName)
{
   return new PagingAlert(pluginName);
}

/// constructor
PagingAlert::PagingAlert(const UtlString& pluginName ///< the name for this instance
                                   )
   : AuthPlugin(pluginName),
     mpSipRouter( 0 )
{
   Os::Logger::instance().log(FAC_SIP,PRI_INFO,"PagingAlert plugin instantiated '%s'",
                 mInstanceName.data());
};

/// destructor
PagingAlert::~PagingAlert()
{
}

/// Read (or re-read) the authorization rules.
void
PagingAlert::readConfig( OsConfigDb& configDb /**< a subhash of the individual configuration
                                                    * parameters for this instance of this plugin. */
                             )
{
   // no configuration to read...
}

void 
PagingAlert::announceAssociatedSipRouter( SipRouter* pSipRouter )
{
   mpSipRouter = pSipRouter;
}

AuthPlugin::AuthResult
PagingAlert::authorizeAndModify(const UtlString& id,    /**< The authenticated identity of the
                                                              *   request originator, if any (the null
                                                              *   string if not).
                                                              *   This is in the form of a SIP uri
                                                              *   identity value as used in the
                                                              *   credentials database (user@domain)
                                                              *   without the scheme or any parameters.
                                                              */
                                     const Url&  requestUri, ///< parsed target Uri
                                     RouteState& routeState, ///< the state for this request.  
                                     const UtlString& method,///< the request method
                                     AuthResult  priorResult,///< results from earlier plugins.
                                     SipMessage& request,    ///< see AuthPlugin wrt modifying
                                     bool bSpiralingRequest, ///< request spiraling indication 
                                     UtlString&  reason      ///< rejection reason
                                     )
{
  AuthResult result = CONTINUE;
  if (   (priorResult != DENY) // no point in modifying a request that won't be sent
       && (method.compareTo(SIP_INVITE_METHOD) == 0) //We only operate on INVITEs
       )   
  {      
    replaceXAlertInfo(request);
  }

  return result;
}


bool 
PagingAlert::replaceXAlertInfo( SipMessage& request )
{
   bool bReplace = false;
   
   if( mpSipRouter )
   {
      const char* existingHeader = request.getHeaderValue(0, X_ALERT_INFO_HEADER);
      //If it exists we will only act if we are instructed to replace it.
      if (existingHeader != NULL) {
        request.setHeaderValue(ALERT_INFO_HEADER, existingHeader);
        while(request.removeHeader(X_ALERT_INFO_HEADER, 0))
        {
        }

        bReplace = true;
      }
   }

   return bReplace;
}


