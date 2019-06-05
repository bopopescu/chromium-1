// This file is generated by PAL generator, do not modify.
// To make changes, modify the source file:
// test1.json

#include "pal/ipc/renderer/system_info/system_info_proxy.h"
#include "pal/ipc/system_info_messages.h"

namespace pal {

SystemInfoProxy::SystemInfoProxy(content::RenderFrame* frame)
    : render_frame_(frame) {}

void SystemInfoProxy::GetSSLCertPath(std::string& pal_ret) {
  if (render_frame_) {
    render_frame_->Send(new SystemInfoHostMsg_GetSSLCertPath(
        render_frame_->GetRoutingID(), &pal_ret));
  }
}

}  // namespace pal
