// This file is generated by PAL generator, do not modify.
// To make changes, modify the source file:
// test2.json

#include "pal/ipc/browser/intent_agent/intent_agent_host.h"

#include "base/bind.h"
#include "pal/ipc/intent_agent_messages.h"
#include "pal/public/pal.h"
#include "pal/public/pal_factory.h"

namespace pal {

IntentAgentHost::IntentAgentHost()
    : content::BrowserMessageFilter(IntentAgentMsgStart),
      weak_ptr_factory_(this) {}

IntentAgentHost::~IntentAgentHost() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

// clang-format off
bool IntentAgentHost::OnMessageReceived(const IPC::Message& message) {
  int routing_id = message.routing_id();
  bool handled = true;

  IPC_BEGIN_MESSAGE_MAP_WITH_PARAM(
      IntentAgentHost, message, &routing_id)
    IPC_MESSAGE_HANDLER(IntentAgentHostMsg_InvokeIntent, OnInvokeIntent)
    IPC_MESSAGE_HANDLER(IntentAgentHostMsg_RespondIntent, OnRespondIntent)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}
// clang-format on

void IntentAgentHost::OnInvokeIntentDone(int routing_id,
                                         int pal_async_callback_id,
                                         int32_t callback_index,
                                         int error_code,
                                         const std::string& data) {
  Send(new IntentAgentMsg_InvokeIntentDone(routing_id, pal_async_callback_id,
                                           callback_index, error_code, data));
}

void IntentAgentHost::OnInvokeIntent(int* routing_id,
                                     int pal_async_callback_id,
                                     const std::string& action,
                                     const std::string& type,
                                     const std::string& data,
                                     const std::string& app_id,
                                     int32_t callback_index) {
  IntentAgentInterface* interface =
      pal::GetInstance()->GetIntentAgentInterface();

  if (interface != NULL) {
    interface->InvokeIntent(
        action, type, data, app_id,
        base::Bind(&IntentAgentHost::OnInvokeIntentDone,
                   weak_ptr_factory_.GetWeakPtr(), *routing_id,
                   pal_async_callback_id, callback_index));
  } else {
    LOG(ERROR) << "Interface not available";
  }
}

void IntentAgentHost::OnRespondIntent(int* routing_id,
                                      bool result,
                                      uint32_t session_id,
                                      const std::string& data) {
  IntentAgentInterface* interface =
      pal::GetInstance()->GetIntentAgentInterface();

  if (interface != NULL) {
    interface->RespondIntent(result, session_id, data);
  } else {
    LOG(ERROR) << "Interface not available";
  }
}

}  // namespace pal