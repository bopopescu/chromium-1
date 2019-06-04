// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/service/service_ipc_server.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/thread.h"
#include "build/build_config.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

void PumpCurrentLoop() {
  base::RunLoop(base::RunLoop::Type::kNestableTasksAllowed).RunUntilIdle();
}

class FakeServiceIPCServerClient : public ServiceIPCServer::Client {
 public:
  FakeServiceIPCServerClient() {}
  ~FakeServiceIPCServerClient() override {}
  void OnShutdown() override;
  void OnUpdateAvailable() override;
  bool OnIPCClientDisconnect() override;
  mojo::ScopedMessagePipeHandle CreateChannelMessagePipe() override;

  int shutdown_calls_ = 0;
  int update_available_calls_ = 0;
  int ipc_client_disconnect_calls_ = 0;
  service_manager::mojom::InterfaceProviderPtr interface_provider_;
};

void FakeServiceIPCServerClient::OnShutdown() {
  shutdown_calls_++;
}

void FakeServiceIPCServerClient::OnUpdateAvailable() {
  update_available_calls_++;
}

bool FakeServiceIPCServerClient::OnIPCClientDisconnect() {
  ipc_client_disconnect_calls_++;

  // Always return true to indicate the server must continue listening for new
  // connections.
  return true;
}

mojo::ScopedMessagePipeHandle
FakeServiceIPCServerClient::CreateChannelMessagePipe() {
  return mojo::MakeRequest(&interface_provider_).PassMessagePipe();
}

}  // namespace

class ServiceIPCServerTest : public ::testing::Test {
 public:
  ServiceIPCServerTest();
  ~ServiceIPCServerTest() override {}
  void SetUp() override;
  void TearDown() override;
  void PumpLoops();

  // Simulates the browser process connecting to the service process.
  void ConnectClientChannel();

  // Simulates the browser process shutting down.
  void DestroyClientChannel();

 protected:
  FakeServiceIPCServerClient service_process_client_;
  base::MessageLoopForUI main_message_loop_;
  base::Thread io_thread_;
  base::WaitableEvent shutdown_event_;
  std::unique_ptr<ServiceIPCServer> server_;
  service_manager::InterfaceProvider remote_interfaces_;
  chrome::mojom::ServiceProcessPtr service_process_;
};

ServiceIPCServerTest::ServiceIPCServerTest()
    : io_thread_("ServiceIPCServerTest IO"),
      shutdown_event_(base::WaitableEvent::ResetPolicy::MANUAL,
                      base::WaitableEvent::InitialState::NOT_SIGNALED) {}

void ServiceIPCServerTest::SetUp() {
  base::Thread::Options options;
  mojo::MessagePipe channel;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  ASSERT_TRUE(io_thread_.StartWithOptions(options));

  server_.reset(new ServiceIPCServer(&service_process_client_,
                                     io_thread_.task_runner(),
                                     &shutdown_event_));
  server_->Init();
}

void ServiceIPCServerTest::TearDown() {
  // Close the ipc channels to prevent memory leaks.
  if (service_process_) {
    remote_interfaces_.Close();
    service_process_.reset();
    PumpLoops();
  }
  io_thread_.Stop();
}

void ServiceIPCServerTest::PumpLoops() {
  base::RunLoop run_loop;
  io_thread_.task_runner()->PostTaskAndReply(FROM_HERE,
                                             base::Bind(&PumpCurrentLoop),
                                             run_loop.QuitClosure());
  run_loop.Run();
  PumpCurrentLoop();
}

void ServiceIPCServerTest::ConnectClientChannel() {
  remote_interfaces_.Close();
  remote_interfaces_.Bind(
      std::move(service_process_client_.interface_provider_));

  remote_interfaces_.GetInterface(&service_process_);
  service_process_->Hello(base::DoNothing());
  PumpLoops();
}

void ServiceIPCServerTest::DestroyClientChannel() {
  remote_interfaces_.Close();
  service_process_.reset();
  PumpLoops();
}

TEST_F(ServiceIPCServerTest, ConnectDisconnectReconnect) {
  // Initially there is no ipc client connected.
  ASSERT_FALSE(server_->is_ipc_client_connected());

  // When a channel is connected the server is notified via OnChannelConnected.
  ConnectClientChannel();
  ASSERT_TRUE(server_->is_ipc_client_connected());

  // When the channel is destroyed the server is notified via OnChannelError.
  // In turn, the server notifies its service process client.
  DestroyClientChannel();
  ASSERT_FALSE(server_->is_ipc_client_connected());
  ASSERT_EQ(1, service_process_client_.ipc_client_disconnect_calls_);

  ConnectClientChannel();
  ASSERT_TRUE(server_->is_ipc_client_connected());
  service_process_->UpdateAvailable();
  PumpLoops();
  ASSERT_TRUE(server_->is_ipc_client_connected());

  // Destroy the client process channel again to verify the
  // ServiceIPCServer::Client is notified again. This means that unlike
  // OnChannelConnected, OnChannelError is called more than once.
  DestroyClientChannel();
  ASSERT_FALSE(server_->is_ipc_client_connected());
  ASSERT_EQ(2, service_process_client_.ipc_client_disconnect_calls_);
}

TEST_F(ServiceIPCServerTest, Shutdown) {
  ConnectClientChannel();
  ASSERT_TRUE(server_->is_ipc_client_connected());

  // When a shutdown message is received, the ServiceIPCServer::Client is
  // notified.
  service_process_->ShutDown();
  PumpLoops();
  ASSERT_EQ(1, service_process_client_.shutdown_calls_);
}

TEST_F(ServiceIPCServerTest, UpdateAvailable) {
  ConnectClientChannel();
  ASSERT_TRUE(server_->is_ipc_client_connected());

  // When a product update message is received, the ServiceIPCServer::Client is
  // notified.
  service_process_->UpdateAvailable();
  PumpLoops();
  ASSERT_EQ(1, service_process_client_.update_available_calls_);
}
