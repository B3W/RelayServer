/*
 * RelayServer - A simple relay server with packet loss simulation
 * Copyright (C) 2019  Weston Berg
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>

#define UPPER_THRESHOLD   100
#define LOWER_THRESHOLD     0

volatile sig_atomic_t sigint_rec = 0; /* Set when SIGINT signal recieved */

/*
 * Handler for SIGINT signal
 */
void sigint_handler(int i)
{
  sigint_rec = 1;
}

/*
 * Prints out command line format
 */
static void usage(void)
{
  printf("Usage: ./relayServer <srcIP> <srcPort> "
	 "<destIP> <destPort> <lossRate>\n");
}

/*
 * Determine if the packet is lost
 */
static int packet_lost(int lossRate)
{
  int randNum = (rand() % (UPPER_THRESHOLD - LOWER_THRESHOLD + 1)) + LOWER_THRESHOLD;
  if(randNum < lossRate) {
    return 1;
  }
  return 0;
}

/*
 * Main loop
 */
int main(int argc, char *argv[])
{
  char srcIP[16], destIP[16];        /* Src & Dest IP addresses */
  unsigned short srcPort, destPort;  /* Src & Dest port numbers */
  int lossRate;                      /* Packet loss rate for transmission */
  int sockfd;                        /* Socket file descriptor */
  int opt = 1;                       /* setsockopt option value */
                                     /* Structs containing address info */
  struct sockaddr_in servAddr, srcAddr, destAddr;
  socklen_t srcLen, destLen;         /* Length of address structs */
  ssize_t inMsgLen, outMsgLen;       /* Result of recvfrom/sendto */
  size_t bufSz = 2048;               /* Size of packets */
  char buffer[bufSz];                /* Buffer to write into/read out of */
  struct sigaction sa;               /* Configures sigaction call */

  // Setup handler for sigation to catch SIGINT
  sa.sa_handler = sigint_handler;
  sigaction(SIGINT, &sa, NULL);

  // Seed random number generator
  srand(time(NULL));

  // Retrieve command line args
  if(argc == 6) {
    strcpy(srcIP, argv[1]);
    srcPort = atoi(argv[2]);
    strcpy(destIP, argv[3]);
    destPort = atoi(argv[4]);
    lossRate = atoi(argv[5]);
  } else {
    usage();
    exit(EXIT_FAILURE);
  }

  // Open socket
  printf("Creating and configuring socket...\n");
  if((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket failure");
    exit(EXIT_FAILURE);
  }

  // Configure socket for reuse
  if((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) == -1) {
    perror("setsockopt failure");
    exit(EXIT_FAILURE);
  }

  // Init and populate address structs
  printf("Initializing local and remote machine addresses...\n");
  memset(&servAddr, 0, sizeof(servAddr));
  memset(&srcAddr, 0, sizeof(srcAddr));
  memset(&destAddr, 0, sizeof(destAddr));

  servAddr.sin_family = AF_INET;
  servAddr.sin_port = htons(srcPort);
  servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

  srcAddr.sin_family = AF_INET;
  srcAddr.sin_port = htons(srcPort);
  srcAddr.sin_addr.s_addr = inet_addr(srcIP);
  srcLen = sizeof(srcAddr);

  destAddr.sin_family = AF_INET;
  destAddr.sin_port = htons(destPort);
  destAddr.sin_addr.s_addr = inet_addr(destIP);
  destLen = sizeof(destAddr);

  // Bind the socket
  printf("Binding socket...\n");
  if((bind(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr))) == -1) {
    perror("bind failure");
    exit(EXIT_FAILURE);
  }

  // Continuously recieve and forward data over socket
  printf("Listening over socket...\n");
  errno = 0;
  while(!sigint_rec) {
    // Recieve data from source
    inMsgLen = recvfrom(sockfd, (char *) buffer, bufSz, MSG_WAITALL,
			(struct sockaddr *) &srcAddr, &srcLen);

    if(inMsgLen == -1) {
      if(errno == EINTR) {
	continue;
      }
      perror("recvfrom failure");
    }

    // Control loss rate
    if(!packet_lost(lossRate)) {
      // Forward recieved data to destination
      outMsgLen = sendto(sockfd, (const char *) buffer, inMsgLen, MSG_CONFIRM,
			 (const struct sockaddr *) &destAddr, destLen);
      
      if(outMsgLen == -1) {
	if(errno == EINTR) {
	  continue;
	}
	perror("sendto failure");
      }
    }
  }

  close(sockfd);
  printf("\nSocket closed\n");

  return 0;
}
