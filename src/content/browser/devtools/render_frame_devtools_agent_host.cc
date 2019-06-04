// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/render_frame_devtools_agent_host.h"

#include <tuple>
#include <utility>

#include "base/json/json_reader.h"
#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "build/build_config.h"
#include "content/browser/bad_message.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "content/browser/devtools/devtools_frame_trace_recorder.h"
#include "content/browser/devtools/devtools_manager.h"
#include "content/browser/devtools/devtools_renderer_channel.h"
#include "content/browser/devtools/devtools_session.h"
#include "content/browser/devtools/protocol/browser_handler.h"
#include "content/browser/devtools/protocol/dom_handler.h"
#include "content/browser/devtools/protocol/emulation_handler.h"
#include "content/browser/devtools/protocol/input_handler.h"
#include "content/browser/devtools/protocol/inspector_handler.h"
#include "content/browser/devtools/protocol/io_handler.h"
#include "content/browser/devtools/protocol/memory_handler.h"
#include "content/browser/devtools/protocol/network_handler.h"
#include "content/browser/devtools/protocol/page_handler.h"
#include "content/browser/devtools/protocol/protocol.h"
#include "content/browser/devtools/protocol/schema_handler.h"
#include "content/browser/devtools/protocol/security_handler.h"
#include "content/browser/devtools/protocol/service_worker_handler.h"
#include "content/browser/devtools/protocol/storage_handler.h"
#include "content/browser/devtools/protocol/target_handler.h"
#include "content/browser/devtools/protocol/tracing_handler.h"
#include "content/browser/frame_host/navigation_handle_impl.h"
#include "content/browser/frame_host/navigation_request.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/site_instance_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/browser/web_package/signed_exchange_envelope.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/web_contents_delegate.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/devtools_agent.mojom.h"

#if defined(OS_ANDROID)
#include "content/browser/renderer_host/compositor_impl_android.h"
#include "content/public/browser/render_widget_host_view.h"
#include "services/device/public/mojom/wake_lock_context.mojom.h"
#endif

