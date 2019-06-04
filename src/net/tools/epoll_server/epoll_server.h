// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_EPOLL_SERVER_EPOLL_SERVER_H_
#define NET_TOOLS_EPOLL_SERVER_EPOLL_SERVER_H_

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/queue.h>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// #define EPOLL_SERVER_EVENT_TRACING 1
//
// Defining EPOLL_SERVER_EVENT_TRACING
// causes code to exist which didn't before.
// This code tracks each event generated by the epollserver,
// as well as providing a per-fd-registered summary of
// events. Note that enabling this code vastly slows
// down operations, and uses substantially more
// memory. For these reasons, it should only be enabled by developers doing
// development at their workstations.
//
// A structure called 'EventRecorder' will exist when
// the macro is defined. See the EventRecorder class interface
// within the EpollServer class for more details.
#ifdef EPOLL_SERVER_EVENT_TRACING
#include <ostream>
#include "base/logging.h"
#endif

#include "base/compiler_specific.h"
#include "base/macros.h"
#include <sys/epoll.h>

namespace net {

class EpollServer;
class EpollAlarmCallbackInterface;
class ReadPipeCallback;

struct EpollEvent {
  EpollEvent(int events)
      : in_events(events),
        out_ready_mask(0) {
  }

  int in_events;            // incoming events
  int out_ready_mask;       // the new event mask for ready list (0 means don't
                            // get on the ready list). This field is always
                            // initialized to 0 when the event is passed to
                            // OnEvent.
};

// Callbacks which go into EpollServers are expected to derive from this class.
class EpollCallbackInterface {
 public:
  // Summary:
  //   Called when the callback is registered into a EpollServer.
  // Args:
  //   eps - the poll server into which this callback was registered
  //   fd - the file descriptor which was registered
  //   event_mask - the event mask (composed of EPOLLIN, EPOLLOUT, etc)
  //                which was registered (and will initially be used
  //                in the epoll() calls)
  virtual void OnRegistration(EpollServer* eps, int fd, int event_mask) = 0;

  // Summary:
  //   Called when the event_mask is modified (for a file-descriptor)
  // Args:
  //   fd - the file descriptor which was registered
  //   event_mask - the event mask (composed of EPOLLIN, EPOLLOUT, etc)
  //                which was is now curren (and will be used
  //                in subsequent epoll() calls)
  virtual void OnModification(int fd, int event_mask) = 0;

  // Summary:
  //   Called whenever an event occurs on the file-descriptor.
  //   This is where the bulk of processing is expected to occur.
  // Args:
  //   fd - the file descriptor which was registered
  //   event - a struct that contains the event mask (composed of EPOLLIN,
  //           EPOLLOUT, etc), a flag that indicates whether this is a true
  //           epoll_wait event vs one from the ready list, and an output
  //           parameter for OnEvent to inform the EpollServer whether to put
  //           this fd on the ready list.
  virtual void OnEvent(int fd, EpollEvent* event) = 0;

  // Summary:
  //   Called when the file-descriptor is unregistered from the poll-server.
  // Args:
  //   fd - the file descriptor which was registered, and of this call, is now
  //        unregistered.
  //   replaced - If true, this callback is being replaced by another, otherwise
  //              it is simply being removed.
  virtual void OnUnregistration(int fd, bool replaced) = 0;

  // Summary:
  //   Called when the epoll server is shutting down.  This is different from
  //   OnUnregistration because the subclass may want to clean up memory.
  //   This is called in leiu of OnUnregistration.
  // Args:
  //  fd - the file descriptor which was registered.
  virtual void OnShutdown(EpollServer* eps, int fd) = 0;

  // Summary:
  //   Returns a name describing the class for use in debug/error reporting.
  virtual std::string Name() const = 0;

  virtual ~EpollCallbackInterface() {}

 protected:
  EpollCallbackInterface() {}
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

class EpollServer {
 public:
  typedef EpollAlarmCallbackInterface AlarmCB;
  typedef EpollCallbackInterface CB;

  typedef std::multimap<int64_t, AlarmCB*> TimeToAlarmCBMap;
  typedef TimeToAlarmCBMap::iterator AlarmRegToken;

  // Summary:
  //   Constructor:
  //    By default, we don't wait any amount of time for events, and
  //    we suggest to the epoll-system that we're going to use on-the-order
  //    of 1024 FDs.
  EpollServer();

  ////////////////////////////////////////

  // Destructor
  virtual ~EpollServer();

  ////////////////////////////////////////

