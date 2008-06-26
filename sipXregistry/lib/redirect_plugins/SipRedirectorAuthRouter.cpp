// 
// 
// Copyright (C) 2007 Pingtel Corp., certain elements licensed under a Contributor Agreement.  
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
// 
// $$
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES
#include <stdlib.h>

// APPLICATION INCLUDES
#include "os/OsSysLog.h"
#include "net/Url.h"
#include "net/SipMessage.h"
#include "net/SignedUrl.h"
#include "sipXecsService/SipXecsService.h"
#include "sipXecsService/SharedSecret.h"
#include "SipRedirectorAuthRouter.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
#define CONFIG_SETTING_SIPX_PROXY "SIPX_PROXY"

// STATIC VARIABLE INITIALIZATIONS

// Static factory function.
extern "C" RedirectPlugin* getRedirectPlugin(const UtlString& instanceName)
{
   return new SipRedirectorAuthRouter(instanceName);
}

// Constructor
SipRedirectorAuthRouter::SipRedirectorAuthRouter(const UtlString& instanceName) :
   RedirectPlugin(instanceName)
{
   mLogName.append("[");
   mLogName.append(instanceName);
   mLogName.append("] SipRedirectorAuthRouter");
}

// Destructor
SipRedirectorAuthRouter::~SipRedirectorAuthRouter()
{
}

// Read config information.
void SipRedirectorAuthRouter::readConfig(OsConfigDb& configDb)
{
   UtlString authProxyConfig;
   if (   (OS_SUCCESS == configDb.get(CONFIG_SETTING_SIPX_PROXY, authProxyConfig) )
       && ! authProxyConfig.isNull()
       )
   {
      Url authUrl(authProxyConfig);
      if ( Url::SipUrlScheme == authUrl.getScheme() )
      {
         authUrl.setUserId(NULL);
         authUrl.setDisplayName(NULL);
         authUrl.removeFieldParameters();
         authUrl.removeHeaderParameters();
         authUrl.setUrlParameter("lr",NULL);

         authUrl.toString(mAuthUrl);
         
         OsSysLog::add(FAC_SIP,
                       (authProxyConfig.compareTo(mAuthUrl, UtlString::ignoreCase)
                        ? PRI_INFO : PRI_NOTICE),
                       "%s::readConfig authorization proxy route '%s'",
                       mLogName.data(), mAuthUrl.data()
                       );
      }
      else
      {
         OsSysLog::add(FAC_SIP, PRI_ERR, "%s::readConfig "
                       "invalid route '%s'", mLogName.data(), authProxyConfig.data());
      }
   }
   else
   {
      OsSysLog::add(FAC_SIP, PRI_INFO, "%s::readConfig "
                    "No authorization proxy specified", mLogName.data());
   }
}

// Initializer
OsStatus
SipRedirectorAuthRouter::initialize(OsConfigDb& configDb,
                                    SipUserAgent* pSipUserAgent,
                                    int redirectorNo,
                                    const UtlString& localDomainHost)
{
   // Get the secret to be used by signed Url
   OsConfigDb domainConfiguration;
   domainConfiguration.loadFromFile(SipXecsService::domainConfigPath());
   UtlString* sharedSecret = new SharedSecret(domainConfiguration);   
   SignedUrl::setSecret(sharedSecret->data());
   delete sharedSecret;
   return OS_SUCCESS;
}

// Finalizer
void
SipRedirectorAuthRouter::finalize()
{
}

