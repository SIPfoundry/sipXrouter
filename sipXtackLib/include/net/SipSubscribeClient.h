// 
// Copyright (C) 2007 Pingtel Corp., certain elements licensed under a Contributor Agreement.  
// Contributors retain copyright to elements licensed under a Contributor Agreement.
// Licensed to the User under the LGPL license.
// 
// $$

//////////////////////////////////////////////////////////////////////////////
#ifndef _SipSubscribeClient_h_
#define _SipSubscribeClient_h_

// SYSTEM INCLUDES

// APPLICATION INCLUDES

#include <os/OsDefs.h>
#include <os/OsServerTask.h>
#include <utl/UtlHashBag.h>
#include <net/SipRefreshManager.h>

// DEFINES
// MACROS
// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STRUCTS
// FORWARD DECLARATIONS
class SipMessage;
class SipUserAgent;
class SipDialogMgr;
class SipRefreshManager;
class SubscribeClientState;

// TYPEDEFS

//! Class for maintaining the subscriber role of SIP subscriptions
/** Once a SipSubscribeClient has been created, it can be directed to
 *  create and maintain subscriptions.  SUBSCRIBE messages are sent
 *  using addSubscription.  Each use of addSubscription creates an
 *  early dialog, and any subscriptions that are established are
 *  created as established dialogs.  The creation and destruction of
 *  early and established dialogs is reported through a callback
 *  function, and the contents of any NOTIFYs received in the
 *  subscriptions is reported through another callback function.
 *
 *  Note that if an established subscription is ended due to error or
 *  action of the notifier, SipSubscribeClient does not attempt to
 *  re-create it.
 */
class SipSubscribeClient : public OsServerTask
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

    enum SubscriptionState
    {
        SUBSCRIPTION_INITIATED, // Early dialog
        SUBSCRIPTION_SETUP,     // Established dialog
        SUBSCRIPTION_TERMINATED // Ended dialog
    };

    /// Type of the callback function for reporting subscription state changes.
    /** Note SipSubscribeClient does automatically refresh subscriptions, but
     *  if the subscription is involuntarily ended (by the notifier or otherwise),
     *  SipSubscribeClient will not attempt to reestablish it.
     */
    typedef void (*SubscriptionStateCallback)
       (SipSubscribeClient::SubscriptionState newState,
        const char* earlyDialogHandle,
        const char* dialogHandle,
        ///< Either earlyDialogHandle or dialogHandle will be set
        void* applicationData,
        int responseCode,
        const char* responseText,
        /**< responseCode and responseTest will be from the relevant failure
         *   response, or -1 and NULL, respectively.
         */
        long expiration,
        ///< current expiration length of the subscription, or -1 if not known
        const SipMessage* subscribeResponse
        ///< relevent response message, or NULL
        );

    /// Type of the callback function for reporting received NOTIFYs.
    /** Note that NOTIFYs are generally reported in the order they are
     *  received.  Duplicated NOTIFYs are not reported, as the duplicated
     *  messages are removed in the SIP stack.  Similarly, out-of-order
     *  NOTIFYs are rejected in the SIP stack and are not reported.
     *  This provides the desired behavior if the notifications for a
     *  subscription give a complete update of the subscription's
     *  state.  If notifications only give partial updates, and so
     *  missed NOTIFYs must be detected, the NOTIFYs need to contain
     *  serial numbers (as many SIP event packages do), as the CSeq
     *  numbers cannot be relied upon to be sequential.
     *  (If the application needs a full update, it must direct
     *  SipSubscribeClient to terminate the subscription and begin a
     *  new one.)
     */
    typedef void (*NotifyEventCallback)
       (const char* earlyDialogHandle,
        /**< handle of the early dialog generated
         *   by the SUBSCRIBE.
         */
        const char* dialogHandle,
        /**< handle of the established
         *   dialog generated by the first NOTIFY.
         */
        void* applicationData,
        const SipMessage* notifyRequest);

/* ============================ CREATORS ================================== */

    //! Default Dialog constructor
    SipSubscribeClient(SipUserAgent& userAgent, 
                       SipDialogMgr& dialogMgr,
                       SipRefreshManager& refreshMgr);

    //! Destructor
    virtual
    ~SipSubscribeClient();

