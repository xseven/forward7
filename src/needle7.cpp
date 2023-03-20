// Need to link with Ws2_32.lib and Iphlpapi.lib
#define _WINSOCKAPI_
#include <Windows.h>

#include <iphlpapi.h>
#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

/* Note: could also use malloc() and free() */

int main()
{

    // Declare and initialize variables.

    /* variables used for GetIfForwardTable */
    PMIB_IPFORWARDTABLE pIpForwardTable;
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    char szDestIp[128];
    char szMaskIp[128];
    char szGatewayIp[128];

    struct in_addr IpAddr;

    int i;

    pIpForwardTable = (MIB_IPFORWARDTABLE*)MALLOC(sizeof(MIB_IPFORWARDTABLE));
    if (pIpForwardTable == NULL) {
        printf("Error allocating memory\n");
        return 1;
    }

    if (GetIpForwardTable(pIpForwardTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER) {
        FREE(pIpForwardTable);
        pIpForwardTable = (MIB_IPFORWARDTABLE*)MALLOC(dwSize);
        if (pIpForwardTable == NULL) {
            printf("Error allocating memory\n");
            return 1;
        }
    }

    /* Note that the IPv4 addresses returned in
     * GetIpForwardTable entries are in network byte order
     */
    if ((dwRetVal = GetIpForwardTable(pIpForwardTable, &dwSize, 0)) == NO_ERROR) {
        printf("\tNumber of entries: %d\n",
            (int)pIpForwardTable->dwNumEntries);
        for (i = 0; i < (int)pIpForwardTable->dwNumEntries; i++) {
            /* Convert IPv4 addresses to strings */
            IpAddr.S_un.S_addr = (u_long)pIpForwardTable->table[i].dwForwardDest;
            strcpy_s(szDestIp, sizeof(szDestIp), inet_ntoa(IpAddr));
            IpAddr.S_un.S_addr = (u_long)pIpForwardTable->table[i].dwForwardMask;
            strcpy_s(szMaskIp, sizeof(szMaskIp), inet_ntoa(IpAddr));
            IpAddr.S_un.S_addr = (u_long)pIpForwardTable->table[i].dwForwardNextHop;
            strcpy_s(szGatewayIp, sizeof(szGatewayIp), inet_ntoa(IpAddr));

            printf("\n\tRoute[%d] Dest IP: %s\n", i, szDestIp);
            printf("\tRoute[%d] Subnet Mask: %s\n", i, szMaskIp);
            printf("\tRoute[%d] Next Hop: %s\n", i, szGatewayIp);
            printf("\tRoute[%d] If Index: %ld\n", i,
                pIpForwardTable->table[i].dwForwardIfIndex);
            printf("\tRoute[%d] Type: %ld - ", i,
                pIpForwardTable->table[i].dwForwardType);
            switch (pIpForwardTable->table[i].dwForwardType) {
            case MIB_IPROUTE_TYPE_OTHER:
                printf("other\n");
                break;
            case MIB_IPROUTE_TYPE_INVALID:
                printf("invalid route\n");
                break;
            case MIB_IPROUTE_TYPE_DIRECT:
                printf("local route where next hop is final destination\n");
                break;
            case MIB_IPROUTE_TYPE_INDIRECT:
                printf("remote route where next hop is not final destination\n");
                break;
            default:
                printf("UNKNOWN Type value\n");
                break;
            }
            printf("\tRoute[%d] Proto: %ld - ", i,
                pIpForwardTable->table[i].dwForwardProto);
            switch (pIpForwardTable->table[i].dwForwardProto) {
            case MIB_IPPROTO_OTHER:
                printf("other\n");
                break;
            case MIB_IPPROTO_LOCAL:
                printf("local interface\n");
                break;
            case MIB_IPPROTO_NETMGMT:
                printf("static route set through network management \n");
                break;
            case MIB_IPPROTO_ICMP:
                printf("result of ICMP redirect\n");
                break;
            case MIB_IPPROTO_EGP:
                printf("Exterior Gateway Protocol (EGP)\n");
                break;
            case MIB_IPPROTO_GGP:
                printf("Gateway-to-Gateway Protocol (GGP)\n");
                break;
            case MIB_IPPROTO_HELLO:
                printf("Hello protocol\n");
                break;
            case MIB_IPPROTO_RIP:
                printf("Routing Information Protocol (RIP)\n");
                break;
            case MIB_IPPROTO_IS_IS:
                printf("Intermediate System-to-Intermediate System (IS-IS) protocol\n");
                break;
            case MIB_IPPROTO_ES_IS:
                printf("End System-to-Intermediate System (ES-IS) protocol\n");
                break;
            case MIB_IPPROTO_CISCO:
                printf("Cisco Interior Gateway Routing Protocol (IGRP)\n");
                break;
            case MIB_IPPROTO_BBN:
                printf("BBN Internet Gateway Protocol (IGP) using SPF\n");
                break;
            case MIB_IPPROTO_OSPF:
                printf("Open Shortest Path First (OSPF) protocol\n");
                break;
            case MIB_IPPROTO_BGP:
                printf("Border Gateway Protocol (BGP)\n");
                break;
            case MIB_IPPROTO_NT_AUTOSTATIC:
                printf("special Windows auto static route\n");
                break;
            case MIB_IPPROTO_NT_STATIC:
                printf("special Windows static route\n");
                break;
            case MIB_IPPROTO_NT_STATIC_NON_DOD:
                printf("special Windows static route not based on Internet standards\n");
                break;
            default:
                printf("UNKNOWN Proto value\n");
                break;
            }

            printf("\tRoute[%d] Age: %ld\n", i,
                pIpForwardTable->table[i].dwForwardAge);
            printf("\tRoute[%d] Metric1: %ld\n", i,
                pIpForwardTable->table[i].dwForwardMetric1);
        }
        FREE(pIpForwardTable);
        return 0;
    } else {
        printf("\tGetIpForwardTable failed.\n");
        FREE(pIpForwardTable);
        return 1;
    }
}
#if 0
#include <WinSock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#include <cstdio>
#include <format>
#include <string>

namespace {
const int counterTreshold { 50 };
int counter { 0 };

std::string srcIp;
std::string dstIp;

uint16_t srcPort;
uint16_t dstPort;
}

void CALLBACK f(HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime)
{
    printf("Packet src: %s dst: %s %i\n", srcIp.c_str(), dstIp.c_str(), dstPort);

    if (counter++ >= counterTreshold) {
        PostQuitMessage(0);
    }

    const auto payload = std::format("src: {} dst: {} port: {}", srcIp, dstIp, dstPort);

    sockaddr_in dest;
    sockaddr_in local;
    WSAData data;
    WSAStartup(MAKEWORD(2, 2), &data);

    local.sin_family = AF_INET;
    inet_pton(AF_INET, srcIp.c_str(), &local.sin_addr.s_addr);
    local.sin_port = htons(srcPort);

    dest.sin_family = AF_INET;
    inet_pton(AF_INET, dstIp.c_str(), &dest.sin_addr.s_addr);
    dest.sin_port = htons(dstPort);

    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    auto bindRes = bind(s, (sockaddr*)&local, sizeof(local));
    if (bindRes != 0) {
        printf("bind res: %i\n", WSAGetLastError());
    }

    sendto(s, payload.c_str(), strlen(payload.c_str()), 0, (sockaddr*)&dest, sizeof(dest));

    closesocket(s);
    WSACleanup();
}

int main(int argc, char** argv)
{
    if (argc == 5) {
        srcIp = argv[1];
        dstIp = argv[2];
        srcPort = std::stoi(argv[3]);
        dstPort = std::stoi(argv[4]);
    } else {
        return -1;
    }

    MSG msg;

    SetTimer(NULL, 0, 1000 * 2, (TIMERPROC)&f);
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
#endif