RedirectPlugin::LookUpStatus SipRedirectorAuthRouter::lookUp(
   const SipMessage& message,
   const UtlString& requestString,
   const Url& requestUri,
   const UtlString& method,
   SipMessage& response,
   RequestSeqNo requestSeqNo,
   int redirectorNo,
   SipRedirectorPrivateStorage*& privateStorage)
{
   RedirectPlugin::LookUpStatus lookupStatus = RedirectPlugin::LOOKUP_SUCCESS; // always, so far
   
   // Do the cheap global tests first
   //   Is there an authorization proxy route?
   //   Is the request method INVITE? (This operates only on INVITEs)
   //   Does the response have any Contacts? (If not, there's nothing to do.)
   if (!mAuthUrl.isNull())
   {
      int contacts = response.getCountHeaderFields(SIP_CONTACT_FIELD);
      if (   (method.compareTo(SIP_INVITE_METHOD, UtlString::ignoreCase) == 0)
          && (contacts)
          )
      {
         /*
          * Loop through each contact in the response,
          *   checking to see if the contact includes a route set
          */
         UtlString contact;
         for (int contactNumber = 0;
              response.getContactEntry(contactNumber, &contact);
              contactNumber++
              )
         {
            Url contactUri(contact);
            
            OsSysLog::add(FAC_SIP, PRI_DEBUG,
                          "%s::lookUp contact %d '%s'",
                          mLogName.data(), contactNumber, contact.data()
                          );

            // Prepend sipXproxy route to Route header parameter 
            // to ensure that sipXproxy sees the INVITE resulting 
            // from 302 Moved Temporarily recursion unless a 
            // signed route header already exists. Please refer to 
            // SipRedirectorAuthRouter.h for more details.
            UtlString checkedRoute(mAuthUrl);
            bool bAddRouteToSipXProxy;
            UtlString routeValue;
            
            if ( contactUri.getHeaderParameter(SIP_ROUTE_FIELD, routeValue) )
            {
               OsSysLog::add(FAC_SIP, PRI_DEBUG,
                             "%s::lookUp contact %d Route value is '%s'",
                             mLogName.data(), contactNumber, routeValue.data() );
               
               // Check if the first entry of the Route header parameter carries a valid
               // signature.URL parameter.  If it does, this indicates that the Route
               // was added by a trusted sipXproxy component, as such there is no need
               // to add a Route to the sipXproxy.  
               Url routeUrl( routeValue );
               if( SignedUrl::isUrlSigned(  routeUrl ) )
               {
                  OsSysLog::add(FAC_SIP, PRI_DEBUG,
                                "%s::lookUp contact %d Route not properly signed - no new route added",
                                mLogName.data(), contactNumber );               
                  bAddRouteToSipXProxy = false;
               }
               else
               {
                  // there is already a Route header parameter in the contact but it is not signed
                  // append it to the sipXproxy route to make sure it sees INVITE resulting from 
                  // 302 recursion.
                  checkedRoute.append(SIP_MULTIFIELD_SEPARATOR);
                  checkedRoute.append(routeValue);
                  contactUri.setHeaderParameter(SIP_ROUTE_FIELD, checkedRoute);
                  bAddRouteToSipXProxy = true;                  
               }
            }
            else
            {
               contactUri.setHeaderParameter(SIP_ROUTE_FIELD, checkedRoute);
               bAddRouteToSipXProxy = true;
            }
            
            if( bAddRouteToSipXProxy )
            {
               // and put the modified contact back into the message
               UtlString modifiedContact;
               contactUri.toString(modifiedContact);
               response.setContactField(modifiedContact, contactNumber);
               OsSysLog::add(FAC_SIP, PRI_DEBUG,
                             "%s::lookUp modified:\n"
                             "   '%s'\n"
                             "in '%s'\n"
                             "to '%s'\n"
                             "in '%s'\n",
                             mLogName.data(),
                             routeValue.data(), contact.data(),
                             checkedRoute.data(), modifiedContact.data()
                             );
            }
         } // loop over all contacts
      }
      else
      {
         // request is not an INVITE, or no Contact headers in the response
         OsSysLog::add(FAC_SIP, PRI_DEBUG,
                       "%s::lookUp "
                       "'%s' request is not an INVITE or has no response contacts (%d) - ignored.",
                       mLogName.data(), method.data(), contacts
                       );
      }
   }
   else
   {
      OsSysLog::add(FAC_SIP, PRI_DEBUG, "%s::lookup No authproxy configured",
                    mLogName.data());
   }

   return lookupStatus;
}