namespace content {

namespace {
using RenderFrameDevToolsMap =
    std::map<FrameTreeNode*, RenderFrameDevToolsAgentHost*>;
base::LazyInstance<RenderFrameDevToolsMap>::Leaky g_agent_host_instances =
    LAZY_INSTANCE_INITIALIZER;

RenderFrameDevToolsAgentHost* FindAgentHost(FrameTreeNode* frame_tree_node) {
  if (!g_agent_host_instances.IsCreated())
    return nullptr;
  auto it = g_agent_host_instances.Get().find(frame_tree_node);
  return it == g_agent_host_instances.Get().end() ? nullptr : it->second;
}

bool ShouldCreateDevToolsForHost(RenderFrameHost* rfh) {
  return rfh->IsCrossProcessSubframe() || !rfh->GetParent();
}

bool ShouldCreateDevToolsForNode(FrameTreeNode* ftn) {
  return !ftn->parent() || ftn->current_frame_host()->IsCrossProcessSubframe();
}

FrameTreeNode* GetFrameTreeNodeAncestor(FrameTreeNode* frame_tree_node) {
  while (frame_tree_node && !ShouldCreateDevToolsForNode(frame_tree_node))
    frame_tree_node = frame_tree_node->parent();
  DCHECK(frame_tree_node);
  return frame_tree_node;
}

}  // namespace

// static
scoped_refptr<DevToolsAgentHost>
DevToolsAgentHost::GetOrCreateFor(WebContents* web_contents) {
  FrameTreeNode* node =
      static_cast<WebContentsImpl*>(web_contents)->GetFrameTree()->root();
  // TODO(dgozman): this check should not be necessary. See
  // http://crbug.com/489664.
  if (!node)
    return nullptr;
  return RenderFrameDevToolsAgentHost::GetOrCreateFor(node);
}

// static
DevToolsAgentHostImpl* RenderFrameDevToolsAgentHost::GetFor(
    FrameTreeNode* frame_tree_node) {
  frame_tree_node = GetFrameTreeNodeAncestor(frame_tree_node);
  return FindAgentHost(frame_tree_node);
}

scoped_refptr<DevToolsAgentHost> RenderFrameDevToolsAgentHost::GetOrCreateFor(
    FrameTreeNode* frame_tree_node) {
  frame_tree_node = GetFrameTreeNodeAncestor(frame_tree_node);
  RenderFrameDevToolsAgentHost* result = FindAgentHost(frame_tree_node);
  if (!result)
    result = new RenderFrameDevToolsAgentHost(frame_tree_node);
  return result;
}

// static
scoped_refptr<DevToolsAgentHost>
RenderFrameDevToolsAgentHost::GetOrCreateForDangling(
    FrameTreeNode* frame_tree_node) {
  // Note that this method does not use FrameTreeNode::current_frame_host(),
  // since it is used while the frame host may not be set as current yet,
  // for example right before commit time.
  // So the caller must be sure that passed frame will indeed be a correct
  // devtools target (see ShouldCreateDevToolsForNode above).
  RenderFrameDevToolsAgentHost* result = FindAgentHost(frame_tree_node);
  if (!result)
    result = new RenderFrameDevToolsAgentHost(frame_tree_node);
  return result;
}

// static
scoped_refptr<DevToolsAgentHost> RenderFrameDevToolsAgentHost::FindForDangling(
    FrameTreeNode* frame_tree_node) {
  return FindAgentHost(frame_tree_node);
}

// static
bool DevToolsAgentHost::HasFor(WebContents* web_contents) {
  FrameTreeNode* node =
      static_cast<WebContentsImpl*>(web_contents)->GetFrameTree()->root();
  return node ? FindAgentHost(node) != nullptr : false;
}

// static
bool DevToolsAgentHost::IsDebuggerAttached(WebContents* web_contents) {
  FrameTreeNode* node =
      static_cast<WebContentsImpl*>(web_contents)->GetFrameTree()->root();
  RenderFrameDevToolsAgentHost* host = node ? FindAgentHost(node) : nullptr;
  return host && host->IsAttached();
}

// static
void RenderFrameDevToolsAgentHost::AddAllAgentHosts(
    DevToolsAgentHost::List* result) {
  for (WebContentsImpl* wc : WebContentsImpl::GetAllWebContents()) {
    for (FrameTreeNode* node : wc->GetFrameTree()->Nodes()) {
      if (!node->current_frame_host() || !ShouldCreateDevToolsForNode(node))
        continue;
      if (!node->current_frame_host()->IsRenderFrameLive())
        continue;
      result->push_back(RenderFrameDevToolsAgentHost::GetOrCreateFor(node));
    }
  }
}

// static
void RenderFrameDevToolsAgentHost::WebContentsCreated(
    WebContents* web_contents) {
  if (ShouldForceCreation()) {
    // Force agent host.
    DevToolsAgentHost::GetOrCreateFor(web_contents);
  }
}

// static
void RenderFrameDevToolsAgentHost::UpdateRawHeadersAccess(
    RenderFrameHostImpl* old_rfh,
    RenderFrameHostImpl* new_rfh) {
  DCHECK_NE(old_rfh, new_rfh);
  RenderProcessHost* old_rph = old_rfh ? old_rfh->GetProcess() : nullptr;
  RenderProcessHost* new_rph = new_rfh ? new_rfh->GetProcess() : nullptr;
  if (old_rph == new_rph)
    return;
  std::set<url::Origin> old_process_origins;
  std::set<url::Origin> new_process_origins;
  for (const auto& entry : g_agent_host_instances.Get()) {
    RenderFrameHostImpl* frame_host = entry.second->frame_host_;
    if (!frame_host)
      continue;
    // Do not skip the nodes if they're about to get attached.
    if (!entry.second->IsAttached() &&
        (!new_rfh || entry.first != new_rfh->frame_tree_node())) {
      continue;
    }
    RenderProcessHost* process_host = frame_host->GetProcess();
    if (process_host == old_rph)
      old_process_origins.insert(frame_host->GetLastCommittedOrigin());
    else if (process_host == new_rph)
      new_process_origins.insert(frame_host->GetLastCommittedOrigin());
  }
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService)) {
    if (old_rph && old_process_origins.empty()) {
      ChildProcessSecurityPolicyImpl::GetInstance()->RevokeReadRawCookies(
          old_rph->GetID());
    }
    if (new_rph && !new_process_origins.empty()) {
      ChildProcessSecurityPolicyImpl::GetInstance()->GrantReadRawCookies(
          new_rph->GetID());
    }
    return;
  }
  if (old_rph) {
    GetNetworkService()->SetRawHeadersAccess(
        old_rph->GetID(), std::vector<url::Origin>(old_process_origins.begin(),
                                                   old_process_origins.end()));
  }
  if (new_rph) {
    GetNetworkService()->SetRawHeadersAccess(
        new_rph->GetID(), std::vector<url::Origin>(new_process_origins.begin(),
                                                   new_process_origins.end()));
  }
}