  // Summary
  //   Register a callback to be called whenever an event contained
  //   in the set of events included in event_mask occurs on the
  //   file-descriptor 'fd'
  //
  //   Note that only one callback is allowed to be registered for
  //   any specific file-decriptor.
  //
  //   If a callback is registered for a file-descriptor which has already
  //   been registered, then the previous callback is unregistered with
  //   the 'replaced' flag set to true. I.e. the previous callback's
  //   OnUnregistration() function is called like so:
  //      OnUnregistration(fd, true);
  //
  //  The epoll server does NOT take on ownership of the callback: the callback
  //  creator is responsible for managing that memory.
  //
  // Args:
  //   fd - a valid file-descriptor
  //   cb - an instance of a subclass of EpollCallbackInterface
  //   event_mask - a combination of (EPOLLOUT, EPOLLIN.. etc) indicating
  //                the events for which the callback would like to be
  //                called.
  virtual void RegisterFD(int fd, CB* cb, int event_mask);

  ////////////////////////////////////////

  // Summary:
  //   A shortcut for RegisterFD which sets things up such that the
  //   callback is called when 'fd' is available for writing.
  // Args:
  //   fd - a valid file-descriptor
  //   cb - an instance of a subclass of EpollCallbackInterface
  virtual void RegisterFDForWrite(int fd, CB* cb);

  ////////////////////////////////////////

  // Summary:
  //   A shortcut for RegisterFD which sets things up such that the
  //   callback is called when 'fd' is available for reading or writing.
  // Args:
  //   fd - a valid file-descriptor
  //   cb - an instance of a subclass of EpollCallbackInterface
  virtual void RegisterFDForReadWrite(int fd, CB* cb);

  ////////////////////////////////////////

  // Summary:
  //   A shortcut for RegisterFD which sets things up such that the
  //   callback is called when 'fd' is available for reading.
  // Args:
  //   fd - a valid file-descriptor
  //   cb - an instance of a subclass of EpollCallbackInterface
  virtual void RegisterFDForRead(int fd, CB* cb);

  ////////////////////////////////////////

  // Summary:
  //   Removes the FD and the associated callback from the pollserver.
  //   If the callback is registered with other FDs, they will continue
  //   to be processed using the callback without modification.
  //   If the file-descriptor specified is not registered in the
  //   epoll_server, then nothing happens as a result of this call.
  // Args:
  //   fd - the file-descriptor which should no-longer be monitored.
  virtual void UnregisterFD(int fd);

  ////////////////////////////////////////

  // Summary:
  //   Modifies the event mask for the file-descriptor, replacing
  //   the old event_mask with the new one specified here.
  //   If the file-descriptor specified is not registered in the
  //   epoll_server, then nothing happens as a result of this call.
  // Args:
  //   fd - the fd whose event mask should be modified.
  //   event_mask - the new event mask.
  virtual void ModifyCallback(int fd, int event_mask);

  ////////////////////////////////////////

  // Summary:
  //   Modifies the event mask for the file-descriptor such that we
  //   no longer request events when 'fd' is readable.
  //   If the file-descriptor specified is not registered in the
  //   epoll_server, then nothing happens as a result of this call.
  // Args:
  //   fd - the fd whose event mask should be modified.
  virtual void StopRead(int fd);

  ////////////////////////////////////////

  // Summary:
  //   Modifies the event mask for the file-descriptor such that we
  //   request events when 'fd' is readable.
  //   If the file-descriptor specified is not registered in the
  //   epoll_server, then nothing happens as a result of this call.
  // Args:
  //   fd - the fd whose event mask should be modified.
  virtual void StartRead(int fd);

  ////////////////////////////////////////

  // Summary:
  //   Modifies the event mask for the file-descriptor such that we
  //   no longer request events when 'fd' is writable.
  //   If the file-descriptor specified is not registered in the
  //   epoll_server, then nothing happens as a result of this call.
  // Args:
  //   fd - the fd whose event mask should be modified.
  virtual void StopWrite(int fd);

  ////////////////////////////////////////

  // Summary:
  //   Modifies the event mask for the file-descriptor such that we
  //   request events when 'fd' is writable.
  //   If the file-descriptor specified is not registered in the
  //   epoll_server, then nothing happens as a result of this call.
  // Args:
  //   fd - the fd whose event mask should be modified.
  virtual void StartWrite(int fd);

  ////////////////////////////////////////

  // Summary:
  //   Looks up the callback associated with the file-desriptor 'fd'.
  //   If a callback is associated with this file-descriptor, then
  //   it's OnEvent() method is called with the file-descriptor 'fd',
  //   and event_mask 'event_mask'
  //
  //   If no callback is registered for this file-descriptor, nothing
  //   will happen as a result of this call.
  //
  //   This function is used internally by the EpollServer, but is
  //   available publically so that events might be 'faked'. Calling
  //   this function with an fd and event_mask is equivalent (as far
  //   as the callback is concerned) to having a real event generated
  //   by epoll (except, of course, that read(), etc won't necessarily
  //   be able to read anything)
  // Args:
  //   fd - the file-descriptor on which an event has occured.
  //   event_mask - a bitmask representing the events which have occured
  //                on/for this fd. This bitmask is composed of
  //                POLLIN, POLLOUT, etc.
  //
  void HandleEvent(int fd, int event_mask);