/* ============================ MANIPULATORS ============================== */

    //! Create a SIP event subscription based on values specified to create a SUBSCRIBE message
    /*! 
     *  Returns TRUE if the SUBSCRIBE request was sent and the 
     *  Subscription state proceeded to SUBSCRIPTION_INITIATED.
     *  Returns FALSE if the SUBSCRIBE request was not able to
     *  be sent, the subscription state is set to SUSCRIPTION_FAILED.
     */
    UtlBoolean addSubscription(const char* resourceId,
                               /**< The request-URI for the SUBSCRIBE.
                                *   The request-URI as it arrives at the
                                *   notifier will (conventionally) be used
                                *   (without parameters) as the
                                *   resourceId to subscribe to.
                                */
                               const char* eventHeaderValue,
                               //< value of the Event header
                               const char* acceptHeaderValue,
                               //< value of the Accept header
                               const char* fromFieldValue,
                               //< value (name-addr) of the From header
                               const char* toFieldValue,
                               //< value (name-addr) of the To header
                               const char* contactFieldValue,
                               /**< Value of the Contact header.
                                *   Must route to this UA.
                                */
                               int subscriptionPeriodSeconds,
                               //< subscription period to request
                               void* applicationData,
                               //< application data to pass to the callback funcs.
                               const SubscriptionStateCallback subscriptionStateCallback,
                               //< callback function for begin/end of subscriptions
                               const NotifyEventCallback notifyEventsCallback,
                               //< callback function for incoming NOTIFYs
                               UtlString& earlyDialogHandle
                               /**< returned handle for the early
                                *   dialog created by the SUBSCRIBE
                                */
       );

    //! Create a SIP event subscription based on a provided SUBSCRIBE request
    /*! 
     *  Returns TRUE if the SUBSCRIBE request was sent and the 
     *  Subscription state proceeded to SUBSCRIPTION_INITIATED.
     *  Returns FALSE if the SUBSCRIBE request was not able to
     *  be sent, the subscription state is set to SUSCRIPTION_FAILED.
     */
    UtlBoolean addSubscription(SipMessage& subscriptionRequest,
                               void* applicationData,
                               const SubscriptionStateCallback subscriptionStateCallback,
                               const NotifyEventCallback notifyEventsCallback,
                               UtlString& earlyDialogHandle);

    //! End the SIP event subscription indicated by the dialog handle
    /*! If the given dialogHandle is an early dialog it will end any
     *  established or early dialog subscriptions created by the
     *  addSubscription() that returned the early dialog handle.
     *  Typically the application should use an established dialog
     *  handle, which terminates the specified established subscription.
     */
    UtlBoolean endSubscription(const char* dialogHandle);

    //! End all subscriptions known by the SipSubscribeClient.
    void endAllSubscriptions();

    /** Change the subscription expiration period.
     * Send a new SUBSCRIBE with a different timeout value, and set refresh timers appropriately.
     * All future re-SUBSCRIBEs (until the next change) will use the new timeout value.
     */
    UtlBoolean changeSubscriptionTime(const char* earlyDialogHandle, int subscriptionPeriodSeconds);

    //! Handler for NOTIFY requests
    UtlBoolean handleMessage(OsMsg &eventMessage);

/* ============================ ACCESSORS ================================= */

    //! Create a debug dump of all the client states
    int dumpStates(UtlString& dumpString);

    //! Get a string representation of an 'enum SubscriptionState' value.
    static void getSubscriptionStateEnumString(enum SubscriptionState stateValue, 
                                               UtlString& stateString);

/* ============================ INQUIRY =================================== */

    //! Get a count of the subscriptions which currently exist.
    int countSubscriptions();

    //! Dump the object's internal state.
    void dumpState();

/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:

/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
    //! Callback to handle subscription state changes from the refresh manager
    /*! RefreshStateCallback
     */
    static void refreshCallback(SipRefreshManager::RefreshRequestState newState,
                               const char* earlyDialogHandle,
                               const char* dialogHandle,
                               void* subscribeClientPtr,
                               int responseCode,
                               const char* responseText,
                               long expiration, // epoch seconds
                               const SipMessage* subscribeResponse);

    //! Method version of callback function for Refresh Manager.
    void refreshCallback(SipRefreshManager::RefreshRequestState newState,
                         const char* earlyDialogHandle,
                         const char* dialogHandle,
                         int responseCode,
                         const char* responseText,
                         long expiration, // epoch seconds
                         const SipMessage* subscribeResponse);

    //! Handle incoming notify request
    void handleNotifyRequest(const SipMessage& notifyRequest);

    //! Add the client state to mSubscriptions.
    void addState(SubscribeClientState& clientState);

    //! find the state from mSubscriptions that matches the dialog
    /* Assumes external locking
     */
    SubscribeClientState* getState(const UtlString& dialogHandle);

    //! remove the state from mSubscriptions that matches the dialog
    /* Assumes external locking
     */
    SubscribeClientState* removeState(const UtlString& dialogHandle);

    //! Copy constructor NOT ALLOWED
    SipSubscribeClient(const SipSubscribeClient& rSipSubscribeClient);

    //! Assignment operator NOT ALLOWED
    SipSubscribeClient& operator=(const SipSubscribeClient& rhs);

    SipUserAgent* mpUserAgent;
    SipDialogMgr* mpDialogMgr;
    SipRefreshManager* mpRefreshManager;
    UtlHashBag mSubscriptions; // state info. for each subscription
    // SIP event types that we are currently a SipUserAgent message observer for.
    UtlHashBag mEventTypes;
    // Protects mSubscriptions and mEventTypes.
    OsBSem mSubscribeClientMutex;
};

/* ============================ INLINE METHODS ============================ */

#endif  // _SipSubscribeClient_h_