RenderFrameDevToolsAgentHost::RenderFrameDevToolsAgentHost(
    FrameTreeNode* frame_tree_node)
    : DevToolsAgentHostImpl(frame_tree_node->devtools_frame_token().ToString()),
      frame_tree_node_(nullptr) {
  SetFrameTreeNode(frame_tree_node);
  frame_host_ = frame_tree_node->current_frame_host();
  render_frame_alive_ = frame_host_ && frame_host_->IsRenderFrameLive();
  AddRef();  // Balanced in DestroyOnRenderFrameGone.
  NotifyCreated();
}

void RenderFrameDevToolsAgentHost::SetFrameTreeNode(
    FrameTreeNode* frame_tree_node) {
  if (frame_tree_node_)
    g_agent_host_instances.Get().erase(frame_tree_node_);
  frame_tree_node_ = frame_tree_node;
  if (frame_tree_node_) {
    // TODO(dgozman): with ConnectWebContents/DisconnectWebContents,
    // we may get two agent hosts for the same FrameTreeNode.
    // That is definitely a bug, and we should fix that, and DCHECK
    // here that there is no other agent host.
    g_agent_host_instances.Get()[frame_tree_node] = this;
  }
  WebContentsObserver::Observe(
      frame_tree_node_ ? WebContentsImpl::FromFrameTreeNode(frame_tree_node_)
                       : nullptr);
}

BrowserContext* RenderFrameDevToolsAgentHost::GetBrowserContext() {
  WebContents* contents = web_contents();
  return contents ? contents->GetBrowserContext() : nullptr;
}

WebContents* RenderFrameDevToolsAgentHost::GetWebContents() {
  return web_contents();
}

bool RenderFrameDevToolsAgentHost::AttachSession(DevToolsSession* session) {
  if (!ShouldAllowSession(session))
    return false;

  protocol::EmulationHandler* emulation_handler =
      new protocol::EmulationHandler();
  session->AddHandler(base::WrapUnique(new protocol::BrowserHandler()));
  session->AddHandler(base::WrapUnique(
      new protocol::DOMHandler(session->client()->MayAffectLocalFiles())));
  session->AddHandler(base::WrapUnique(emulation_handler));
  session->AddHandler(base::WrapUnique(new protocol::InputHandler()));
  session->AddHandler(base::WrapUnique(new protocol::InspectorHandler()));
  session->AddHandler(base::WrapUnique(new protocol::IOHandler(
      GetIOContext())));
  session->AddHandler(base::WrapUnique(new protocol::MemoryHandler()));
  session->AddHandler(base::WrapUnique(new protocol::NetworkHandler(
      GetId(),
      frame_tree_node_ ? frame_tree_node_->devtools_frame_token()
                       : base::UnguessableToken(),
      GetIOContext())));
  session->AddHandler(base::WrapUnique(new protocol::SchemaHandler()));
  session->AddHandler(base::WrapUnique(new protocol::ServiceWorkerHandler()));
  session->AddHandler(base::WrapUnique(new protocol::StorageHandler()));
  session->AddHandler(base::WrapUnique(new protocol::TargetHandler(
      session->client()->MayAttachToBrowser()
          ? protocol::TargetHandler::AccessMode::kRegular
          : protocol::TargetHandler::AccessMode::kAutoAttachOnly,
      GetId(), GetRendererChannel(), session->GetRootSession())));
  session->AddHandler(base::WrapUnique(new protocol::PageHandler(
      emulation_handler, session->client()->MayAffectLocalFiles())));
  session->AddHandler(base::WrapUnique(new protocol::SecurityHandler()));
  if (!frame_tree_node_ || !frame_tree_node_->parent()) {
    session->AddHandler(base::WrapUnique(
        new protocol::TracingHandler(frame_tree_node_, GetIOContext())));
  }

  if (sessions().empty()) {
    bool use_video_capture_api = true;
#ifdef OS_ANDROID
    // Video capture API cannot be used on Android WebView.
    if (!CompositorImpl::IsInitialized())
      use_video_capture_api = false;
#endif
    // When video capture API is used, don't instantiate
    // DevToolsFrameTraceRecorder. Taking snapshots happens in TracingHandler.
    if (!use_video_capture_api)
      frame_trace_recorder_.reset(new DevToolsFrameTraceRecorder());
    UpdateRawHeadersAccess(nullptr, frame_host_);
#if defined(OS_ANDROID)
    GetWakeLock()->RequestWakeLock();
#endif
  }
  return true;
}