  // Summary:
  //   Call this when you want the pollserver to
  //   wait for events and execute the callbacks associated with
  //   the file-descriptors on which those events have occured.
  //   Depending on the value of timeout_in_us_, this may or may
  //   not return immediately. Please reference the set_timeout()
  //   function for the specific behaviour.
  virtual void WaitForEventsAndExecuteCallbacks();

  // Summary:
  //   When an fd is registered to use edge trigger notification, the ready
  //   list can be used to simulate level trigger semantics. Edge trigger
  //   registration doesn't send an initial event, and only rising edge (going
  //   from blocked to unblocked) events are sent. A callback can put itself on
  //   the ready list by calling SetFDReady() after calling RegisterFD(). The
  //   OnEvent method of all callbacks associated with the fds on the ready
  //   list will be called immediately after processing the events returned by
  //   epoll_wait(). The fd is removed from the ready list before the
  //   callback's OnEvent() method is invoked. To stay on the ready list, the
  //   OnEvent() (or some function in that call chain) must call SetFDReady
  //   again. When a fd is unregistered using UnregisterFD(), the fd is
  //   automatically removed from the ready list.
  //
  //   When the callback for a edge triggered fd hits the falling edge (about
  //   to block, either because of it got an EAGAIN, or had a short read/write
  //   operation), it should remove itself from the ready list using
  //   SetFDNotReady() (since OnEvent cannot distinguish between invocation
  //   from the ready list vs from a normal epoll event). All four ready list
  //   methods are safe to be called  within the context of the callbacks.
  //
  //   Since the ready list invokes EpollCallbackInterface::OnEvent, only fds
  //   that are registered with the EpollServer will be put on the ready list.
  //   SetFDReady() and SetFDNotReady() will do nothing if the EpollServer
  //   doesn't know about the fd passed in.
  //
  //   Since the ready list cannot reliably determine proper set of events
  //   which should be sent to the callback, SetFDReady() requests the caller
  //   to provide the ready list with the event mask, which will be used later
  //   when OnEvent() is invoked by the ready list. Hence, the event_mask
  //   passedto SetFDReady() does not affect the actual epoll registration of
  //   the fd with the kernel. If a fd is already put on the ready list, and
  //   SetFDReady() is called again for that fd with a different event_mask,
  //   the event_mask will be updated.
  virtual void SetFDReady(int fd, int events_to_fake);

  virtual void SetFDNotReady(int fd);

  // Summary:
  //   IsFDReady(), ReadyListSize(), and VerifyReadyList are intended as
  //   debugging tools and for writing unit tests.
  //   ISFDReady() returns whether a fd is in the ready list.
  //   ReadyListSize() returns the number of fds on the ready list.
  //   VerifyReadyList() checks the consistency of internal data structure. It
  //   will CHECK if it finds an error.
  virtual bool IsFDReady(int fd) const;

  size_t ReadyListSize() const { return ready_list_size_; }

  void VerifyReadyList() const;

  ////////////////////////////////////////

  // Summary:
  //   Registers an alarm 'ac' to go off at time 'timeout_time_in_us'.
  //   If the callback returns a positive number from its OnAlarm() function,
  //   then the callback will be re-registered at that time, else the alarm
  //   owner is responsible for freeing up memory.
  //
  //   Important: A give AlarmCB* can not be registered again if it is already
  //    registered. If a user wants to register a callback again it should first
  //    unregister the previous callback before calling RegisterAlarm again.
  // Args:
  //   timeout_time_in_us - the absolute time at which the alarm should go off
  //   ac - the alarm which will be called.
  virtual void RegisterAlarm(int64_t timeout_time_in_us, AlarmCB* ac);

  // Summary:
  //   Registers an alarm 'ac' to go off at time: (ApproximateNowInUs() +
  //   delta_in_us). While this is somewhat less accurate (see the description
  //   for ApproximateNowInUs() to see how 'approximate'), the error is never
  //   worse than the amount of time it takes to process all events in one
  //   WaitForEvents.  As with 'RegisterAlarm()', if the callback returns a
  //   positive number from its OnAlarm() function, then the callback will be
  //   re-registered at that time, else the alarm owner is responsible for
  //   freeing up memory.
  //   Note that this function is purely a convienence. The
  //   same thing may be accomplished by using RegisterAlarm with
  //   ApproximateNowInUs() directly.
  //
  //   Important: A give AlarmCB* can not be registered again if it is already
  //    registered. If a user wants to register a callback again it should first
  //    unregister the previous callback before calling RegisterAlarm again.
  // Args:
  //   delta_in_us - the delta in microseconds from the ApproximateTimeInUs() at
  //                 which point the alarm should go off.
  //   ac - the alarm which will be called.
  void RegisterAlarmApproximateDelta(int64_t delta_in_us, AlarmCB* ac) {
    RegisterAlarm(ApproximateNowInUsec() + delta_in_us, ac);
  }

