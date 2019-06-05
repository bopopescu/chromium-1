// This file is generated by PAL generator, do not modify.
// To make changes, modify the source file:
// test2.json

#ifndef PAL_PUBLIC_INTERFACES_INTENT_AGENT_INTERFACE_H_
#define PAL_PUBLIC_INTERFACES_INTENT_AGENT_INTERFACE_H_

#include "base/callback.h"
#include "pal/ipc/pal_export.h"

namespace pal {
// The callback is called to notify client with recieved service response
using InvokeIntentRespondCallback =
    base::Callback<void(int error_code, const std::string& data)>;

// This interface provides a framework which lets one web application
// (intent service) use its functionalities by other web applications
// (intent clients)
//
// Each platform has intent services registered as pairs of <action, type>
// These pairs are unique
//
// Introduced-by: LPD team

class PAL_EXPORT IntentAgentInterface {
 public:
  virtual ~IntentAgentInterface(){};

  // This function is called by an intent client when it wants to execute an
  // intent

  virtual void InvokeIntent(
      const std::string&
          action,  // Invoke action (e.g. "guide", "call", "nfcpush")
      const std::string& type,    // Invoke payload type (e.g. "coordinates",
                                  // "address", "tel", "pairAppInfo")
      const std::string& data,    // Payload
      const std::string& app_id,  // App id which invokes the intent
      const InvokeIntentRespondCallback& on_done) = 0;  // Callback to notify
                                                        // client with received
                                                        // service response

  // This function is called by an intent service when intent is completed
  // to provide response

  virtual void RespondIntent(bool result,          // True if success
                             uint32_t session_id,  // An ID provided by platform
                                                   // when launching intent
                                                   // service
                             const std::string& data) = 0;  // Response payload
};

}  // namespace pal

#endif  // PAL_PUBLIC_INTERFACES_INTENT_AGENT_INTERFACE_H_