void RenderFrameDevToolsAgentHost::DetachSession(DevToolsSession* session) {
  // Destroying session automatically detaches in renderer.
  if (sessions().empty()) {
    frame_trace_recorder_.reset();
    UpdateRawHeadersAccess(frame_host_, nullptr);
#if defined(OS_ANDROID)
    GetWakeLock()->CancelWakeLock();
#endif
  }
}

void RenderFrameDevToolsAgentHost::InspectElement(RenderFrameHost* frame_host,
                                                  int x,
                                                  int y) {
  FrameTreeNode* ftn =
      static_cast<RenderFrameHostImpl*>(frame_host)->frame_tree_node();
  RenderFrameDevToolsAgentHost* host =
      static_cast<RenderFrameDevToolsAgentHost*>(GetOrCreateFor(ftn).get());

  gfx::Point point(x, y);
  // The renderer expects coordinates relative to the local frame root,
  // so we need to transform the coordinates from the root space
  // to the local frame root widget's space.
  if (host->frame_host_) {
    if (RenderWidgetHostView* view = host->frame_host_->GetView()) {
      point = gfx::ToRoundedPoint(
          view->TransformRootPointToViewCoordSpace(gfx::PointF(point)));
    }
  }
  host->GetRendererChannel()->InspectElement(point);
}

RenderFrameDevToolsAgentHost::~RenderFrameDevToolsAgentHost() {
  SetFrameTreeNode(nullptr);
}

void RenderFrameDevToolsAgentHost::ReadyToCommitNavigation(
    NavigationHandle* navigation_handle) {
  NavigationHandleImpl* handle =
      static_cast<NavigationHandleImpl*>(navigation_handle);
  for (auto* tracing : protocol::TracingHandler::ForAgentHost(this))
    tracing->ReadyToCommitNavigation(handle);

  if (handle->frame_tree_node() != frame_tree_node_) {
    if (ShouldForceCreation() && handle->GetRenderFrameHost() &&
        handle->GetRenderFrameHost()->IsCrossProcessSubframe()) {
      RenderFrameDevToolsAgentHost::GetOrCreateForDangling(
          handle->frame_tree_node());
    }
    return;
  }

  UpdateFrameHost(handle->GetRenderFrameHost());
  // UpdateFrameHost may destruct |this|.
}

void RenderFrameDevToolsAgentHost::DidFinishNavigation(
    NavigationHandle* navigation_handle) {
  NavigationHandleImpl* handle =
      static_cast<NavigationHandleImpl*>(navigation_handle);
  if (handle->frame_tree_node() != frame_tree_node_)
    return;
  navigation_handles_.erase(handle);
  NotifyNavigated();

  // UpdateFrameHost may destruct |this|.
  scoped_refptr<RenderFrameDevToolsAgentHost> protect(this);
  UpdateFrameHost(frame_tree_node_->current_frame_host());

  if (navigation_handles_.empty()) {
    for (DevToolsSession* session : sessions())
      session->ResumeSendingMessagesToAgent();
  }
  if (handle->HasCommitted()) {
    for (auto* target : protocol::TargetHandler::ForAgentHost(this))
      target->DidCommitNavigation();
  }
}