  ////////////////////////////////////////

  // Summary:
  //   Unregister  the alarm referred to by iterator_token; Callers should
  //   be warned that a token may have become already invalid when OnAlarm()
  //   is called, was unregistered, or OnShutdown was called on that alarm.
  // Args:
  //    iterator_token - iterator to the alarm callback to unregister.
  virtual void UnregisterAlarm(
      const EpollServer::AlarmRegToken& iterator_token);

  ////////////////////////////////////////

  // Summary:
  //   returns the number of file-descriptors registered in this EpollServer.
  // Returns:
  //   number of FDs registered (discounting the internal pipe used for Wake)
  virtual int NumFDsRegistered() const;

  // Summary:
  //   Force the epoll server to wake up (by writing to an internal pipe).
  virtual void Wake();

  // Summary:
  //   Wrapper around WallTimer's NowInUsec.  We do this so that we can test
  //   EpollServer without using the system clock (and can avoid the flakiness
  //   that would ensue)
  // Returns:
  //   the current time as number of microseconds since the Unix epoch.
  virtual int64_t NowInUsec() const;

  // Summary:
  //   Since calling NowInUsec() many thousands of times per
  //   WaitForEventsAndExecuteCallbacks function call is, to say the least,
  //   inefficient, we allow users to use an approximate time instead. The
  //   time returned from this function is as accurate as NowInUsec() when
  //   WaitForEventsAndExecuteCallbacks is not an ancestor of the caller's
  //   callstack.
  //   However, when WaitForEventsAndExecuteCallbacks -is- an ancestor, then
  //   this function returns the time at which the
  //   WaitForEventsAndExecuteCallbacks function started to process events or
  //   alarms.
  //
  //   Essentially, this function makes available a fast and mostly accurate
  //   mechanism for getting the time for any function handling an event or
  //   alarm. When functions which are not handling callbacks or alarms call
  //   this function, they get the slow and "absolutely" accurate time.
  //
  //   Users should be encouraged to use this function.
  // Returns:
  //   the "approximate" current time as number of microseconds since the Unix
  //   epoch.
  virtual int64_t ApproximateNowInUsec() const;

  static std::string EventMaskToString(int event_mask);

  // Summary:
  //   Logs the state of the epoll server with LOG(ERROR).
  void LogStateOnCrash();

  // Summary:
  //   Set the timeout to the value specified.
  //   If the timeout is set to a negative number,
  //      WaitForEventsAndExecuteCallbacks() will only return when an event has
  //      occured
  //   If the timeout is set to zero,
  //      WaitForEventsAndExecuteCallbacks() will return immediately
  //   If the timeout is set to a positive number,
  //      WaitForEventsAndExecuteCallbacks() will return when an event has
  //      occured, or when timeout_in_us microseconds has elapsed, whichever
  //      is first.
  //  Args:
  //    timeout_in_us - value specified depending on behaviour desired.
  //                    See above.
  void set_timeout_in_us(int64_t timeout_in_us) {
    timeout_in_us_ = timeout_in_us;
  }

  ////////////////////////////////////////

  // Summary:
  //   Accessor for the current value of timeout_in_us.
  int timeout_in_us() const { return timeout_in_us_; }

  // Summary:
  // Returns true when the EpollServer() is being destroyed.
  bool in_shutdown() const { return in_shutdown_; }

 protected:
  virtual void SetNonblocking(int fd);

  // This exists here so that we can override this function in unittests
  // in order to make effective mock EpollServer objects.
  virtual int epoll_wait_impl(int epfd,
                              struct epoll_event* events,
                              int max_events,
                              int timeout_in_ms);

  // this struct is used internally, and is never used by anything external
  // to this class. Some of its members are declared mutable to get around the
  // restriction imposed by hash_set. Since hash_set knows nothing about the
  // objects it stores, it has to assume that every bit of the object is used
  // in the hash function and equal_to comparison. Thus hash_set::iterator is a
  // const iterator. In this case, the only thing that must stay constant is
  // fd. Everything else are just along for the ride and changing them doesn't
  // compromise the hash_set integrity.
  struct CBAndEventMask {
    CBAndEventMask()
        : cb(NULL),
          fd(-1),
          event_mask(0),
          events_asserted(0),
          events_to_fake(0),
          in_use(false) {
      entry.le_next = NULL;
      entry.le_prev = NULL;
    }

    CBAndEventMask(EpollCallbackInterface* cb,
                   int event_mask,
                   int fd)
        : cb(cb), fd(fd), event_mask(event_mask), events_asserted(0),
          events_to_fake(0), in_use(false) {
      entry.le_next = NULL;
      entry.le_prev = NULL;
    }

