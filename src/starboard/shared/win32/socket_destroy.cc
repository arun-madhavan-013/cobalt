// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "starboard/socket.h"

#include <winsock2.h>

#include "starboard/log.h"
#include "starboard/shared/win32/socket_internal.h"
#include "starboard/socket_waiter.h"

bool SbSocketDestroy(SbSocket socket) {
  if (!SbSocketIsValid(socket)) {
    return false;
  }

  SB_DCHECK(socket->socket_handle != INVALID_SOCKET);

  if (socket->waiter != nullptr) {
    bool result = SbSocketWaiterRemove(socket->waiter, socket);
    SB_DCHECK(result);
  }

  bool result = closesocket(socket->socket_handle) != SOCKET_ERROR;

  delete socket;
  return result;
}