void RenderFrameDevToolsAgentHost::UpdateFrameHost(
    RenderFrameHostImpl* frame_host) {
  if (frame_host == frame_host_) {
    if (frame_host && !render_frame_alive_) {
      render_frame_alive_ = true;
      for (auto* inspector : protocol::InspectorHandler::ForAgentHost(this))
        inspector->TargetReloadedAfterCrash();
      UpdateRendererChannel(IsAttached());
    }
    return;
  }

  if (frame_host && !ShouldCreateDevToolsForHost(frame_host)) {
    DestroyOnRenderFrameGone();
    // |this| may be deleted at this point.
    return;
  }

  RenderFrameHostImpl* old_host = frame_host_;
  frame_host_ = frame_host;
  if (IsAttached())
    UpdateRawHeadersAccess(old_host, frame_host);

  std::vector<DevToolsSession*> restricted_sessions;
  for (DevToolsSession* session : sessions()) {
    if (!ShouldAllowSession(session))
      restricted_sessions.push_back(session);
  }
  if (!restricted_sessions.empty())
    ForceDetachRestrictedSessions(restricted_sessions);

  if (!render_frame_alive_) {
    render_frame_alive_ = true;
    for (auto* inspector : protocol::InspectorHandler::ForAgentHost(this))
      inspector->TargetReloadedAfterCrash();
  }

  UpdateRendererChannel(IsAttached());
}

void RenderFrameDevToolsAgentHost::DidStartNavigation(
    NavigationHandle* navigation_handle) {
  NavigationHandleImpl* handle =
      static_cast<NavigationHandleImpl*>(navigation_handle);
  if (handle->frame_tree_node() != frame_tree_node_)
    return;
  if (navigation_handles_.empty()) {
    for (DevToolsSession* session : sessions())
      session->SuspendSendingMessagesToAgent();
  }
  navigation_handles_.insert(handle);
}

void RenderFrameDevToolsAgentHost::RenderFrameHostChanged(
    RenderFrameHost* old_host,
    RenderFrameHost* new_host) {
  if (old_host != frame_host_)
    return;

  UpdateFrameHost(nullptr);
  // UpdateFrameHost may destruct |this|.
}

void RenderFrameDevToolsAgentHost::FrameDeleted(RenderFrameHost* rfh) {
  RenderFrameHostImpl* host = static_cast<RenderFrameHostImpl*>(rfh);
  for (auto* tracing : protocol::TracingHandler::ForAgentHost(this))
    tracing->FrameDeleted(host);
  if (host->frame_tree_node() == frame_tree_node_) {
    DestroyOnRenderFrameGone();
    // |this| may be deleted at this point.
  }
}

void RenderFrameDevToolsAgentHost::RenderFrameDeleted(RenderFrameHost* rfh) {
  if (rfh == frame_host_) {
    render_frame_alive_ = false;
    UpdateRendererChannel(IsAttached());
  }
}

void RenderFrameDevToolsAgentHost::DestroyOnRenderFrameGone() {
  scoped_refptr<RenderFrameDevToolsAgentHost> protect(this);
  if (IsAttached()) {
    ForceDetachAllSessions();
    UpdateRawHeadersAccess(frame_host_, nullptr);
  }
  frame_host_ = nullptr;
  UpdateRendererChannel(IsAttached());
  SetFrameTreeNode(nullptr);
  Release();
}

#if defined(OS_ANDROID)
device::mojom::WakeLock* RenderFrameDevToolsAgentHost::GetWakeLock() {
  // Here is a lazy binding, and will not reconnect after connection error.
  if (!wake_lock_) {
    device::mojom::WakeLockRequest request = mojo::MakeRequest(&wake_lock_);
    device::mojom::WakeLockContext* wake_lock_context =
        web_contents()->GetWakeLockContext();
    if (wake_lock_context) {
      wake_lock_context->GetWakeLock(
          device::mojom::WakeLockType::kPreventDisplaySleep,
          device::mojom::WakeLockReason::kOther, "DevTools",
          std::move(request));
    }
  }
  return wake_lock_.get();
}
#endif

void RenderFrameDevToolsAgentHost::RenderProcessGone(
    base::TerminationStatus status) {
  switch(status) {
    case base::TERMINATION_STATUS_ABNORMAL_TERMINATION:
    case base::TERMINATION_STATUS_PROCESS_WAS_KILLED:
#if defined(OS_CHROMEOS)
    case base::TERMINATION_STATUS_PROCESS_WAS_KILLED_BY_OOM:
#endif
    case base::TERMINATION_STATUS_PROCESS_CRASHED:
#if defined(OS_ANDROID)
    case base::TERMINATION_STATUS_OOM_PROTECTED:
#endif
    case base::TERMINATION_STATUS_LAUNCH_FAILED:
      for (auto* inspector : protocol::InspectorHandler::ForAgentHost(this))
        inspector->TargetCrashed();
      NotifyCrashed(status);
      break;
    default:
      for (auto* inspector : protocol::InspectorHandler::ForAgentHost(this))
        inspector->TargetDetached("Render process gone.");
      break;
  }
}