    // Required operator for hash_set. Normally operator== should be a free
    // standing function. However, since CBAndEventMask is a protected type and
    // it will never be a base class, it makes no difference.
    bool operator==(const CBAndEventMask& cb_and_mask) const {
      return fd == cb_and_mask.fd;
    }
    // A callback. If the fd is unregistered inside the callchain of OnEvent,
    // the cb will be set to NULL.
    mutable EpollCallbackInterface* cb;

    mutable LIST_ENTRY(CBAndEventMask) entry;
    // file descriptor registered with the epoll server.
    int fd;
    // the current event_mask registered for this callback.
    mutable int event_mask;
    // the event_mask that was returned by epoll
    mutable int events_asserted;
    // the event_mask for the ready list to use to call OnEvent.
    mutable int events_to_fake;
    // toggle around calls to OnEvent to tell UnregisterFD to not erase the
    // iterator because HandleEvent is using it.
    mutable bool in_use;
  };

  // Custom hash function to be used by hash_set.
  struct CBAndEventMaskHash {
    size_t operator()(const CBAndEventMask& cb_and_eventmask) const {
      return static_cast<size_t>(cb_and_eventmask.fd);
    }
  };

  using FDToCBMap = std::unordered_set<CBAndEventMask, CBAndEventMaskHash>;

  // the following four functions are OS-specific, and are likely
  // to be changed in a subclass if the poll/select method is changed
  // from epoll.

  // Summary:
  //   Deletes a file-descriptor from the set of FDs that should be
  //   monitored with epoll.
  //   Note that this only deals with modifying data relating -directly-
  //   with the epoll call-- it does not modify any data within the
  //   epoll_server.
  // Args:
  //   fd - the file descriptor to-be-removed from the monitoring set
  virtual void DelFD(int fd) const;

  ////////////////////////////////////////

  // Summary:
  //   Adds a file-descriptor to the set of FDs that should be
  //   monitored with epoll.
  //   Note that this only deals with modifying data relating -directly-
  //   with the epoll call.
  // Args:
  //   fd - the file descriptor to-be-added to the monitoring set
  //   event_mask - the event mask (consisting of EPOLLIN, EPOLLOUT, etc
  //                 OR'd together) which will be associated with this
  //                 FD initially.
  virtual void AddFD(int fd, int event_mask) const;

  ////////////////////////////////////////

  // Summary:
  //   Modifies a file-descriptor in the set of FDs that should be
  //   monitored with epoll.
  //   Note that this only deals with modifying data relating -directly-
  //   with the epoll call.
  // Args:
  //   fd - the file descriptor to-be-added to the monitoring set
  //   event_mask - the event mask (consisting of EPOLLIN, EPOLLOUT, etc
  //                 OR'd together) which will be associated with this
  //                 FD after this call.
  virtual void ModFD(int fd, int event_mask) const;

  ////////////////////////////////////////

  // Summary:
  //   Modified the event mask associated with an FD in the set of
  //   data needed by epoll.
  //   Events are removed before they are added, thus, if ~0 is put
  //   in 'remove_event', whatever is put in 'add_event' will be
  //   the new event mask.
  //   If the file-descriptor specified is not registered in the
  //   epoll_server, then nothing happens as a result of this call.
  // Args:
  //   fd - the file descriptor whose event mask is to be modified
  //   remove_event - the events which are to be removed from the current
  //                  event_mask
  //   add_event - the events which are to be added to the current event_mask
  //
  //
  virtual void ModifyFD(int fd, int remove_event, int add_event);

  ////////////////////////////////////////

  // Summary:
  //   Waits for events, and calls HandleEvents() for each
  //   fd, event pair discovered to possibly have an event.
  //   Note that a callback (B) may get a spurious event if
  //   another callback (A) has closed a file-descriptor N, and
  //   the callback (B) has a newly opened file-descriptor, which
  //   also happens to be N.
  virtual void WaitForEventsAndCallHandleEvents(int64_t timeout_in_us,
                                                struct epoll_event events[],
                                                int events_size);

  // Summary:
  //   A function for implementing the ready list. It invokes OnEvent for each
  //   of the fd in the ready list, and takes care of adding them back to the
  //   ready list if the callback requests it (by checking that out_ready_mask
  //   is non-zero).
  void CallReadyListCallbacks();

  // Summary:
  //   An internal function for implementing the ready list. It adds a fd's
  //   CBAndEventMask to the ready list. If the fd is already on the ready
  //   list, it is a no-op.
  void AddToReadyList(CBAndEventMask* cb_and_mask);

