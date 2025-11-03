/* -----------------------------------------------------------------------------
 * Copyright (c) 2020-2025 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * -------------------------------------------------------------------------- */

#include <stdio.h>
#include "cmsis_os2.h"
#include "rl_net.h"

static uint8_t LinkUp;

/* Ethernet event callback function */
void netETH_Notify (uint32_t if_num, netETH_Event event, uint32_t val) {

  if (event == netETH_LinkUp) {
    LinkUp = 1U;
  }
  if (event == netETH_LinkDown) {
    LinkUp = 0U;
  }
}

int32_t socket_startup (void) {
  char buf[INET_ADDRSTRLEN];
  uint32_t addr,tout;

  LinkUp = 0U;

  printf("Initializing sockets...\n");
  netInitialize();

  /* Wait for link up */
  for (tout = 5000U; tout; tout -= 200U) {
    if (LinkUp == 0U) {
      osDelay(200U);
    }
  }
  if (LinkUp == 0U) {
    printf("ERROR: Network cable not connected.\n");
    return -1;
  }

  /* Wait for DHCP */
  for (tout = 10000U; tout; tout -= 200U) {
    netIF_GetOption(NET_IF_CLASS_ETH | 0,
                    netIF_OptionIP4_Address,
                    (uint8_t *)&addr, sizeof (addr));
    if (addr != 0U) {
      break;
    }
    osDelay(200U);
  }
  if (addr == 0U) {
    printf("ERROR: IP address not assigned.\n");
    return -1;
  }
  netIP_ntoa(NET_ADDR_IP4, (uint8_t *)&addr, buf, sizeof(buf));
  printf("IP address: %s\n", buf);

  return 0;
}