void RenderFrameDevToolsAgentHost::DidAttachInterstitialPage() {
  for (auto* page : protocol::PageHandler::ForAgentHost(this))
    page->DidAttachInterstitialPage();
}

void RenderFrameDevToolsAgentHost::DidDetachInterstitialPage() {
  for (auto* page : protocol::PageHandler::ForAgentHost(this))
    page->DidDetachInterstitialPage();
}

void RenderFrameDevToolsAgentHost::OnVisibilityChanged(
    content::Visibility visibility) {
#if defined(OS_ANDROID)
  if (visibility == content::Visibility::HIDDEN)
    GetWakeLock()->CancelWakeLock();
  else
    GetWakeLock()->RequestWakeLock();
#endif
}

void RenderFrameDevToolsAgentHost::OnPageScaleFactorChanged(
    float page_scale_factor) {
  for (auto* input : protocol::InputHandler::ForAgentHost(this))
    input->OnPageScaleFactorChanged(page_scale_factor);
}

void RenderFrameDevToolsAgentHost::DisconnectWebContents() {
  navigation_handles_.clear();
  SetFrameTreeNode(nullptr);
  // UpdateFrameHost may destruct |this|.
  scoped_refptr<RenderFrameDevToolsAgentHost> protect(this);
  UpdateFrameHost(nullptr);
  for (DevToolsSession* session : sessions())
    session->ResumeSendingMessagesToAgent();
}

void RenderFrameDevToolsAgentHost::ConnectWebContents(WebContents* wc) {
  RenderFrameHostImpl* host =
      static_cast<RenderFrameHostImpl*>(wc->GetMainFrame());
  DCHECK(host);
  SetFrameTreeNode(host->frame_tree_node());
  UpdateFrameHost(host);
  // UpdateFrameHost may destruct |this|.
}

std::string RenderFrameDevToolsAgentHost::GetParentId() {
  if (IsChildFrame()) {
    FrameTreeNode* frame_tree_node =
        GetFrameTreeNodeAncestor(frame_tree_node_->parent());
    return RenderFrameDevToolsAgentHost::GetOrCreateFor(frame_tree_node)
        ->GetId();
  }

  WebContentsImpl* contents = static_cast<WebContentsImpl*>(web_contents());
  if (!contents)
    return "";
  contents = contents->GetOuterWebContents();
  if (contents)
    return DevToolsAgentHost::GetOrCreateFor(contents)->GetId();
  return "";
}

std::string RenderFrameDevToolsAgentHost::GetOpenerId() {
  if (!frame_tree_node_)
    return std::string();
  FrameTreeNode* opener = frame_tree_node_->original_opener();
  return opener ? opener->devtools_frame_token().ToString() : std::string();
}

std::string RenderFrameDevToolsAgentHost::GetType() {
  if (web_contents() &&
      static_cast<WebContentsImpl*>(web_contents())->GetOuterWebContents()) {
    return kTypeGuest;
  }
  if (IsChildFrame())
    return kTypeFrame;
  DevToolsManager* manager = DevToolsManager::GetInstance();
  if (manager->delegate() && web_contents()) {
    std::string type = manager->delegate()->GetTargetType(web_contents());
    if (!type.empty())
      return type;
  }
  return kTypePage;
}

std::string RenderFrameDevToolsAgentHost::GetTitle() {
  DevToolsManager* manager = DevToolsManager::GetInstance();
  if (manager->delegate() && web_contents()) {
    std::string title = manager->delegate()->GetTargetTitle(web_contents());
    if (!title.empty())
      return title;
  }
  if (IsChildFrame() && frame_host_)
    return frame_host_->GetLastCommittedURL().spec();
  if (web_contents())
    return base::UTF16ToUTF8(web_contents()->GetTitle());
  return GetURL().spec();
}

std::string RenderFrameDevToolsAgentHost::GetDescription() {
  DevToolsManager* manager = DevToolsManager::GetInstance();
  if (manager->delegate() && web_contents())
    return manager->delegate()->GetTargetDescription(web_contents());
  return std::string();
}

GURL RenderFrameDevToolsAgentHost::GetURL() {
  // Order is important here.
  WebContents* web_contents = GetWebContents();
  if (web_contents && !IsChildFrame())
    return web_contents->GetVisibleURL();
  if (frame_host_)
    return frame_host_->GetLastCommittedURL();
  return GURL();
}