  // Summary:
  //   An internal function for implementing the ready list. It remove a fd's
  //   CBAndEventMask from the ready list. If the fd is not on the ready list,
  //   it is a no-op.
  void RemoveFromReadyList(const CBAndEventMask& cb_and_mask);

  // Summary:
  // Calls any pending alarms that should go off and reregisters them if they
  // were recurring.
  virtual void CallAndReregisterAlarmEvents();

  // The file-descriptor created for epolling
  int epoll_fd_;

  // The mapping of file-descriptor to CBAndEventMasks
  FDToCBMap cb_map_;

  // Custom hash function to be used by hash_set.
  struct AlarmCBHash {
    size_t operator()(AlarmCB*const& p) const {
      return reinterpret_cast<size_t>(p);
    }
  };


  // TODO(sushantj): Having this hash_set is avoidable. We currently have it
  // only so that we can enforce stringent checks that a caller can not register
  // the same alarm twice. One option is to have an implementation in which
  // this hash_set is used only in the debug mode.
  using AlarmCBMap = std::unordered_set<AlarmCB*, AlarmCBHash>;
  AlarmCBMap all_alarms_;

  TimeToAlarmCBMap alarm_map_;

  // The amount of time in microseconds that we'll wait before returning
  // from the WaitForEventsAndExecuteCallbacks() function.
  // If this is positive, wait that many microseconds.
  // If this is negative, wait forever, or for the first event that occurs
  // If this is zero, never wait for an event.
  int64_t timeout_in_us_;

  // This is nonzero only after the invocation of epoll_wait_impl within
  // WaitForEventsAndCallHandleEvents and before the function
  // WaitForEventsAndExecuteCallbacks returns.  At all other times, this is
  // zero. This enables us to have relatively accurate time returned from the
  // ApproximateNowInUs() function. See that function for more details.
  int64_t recorded_now_in_us_;

  // This is used to implement CallAndReregisterAlarmEvents. This stores
  // all alarms that were reregistered because OnAlarm() returned a
  // value > 0 and the time at which they should be executed is less that
  // the current time.  By storing such alarms in this map we ensure
  // that while calling CallAndReregisterAlarmEvents we do not call
  // OnAlarm on any alarm in this set. This ensures that we do not
  // go in an infinite loop.
  AlarmCBMap alarms_reregistered_and_should_be_skipped_;

  LIST_HEAD(ReadyList, CBAndEventMask) ready_list_;
  LIST_HEAD(TmpList, CBAndEventMask) tmp_list_;
  int ready_list_size_;
  // TODO(alyssar): make this into something that scales up.
  static const int events_size_ = 256;
  struct epoll_event events_[256];

#ifdef EPOLL_SERVER_EVENT_TRACING
  struct EventRecorder {
   public:
    EventRecorder() : num_records_(0), record_threshold_(10000) {}

    ~EventRecorder() {
      Clear();
    }

    // When a number of events equals the record threshold,
    // the collected data summary for all FDs will be written
    // to LOG(INFO). Note that this does not include the
    // individual events (if you'reinterested in those, you'll
    // have to get at them programmatically).
    // After any such flushing to LOG(INFO) all events will
    // be cleared.
    // Note that the definition of an 'event' is a bit 'hazy',
    // as it includes the 'Unregistration' event, and perhaps
    // others.
    void set_record_threshold(int64_t new_threshold) {
      record_threshold_ = new_threshold;
    }

    void Clear() {
      for (int i = 0; i < debug_events_.size(); ++i) {
        delete debug_events_[i];
      }
      debug_events_.clear();
      unregistered_fds_.clear();
      event_counts_.clear();
    }

    void MaybeRecordAndClear() {
      ++num_records_;
      if ((num_records_ > record_threshold_) &&
          (record_threshold_ > 0)) {
        LOG(INFO) << "\n" << *this;
        num_records_ = 0;
        Clear();
      }
    }

    void RecordFDMaskEvent(int fd, int mask, const char* function) {
      FDMaskOutput* fdmo = new FDMaskOutput(fd, mask, function);
      debug_events_.push_back(fdmo);
      MaybeRecordAndClear();
    }

    void RecordEpollWaitEvent(int timeout_in_ms,
                              int num_events_generated) {
      EpollWaitOutput* ewo = new EpollWaitOutput(timeout_in_ms,
                                                  num_events_generated);
      debug_events_.push_back(ewo);
      MaybeRecordAndClear();
    }

    void RecordEpollEvent(int fd, int event_mask) {
      Events& events_for_fd = event_counts_[fd];
      events_for_fd.AssignFromMask(event_mask);
      MaybeRecordAndClear();
    }

