// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/transport_client_socket_pool.h"

#include <algorithm>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/metrics/histogram.h"
#include "base/string_util.h"
#include "base/time.h"
#include "base/values.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_log.h"
#include "net/base/net_errors.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/client_socket_pool_base.h"
#include "net/socket/socket_net_log_params.h"
#include "net/socket/tcp_client_socket.h"

using base::TimeDelta;

namespace net {

// TODO(willchan): Base this off RTT instead of statically setting it. Note we
// choose a timeout that is different from the backup connect job timer so they
// don't synchronize.
const int TransportConnectJob::kIPv6FallbackTimerInMs = 300;

namespace {

// Returns true iff all addresses in |list| are in the IPv6 family.
bool AddressListOnlyContainsIPv6(const AddressList& list) {
  DCHECK(!list.empty());
  for (AddressList::const_iterator iter = list.begin(); iter != list.end();
       ++iter) {
    if (iter->GetFamily() != ADDRESS_FAMILY_IPV6)
      return false;
  }
  return true;
}

}  // namespace

TransportSocketParams::TransportSocketParams(
    const HostPortPair& host_port_pair,
    RequestPriority priority,
    bool disable_resolver_cache,
    bool ignore_limits,
    const OnHostResolutionCallback& host_resolution_callback)
    : destination_(host_port_pair),
      ignore_limits_(ignore_limits),
      host_resolution_callback_(host_resolution_callback) {
  Initialize(priority, disable_resolver_cache);
}

TransportSocketParams::~TransportSocketParams() {}

void TransportSocketParams::Initialize(RequestPriority priority,
                                       bool disable_resolver_cache) {
  destination_.set_priority(priority);
  if (disable_resolver_cache)
    destination_.set_allow_cached_response(false);
}

// TransportConnectJobs will time out after this many seconds.  Note this is
// the total time, including both host resolution and TCP connect() times.
//
// TODO(eroman): The use of this constant needs to be re-evaluated. The time
// needed for TCPClientSocketXXX::Connect() can be arbitrarily long, since
// the address list may contain many alternatives, and most of those may
// timeout. Even worse, the per-connect timeout threshold varies greatly
// between systems (anywhere from 20 seconds to 190 seconds).
// See comment #12 at http://crbug.com/23364 for specifics.
static const int kTransportConnectJobTimeoutInSeconds = 240;  // 4 minutes.

TransportConnectJob::TransportConnectJob(
    const std::string& group_name,
    const scoped_refptr<TransportSocketParams>& params,
    base::TimeDelta timeout_duration,
    ClientSocketFactory* client_socket_factory,
    HostResolver* host_resolver,
    Delegate* delegate,
    NetLog* net_log)
    : ConnectJob(group_name, timeout_duration, delegate,
                 BoundNetLog::Make(net_log, NetLog::SOURCE_CONNECT_JOB)),
      params_(params),
      client_socket_factory_(client_socket_factory),
      resolver_(host_resolver),
      next_state_(STATE_NONE) {
}

TransportConnectJob::~TransportConnectJob() {
  // We don't worry about cancelling the host resolution and TCP connect, since
  // ~SingleRequestHostResolver and ~StreamSocket will take care of it.
}

LoadState TransportConnectJob::GetLoadState() const {
  switch (next_state_) {
    case STATE_RESOLVE_HOST:
    case STATE_RESOLVE_HOST_COMPLETE:
      return LOAD_STATE_RESOLVING_HOST;
    case STATE_TRANSPORT_CONNECT:
    case STATE_TRANSPORT_CONNECT_COMPLETE:
      return LOAD_STATE_CONNECTING;
    default:
      NOTREACHED();
      return LOAD_STATE_IDLE;
  }
}

// static
void TransportConnectJob::MakeAddressListStartWithIPv4(AddressList* list) {
  for (AddressList::iterator i = list->begin(); i != list->end(); ++i) {
    if (i->GetFamily() == ADDRESS_FAMILY_IPV4) {
      std::rotate(list->begin(), i, list->end());
      break;
    }
  }
}

void TransportConnectJob::OnIOComplete(int result) {
  int rv = DoLoop(result);
  if (rv != ERR_IO_PENDING)
    NotifyDelegateOfCompletion(rv);  // Deletes |this|
}

int TransportConnectJob::DoLoop(int result) {
  DCHECK_NE(next_state_, STATE_NONE);

  int rv = result;
  do {
    State state = next_state_;
    next_state_ = STATE_NONE;
    switch (state) {
      case STATE_RESOLVE_HOST:
        DCHECK_EQ(OK, rv);
        rv = DoResolveHost();
        break;
      case STATE_RESOLVE_HOST_COMPLETE:
        rv = DoResolveHostComplete(rv);
        break;
      case STATE_TRANSPORT_CONNECT:
        DCHECK_EQ(OK, rv);
        rv = DoTransportConnect();
        break;
      case STATE_TRANSPORT_CONNECT_COMPLETE:
        rv = DoTransportConnectComplete(rv);
        break;
      default:
        NOTREACHED();
        rv = ERR_FAILED;
        break;
    }
  } while (rv != ERR_IO_PENDING && next_state_ != STATE_NONE);

  return rv;
}

int TransportConnectJob::DoResolveHost() {
  next_state_ = STATE_RESOLVE_HOST_COMPLETE;
  return resolver_.Resolve(
      params_->destination(), &addresses_,
      base::Bind(&TransportConnectJob::OnIOComplete, base::Unretained(this)),
      net_log());
}

int TransportConnectJob::DoResolveHostComplete(int result) {
  if (result == OK) {
#ifdef STARBOARD
    // Preferentially connect to an IPv4 address first, if available. Some
    // hosts may have IPv6 addresses to which we can connect, but the read
    // may still fail if the network is not properly configured. The existing
    // code has a fallback mechanism to try different IPs in |addresses_|
    // when connection fails. However, in this case, a connection can be made
    // with the IPv6 address, but the read fails.
    MakeAddressListStartWithIPv4(&addresses_);
#endif

    // Invoke callback, and abort if it fails.
    if (!params_->host_resolution_callback().is_null())
      result = params_->host_resolution_callback().Run(addresses_, net_log());

    if (result == OK)
      next_state_ = STATE_TRANSPORT_CONNECT;
  }
  return result;
}

int TransportConnectJob::DoTransportConnect() {
  next_state_ = STATE_TRANSPORT_CONNECT_COMPLETE;
  transport_socket_.reset(client_socket_factory_->CreateTransportClientSocket(
        addresses_, net_log().net_log(), net_log().source()));
  connect_start_time_ = base::TimeTicks::Now();
  int rv = transport_socket_->Connect(
      base::Bind(&TransportConnectJob::OnIOComplete, base::Unretained(this)));
  if (rv == ERR_IO_PENDING &&
      addresses_.front().GetFamily() == ADDRESS_FAMILY_IPV6 &&
      !AddressListOnlyContainsIPv6(addresses_)) {
    fallback_timer_.Start(FROM_HERE,
        base::TimeDelta::FromMilliseconds(kIPv6FallbackTimerInMs),
        this, &TransportConnectJob::DoIPv6FallbackTransportConnect);
  }
  return rv;
}

int TransportConnectJob::DoTransportConnectComplete(int result) {
  if (result == OK) {
    bool is_ipv4 = addresses_.front().GetFamily() == ADDRESS_FAMILY_IPV4;
    DCHECK(connect_start_time_ != base::TimeTicks());
    DCHECK(start_time_ != base::TimeTicks());
    base::TimeTicks now = base::TimeTicks::Now();
    base::TimeDelta total_duration = now - start_time_;
    UMA_HISTOGRAM_CUSTOM_TIMES(
        "Net.DNS_Resolution_And_TCP_Connection_Latency2",
        total_duration,
        base::TimeDelta::FromMilliseconds(1),
        base::TimeDelta::FromMinutes(10),
        100);

    base::TimeDelta connect_duration = now - connect_start_time_;
    UMA_HISTOGRAM_CUSTOM_TIMES("Net.TCP_Connection_Latency",
        connect_duration,
        base::TimeDelta::FromMilliseconds(1),
        base::TimeDelta::FromMinutes(10),
        100);

    if (is_ipv4) {
      UMA_HISTOGRAM_CUSTOM_TIMES("Net.TCP_Connection_Latency_IPv4_No_Race",
                                 connect_duration,
                                 base::TimeDelta::FromMilliseconds(1),
                                 base::TimeDelta::FromMinutes(10),
                                 100);
    } else {
      if (AddressListOnlyContainsIPv6(addresses_)) {
        UMA_HISTOGRAM_CUSTOM_TIMES("Net.TCP_Connection_Latency_IPv6_Solo",
                                   connect_duration,
                                   base::TimeDelta::FromMilliseconds(1),
                                   base::TimeDelta::FromMinutes(10),
                                   100);
      } else {
        UMA_HISTOGRAM_CUSTOM_TIMES("Net.TCP_Connection_Latency_IPv6_Raceable",
                                   connect_duration,
                                   base::TimeDelta::FromMilliseconds(1),
                                   base::TimeDelta::FromMinutes(10),
                                   100);
      }
    }
    set_socket(transport_socket_.release());
    fallback_timer_.Stop();
  } else {
    // Be a bit paranoid and kill off the fallback members to prevent reuse.
    fallback_transport_socket_.reset();
    fallback_addresses_.reset();
  }

  return result;
}

void TransportConnectJob::DoIPv6FallbackTransportConnect() {
  // The timer should only fire while we're waiting for the main connect to
  // succeed.
  if (next_state_ != STATE_TRANSPORT_CONNECT_COMPLETE) {
    NOTREACHED();
    return;
  }

  DCHECK(!fallback_transport_socket_.get());
  DCHECK(!fallback_addresses_.get());

  fallback_addresses_.reset(new AddressList(addresses_));
  MakeAddressListStartWithIPv4(fallback_addresses_.get());
  fallback_transport_socket_.reset(
      client_socket_factory_->CreateTransportClientSocket(
          *fallback_addresses_, net_log().net_log(), net_log().source()));
  fallback_connect_start_time_ = base::TimeTicks::Now();
  int rv = fallback_transport_socket_->Connect(
      base::Bind(
          &TransportConnectJob::DoIPv6FallbackTransportConnectComplete,
          base::Unretained(this)));
  if (rv != ERR_IO_PENDING)
    DoIPv6FallbackTransportConnectComplete(rv);
}

void TransportConnectJob::DoIPv6FallbackTransportConnectComplete(int result) {
  // This should only happen when we're waiting for the main connect to succeed.
  if (next_state_ != STATE_TRANSPORT_CONNECT_COMPLETE) {
    NOTREACHED();
    return;
  }

  DCHECK_NE(ERR_IO_PENDING, result);
  DCHECK(fallback_transport_socket_.get());
  DCHECK(fallback_addresses_.get());

  if (result == OK) {
    DCHECK(fallback_connect_start_time_ != base::TimeTicks());
    DCHECK(start_time_ != base::TimeTicks());
    base::TimeTicks now = base::TimeTicks::Now();
    base::TimeDelta total_duration = now - start_time_;
    UMA_HISTOGRAM_CUSTOM_TIMES(
        "Net.DNS_Resolution_And_TCP_Connection_Latency2",
        total_duration,
        base::TimeDelta::FromMilliseconds(1),
        base::TimeDelta::FromMinutes(10),
        100);

    base::TimeDelta connect_duration = now - fallback_connect_start_time_;
    UMA_HISTOGRAM_CUSTOM_TIMES("Net.TCP_Connection_Latency",
        connect_duration,
        base::TimeDelta::FromMilliseconds(1),
        base::TimeDelta::FromMinutes(10),
        100);

    UMA_HISTOGRAM_CUSTOM_TIMES("Net.TCP_Connection_Latency_IPv4_Wins_Race",
        connect_duration,
        base::TimeDelta::FromMilliseconds(1),
        base::TimeDelta::FromMinutes(10),
        100);
    set_socket(fallback_transport_socket_.release());
    next_state_ = STATE_NONE;
    transport_socket_.reset();
  } else {
    // Be a bit paranoid and kill off the fallback members to prevent reuse.
    fallback_transport_socket_.reset();
    fallback_addresses_.reset();
  }
  NotifyDelegateOfCompletion(result);  // Deletes |this|
}

int TransportConnectJob::ConnectInternal() {
  next_state_ = STATE_RESOLVE_HOST;
  start_time_ = base::TimeTicks::Now();
  return DoLoop(OK);
}

ConnectJob*
    TransportClientSocketPool::TransportConnectJobFactory::NewConnectJob(
    const std::string& group_name,
    const PoolBase::Request& request,
    ConnectJob::Delegate* delegate) const {
  return new TransportConnectJob(group_name,
                                 request.params(),
                                 ConnectionTimeout(),
                                 client_socket_factory_,
                                 host_resolver_,
                                 delegate,
                                 net_log_);
}

base::TimeDelta
    TransportClientSocketPool::TransportConnectJobFactory::ConnectionTimeout()
    const {
  return base::TimeDelta::FromSeconds(kTransportConnectJobTimeoutInSeconds);
}

TransportClientSocketPool::TransportClientSocketPool(
    int max_sockets,
    int max_sockets_per_group,
    ClientSocketPoolHistograms* histograms,
    HostResolver* host_resolver,
    ClientSocketFactory* client_socket_factory,
    NetLog* net_log)
    : base_(max_sockets, max_sockets_per_group, histograms,
            ClientSocketPool::unused_idle_socket_timeout(),
            ClientSocketPool::used_idle_socket_timeout(),
            new TransportConnectJobFactory(client_socket_factory,
                                     host_resolver, net_log)) {
  base_.EnableConnectBackupJobs();
}

TransportClientSocketPool::~TransportClientSocketPool() {}

int TransportClientSocketPool::RequestSocket(
    const std::string& group_name,
    const void* params,
    RequestPriority priority,
    ClientSocketHandle* handle,
    const CompletionCallback& callback,
    const BoundNetLog& net_log) {
  const scoped_refptr<TransportSocketParams>* casted_params =
      static_cast<const scoped_refptr<TransportSocketParams>*>(params);

  if (net_log.IsLoggingAllEvents()) {
    // TODO(eroman): Split out the host and port parameters.
    net_log.AddEvent(
        NetLog::TYPE_TCP_CLIENT_SOCKET_POOL_REQUESTED_SOCKET,
        CreateNetLogHostPortPairCallback(
            &casted_params->get()->destination().host_port_pair()));
  }

  return base_.RequestSocket(group_name, *casted_params, priority, handle,
                             callback, net_log);
}

void TransportClientSocketPool::RequestSockets(
    const std::string& group_name,
    const void* params,
    int num_sockets,
    const BoundNetLog& net_log) {
  const scoped_refptr<TransportSocketParams>* casted_params =
      static_cast<const scoped_refptr<TransportSocketParams>*>(params);

  if (net_log.IsLoggingAllEvents()) {
    // TODO(eroman): Split out the host and port parameters.
    net_log.AddEvent(
        NetLog::TYPE_TCP_CLIENT_SOCKET_POOL_REQUESTED_SOCKETS,
        CreateNetLogHostPortPairCallback(
            &casted_params->get()->destination().host_port_pair()));
  }

  base_.RequestSockets(group_name, *casted_params, num_sockets, net_log);
}

void TransportClientSocketPool::CancelRequest(
    const std::string& group_name,
    ClientSocketHandle* handle) {
  base_.CancelRequest(group_name, handle);
}

void TransportClientSocketPool::ReleaseSocket(
    const std::string& group_name,
    StreamSocket* socket,
    int id) {
  base_.ReleaseSocket(group_name, socket, id);
}

void TransportClientSocketPool::FlushWithError(int error) {
  base_.FlushWithError(error);
}

bool TransportClientSocketPool::IsStalled() const {
  return base_.IsStalled();
}

void TransportClientSocketPool::CloseIdleSockets() {
  base_.CloseIdleSockets();
}

int TransportClientSocketPool::IdleSocketCount() const {
  return base_.idle_socket_count();
}

int TransportClientSocketPool::IdleSocketCountInGroup(
    const std::string& group_name) const {
  return base_.IdleSocketCountInGroup(group_name);
}

LoadState TransportClientSocketPool::GetLoadState(
    const std::string& group_name, const ClientSocketHandle* handle) const {
  return base_.GetLoadState(group_name, handle);
}

void TransportClientSocketPool::AddLayeredPool(LayeredPool* layered_pool) {
  base_.AddLayeredPool(layered_pool);
}

void TransportClientSocketPool::RemoveLayeredPool(LayeredPool* layered_pool) {
  base_.RemoveLayeredPool(layered_pool);
}

DictionaryValue* TransportClientSocketPool::GetInfoAsValue(
    const std::string& name,
    const std::string& type,
    bool include_nested_pools) const {
  return base_.GetInfoAsValue(name, type);
}

base::TimeDelta TransportClientSocketPool::ConnectionTimeout() const {
  return base_.ConnectionTimeout();
}

ClientSocketPoolHistograms* TransportClientSocketPool::histograms() const {
  return base_.histograms();
}

}  // namespace net