GURL RenderFrameDevToolsAgentHost::GetFaviconURL() {
  WebContents* wc = web_contents();
  if (!wc)
    return GURL();
  NavigationEntry* entry = wc->GetController().GetLastCommittedEntry();
  if (entry)
    return entry->GetFavicon().url;
  return GURL();
}

bool RenderFrameDevToolsAgentHost::Activate() {
  WebContentsImpl* wc = static_cast<WebContentsImpl*>(web_contents());
  if (wc) {
    wc->Activate();
    return true;
  }
  return false;
}

void RenderFrameDevToolsAgentHost::Reload() {
  WebContentsImpl* wc = static_cast<WebContentsImpl*>(web_contents());
  if (wc)
    wc->GetController().Reload(ReloadType::NORMAL, true);
}

bool RenderFrameDevToolsAgentHost::Close() {
  if (web_contents()) {
    web_contents()->ClosePage();
    return true;
  }
  return false;
}

base::TimeTicks RenderFrameDevToolsAgentHost::GetLastActivityTime() {
  if (WebContents* contents = web_contents())
    return contents->GetLastActiveTime();
  return base::TimeTicks();
}

void RenderFrameDevToolsAgentHost::SignalSynchronousSwapCompositorFrame(
    RenderFrameHost* frame_host,
    viz::CompositorFrameMetadata frame_metadata) {
  scoped_refptr<RenderFrameDevToolsAgentHost> dtah(FindAgentHost(
      static_cast<RenderFrameHostImpl*>(frame_host)->frame_tree_node()));
  if (dtah) {
    // Unblock the compositor.
    base::PostTaskWithTraits(
        FROM_HERE, {BrowserThread::UI},
        base::BindOnce(
            &RenderFrameDevToolsAgentHost::SynchronousSwapCompositorFrame,
            dtah.get(), std::move(frame_metadata)));
  }
}

void RenderFrameDevToolsAgentHost::SynchronousSwapCompositorFrame(
    viz::CompositorFrameMetadata frame_metadata) {
  for (auto* page : protocol::PageHandler::ForAgentHost(this))
    page->OnSynchronousSwapCompositorFrame(frame_metadata.Clone());

  if (!frame_trace_recorder_)
    return;
  bool did_initiate_recording = false;
  for (auto* tracing : protocol::TracingHandler::ForAgentHost(this))
    did_initiate_recording |= tracing->did_initiate_recording();
  if (did_initiate_recording) {
    frame_trace_recorder_->OnSynchronousSwapCompositorFrame(frame_host_,
                                                            frame_metadata);
  }
}

void RenderFrameDevToolsAgentHost::UpdateRendererChannel(bool force) {
  blink::mojom::DevToolsAgentAssociatedPtr agent_ptr;
  blink::mojom::DevToolsAgentHostAssociatedRequest host_request;
  if (frame_host_ && render_frame_alive_ && force) {
    blink::mojom::DevToolsAgentHostAssociatedPtrInfo host_ptr_info;
    host_request = mojo::MakeRequest(&host_ptr_info);
    frame_host_->BindDevToolsAgent(std::move(host_ptr_info),
                                   mojo::MakeRequest(&agent_ptr));
  }
  int process_id = frame_host_ ? frame_host_->GetProcess()->GetID()
                               : ChildProcessHost::kInvalidUniqueID;
  GetRendererChannel()->SetRendererAssociated(
      std::move(agent_ptr), std::move(host_request), process_id, frame_host_);
}

bool RenderFrameDevToolsAgentHost::IsChildFrame() {
  return frame_tree_node_ && frame_tree_node_->parent();
}

bool RenderFrameDevToolsAgentHost::ShouldAllowSession(
    DevToolsSession* session) {
  DevToolsManager* manager = DevToolsManager::GetInstance();
  if (manager->delegate() && frame_host_) {
    if (!manager->delegate()->AllowInspectingRenderFrameHost(frame_host_))
      return false;
  }
  const bool is_webui =
      frame_host_ && (frame_host_->web_ui() || frame_host_->pending_web_ui());
  if (!session->client()->MayAttachToRenderer(frame_host_, is_webui))
    return false;
  return true;
}

}  // namespace content