    friend ostream& operator<<(ostream& os, const EventRecorder& er) {
      for (int i = 0; i < er.unregistered_fds_.size(); ++i) {
        os << "fd: " << er.unregistered_fds_[i] << "\n";
        os << er.unregistered_fds_[i];
      }
      for (EventCountsMap::const_iterator i = er.event_counts_.begin();
           i != er.event_counts_.end();
           ++i) {
        os << "fd: " << i->first << "\n";
        os << i->second;
      }
      for (int i = 0; i < er.debug_events_.size(); ++i) {
        os << *(er.debug_events_[i]) << "\n";
      }
      return os;
    }

    void RecordUnregistration(int fd) {
      EventCountsMap::iterator i = event_counts_.find(fd);
      if (i != event_counts_.end()) {
        unregistered_fds_.push_back(i->second);
        event_counts_.erase(i);
      }
      MaybeRecordAndClear();
    }

   protected:
    class DebugOutput {
     public:
      friend ostream& operator<<(ostream& os, const DebugOutput& debug_output) {
        debug_output.OutputToStream(os);
        return os;
      }
      virtual void OutputToStream(ostream* os) const = 0;
      virtual ~DebugOutput() {}
    };

    class FDMaskOutput : public DebugOutput {
     public:
      FDMaskOutput(int fd, int mask, const char* function) :
          fd_(fd), mask_(mask), function_(function) {}
      virtual void OutputToStream(ostream* os) const {
        (*os) << "func: " << function_
              << "\tfd: " << fd_;
        if (mask_ != 0) {
           (*os) << "\tmask: " << EventMaskToString(mask_);
        }
      }
      int fd_;
      int mask_;
      const char* function_;
    };

    class EpollWaitOutput : public DebugOutput {
     public:
      EpollWaitOutput(int timeout_in_ms,
                      int num_events_generated) :
          timeout_in_ms_(timeout_in_ms),
          num_events_generated_(num_events_generated) {}
      virtual void OutputToStream(ostream* os) const {
        (*os) << "timeout_in_ms: " << timeout_in_ms_
              << "\tnum_events_generated: " << num_events_generated_;
      }
     protected:
      int timeout_in_ms_;
      int num_events_generated_;
    };

    struct Events {
      Events() :
          epoll_in(0),
          epoll_pri(0),
          epoll_out(0),
          epoll_rdnorm(0),
          epoll_rdband(0),
          epoll_wrnorm(0),
          epoll_wrband(0),
          epoll_msg(0),
          epoll_err(0),
          epoll_hup(0),
          epoll_oneshot(0),
          epoll_et(0) {}

      void AssignFromMask(int event_mask) {
        if (event_mask & EPOLLIN) ++epoll_in;
        if (event_mask & EPOLLPRI) ++epoll_pri;
        if (event_mask & EPOLLOUT) ++epoll_out;
        if (event_mask & EPOLLRDNORM) ++epoll_rdnorm;
        if (event_mask & EPOLLRDBAND) ++epoll_rdband;
        if (event_mask & EPOLLWRNORM) ++epoll_wrnorm;
        if (event_mask & EPOLLWRBAND) ++epoll_wrband;
        if (event_mask & EPOLLMSG) ++epoll_msg;
        if (event_mask & EPOLLERR) ++epoll_err;
        if (event_mask & EPOLLHUP) ++epoll_hup;
        if (event_mask & EPOLLONESHOT) ++epoll_oneshot;
        if (event_mask & EPOLLET) ++epoll_et;
      };

      friend ostream& operator<<(ostream& os, const Events& ev) {
        if (ev.epoll_in) {
          os << "\t      EPOLLIN: " << ev.epoll_in << "\n";
        }
        if (ev.epoll_pri) {
          os << "\t     EPOLLPRI: " << ev.epoll_pri << "\n";
        }
        if (ev.epoll_out) {
          os << "\t     EPOLLOUT: " << ev.epoll_out << "\n";
        }
        if (ev.epoll_rdnorm) {
          os << "\t  EPOLLRDNORM: " << ev.epoll_rdnorm << "\n";
        }
        if (ev.epoll_rdband) {
          os << "\t  EPOLLRDBAND: " << ev.epoll_rdband << "\n";
        }
        if (ev.epoll_wrnorm) {
          os << "\t  EPOLLWRNORM: " << ev.epoll_wrnorm << "\n";
        }
        if (ev.epoll_wrband) {
          os << "\t  EPOLLWRBAND: " << ev.epoll_wrband << "\n";
        }
        if (ev.epoll_msg) {
          os << "\t     EPOLLMSG: " << ev.epoll_msg << "\n";
        }
        if (ev.epoll_err) {
          os << "\t     EPOLLERR: " << ev.epoll_err << "\n";
        }
        if (ev.epoll_hup) {
          os << "\t     EPOLLHUP: " << ev.epoll_hup << "\n";
        }
        if (ev.epoll_oneshot) {
          os << "\t EPOLLONESHOT: " << ev.epoll_oneshot << "\n";
        }
        if (ev.epoll_et) {
          os << "\t      EPOLLET: " << ev.epoll_et << "\n";
        }
        return os;
      }

