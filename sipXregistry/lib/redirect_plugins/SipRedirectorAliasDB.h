//
//
// Copyright (C) 2007 Pingtel Corp., certain elements licensed under a Contributor Agreement.
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
//
// $$
//////////////////////////////////////////////////////////////////////////////

#ifndef SIPREDIRECTORALIASDB_H
#define SIPREDIRECTORALIASDB_H

// SYSTEM INCLUDES
//#include <...>

// APPLICATION INCLUDES
#include "registry/RedirectPlugin.h"

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// TYPEDEFS
// FORWARD DECLARATIONS


#ifndef ALIASDB_RELATION_USERFORWARD
#define ALIASDB_RELATION_USERFORWARD "userforward"
#endif

/**
 * SipRedirectorAliasDB is singleton class whose object adds contacts that are
 * listed in the registration database.
 */

class SipRedirectorAliasDB : public RedirectPlugin
{
  public:

   explicit SipRedirectorAliasDB(const UtlString& instanceName);

   ~SipRedirectorAliasDB();

   virtual void readConfig(OsConfigDb& configDb);

   virtual OsStatus initialize(OsConfigDb& configDb,
                               int redirectorNo,
                               const UtlString& localDomainHost);

   virtual void finalize();

   virtual RedirectPlugin::LookUpStatus lookUp(
      const SipMessage& message,
      UtlString& requestString,
      Url& requestUri,
      const UtlString& method,
      ContactList& contactList,
      RequestSeqNo requestSeqNo,
      int redirectorNo,
      SipRedirectorPrivateStorage*& privateStorage,
      ErrorDescriptor& errorDescriptor);

   virtual const UtlString& name( void ) const;

  private:

    bool resolveAlias(
       const SipMessage& message,
       UtlString& requestString,
       Url& requestUri
    );

   // String to use in place of class name in log messages:
   // "[instance] class".
   UtlString mLogName;
   UtlBoolean _enableDiversionHeader;
   UtlBoolean _enableEarlyAliasResolution;
  protected:
};

#endif // SIPREDIRECTORALIASDB_H