      unsigned int epoll_in;
      unsigned int epoll_pri;
      unsigned int epoll_out;
      unsigned int epoll_rdnorm;
      unsigned int epoll_rdband;
      unsigned int epoll_wrnorm;
      unsigned int epoll_wrband;
      unsigned int epoll_msg;
      unsigned int epoll_err;
      unsigned int epoll_hup;
      unsigned int epoll_oneshot;
      unsigned int epoll_et;
    };

    std::vector<DebugOutput*> debug_events_;
    std::vector<Events> unregistered_fds_;
    using EventCountsMap = std::unordered_map<int, Events>;
    EventCountsMap event_counts_;
    int64_t num_records_;
    int64_t record_threshold_;
  };

  void ClearEventRecords() {
    event_recorder_.Clear();
  }
  void WriteEventRecords(ostream* os) const {
    (*os) << event_recorder_;
  }

  mutable EventRecorder event_recorder_;

#endif

 private:
  // Helper functions used in the destructor.
  void CleanupFDToCBMap();
  void CleanupTimeToAlarmCBMap();

  // The callback registered to the fds below.  As the purpose of their
  // registration is to wake the epoll server it just clears the pipe and
  // returns.
  std::unique_ptr<ReadPipeCallback> wake_cb_;

  // A pipe owned by the epoll server.  The server will be registered to listen
  // on read_fd_ and can be woken by Wake() which writes to write_fd_.
  int read_fd_;
  int write_fd_;

  // This boolean is checked to see if it is false at the top of the
  // WaitForEventsAndExecuteCallbacks function. If not, then it either returns
  // without doing work, and logs to ERROR, or aborts the program (in
  // DEBUG mode). If so, then it sets the bool to true, does work, and
  // sets it back to false when done. This catches unwanted recursion.
  bool in_wait_for_events_and_execute_callbacks_;

  // Returns true when the EpollServer() is being destroyed.
  bool in_shutdown_;

  DISALLOW_COPY_AND_ASSIGN(EpollServer);
};

class EpollAlarmCallbackInterface {
 public:
  // Summary:
  //   Called when an alarm times out. Invalidates an AlarmRegToken.
  //   WARNING: If a token was saved to refer to an alarm callback, OnAlarm must
  //   delete it, as the reference is no longer valid.
  // Returns:
  //   the unix time (in microseconds) at which this alarm should be signaled
  //   again, or 0 if the alarm should be removed.
  virtual int64_t OnAlarm() = 0;

  // Summary:
  //   Called when the an alarm is registered. Invalidates an AlarmRegToken.
  // Args:
  //   token: the iterator to the the alarm registered in the alarm map.
  //   WARNING: this token becomes invalid when the alarm fires, is
  //   unregistered, or OnShutdown is called on that alarm.
  //   eps: the epoll server the alarm is registered with.
  virtual void OnRegistration(const EpollServer::AlarmRegToken& token,
                              EpollServer* eps) = 0;

  // Summary:
  //   Called when the an alarm is unregistered.
  //   WARNING: It is not valid to unregister a callback and then use the token
  //   that was saved to refer to the callback.
  virtual void OnUnregistration() = 0;

  // Summary:
  //   Called when the epoll server is shutting down.
  //   Invalidates the AlarmRegToken that was given when this alarm was
  //   registered.
  virtual void OnShutdown(EpollServer* eps) = 0;

  virtual ~EpollAlarmCallbackInterface() {}

 protected:
  EpollAlarmCallbackInterface() {}
};

// A simple alarm which unregisters itself on destruction.
//
// PLEASE NOTE:
// Any classes overriding these functions must either call the implementation
// of the parent class, or is must otherwise make sure that the 'registered_'
// boolean and the token, 'token_', are updated appropriately.
class EpollAlarm : public EpollAlarmCallbackInterface {
 public:
  EpollAlarm();

  ~EpollAlarm() override;

  // Marks the alarm as unregistered and returns 0.  The return value may be
  // safely ignored by subclasses.
  int64_t OnAlarm() override;

  // Marks the alarm as registered, and stores the token.
  void OnRegistration(const EpollServer::AlarmRegToken& token,
                      EpollServer* eps) override;

  // Marks the alarm as unregistered.
  void OnUnregistration() override;

  // Marks the alarm as unregistered.
  void OnShutdown(EpollServer* eps) override;

  // If the alarm was registered, unregister it.
  void UnregisterIfRegistered();

  bool registered() const { return registered_; }

  const EpollServer* eps() const { return eps_; }

 private:
  EpollServer::AlarmRegToken token_;
  EpollServer* eps_;
  bool registered_;
};

}  // namespace net

#endif  // NET_TOOLS_EPOLL_SERVER_EPOLL_SERVER_H_
