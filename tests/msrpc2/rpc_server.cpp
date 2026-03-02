// rpc_server.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "RPCServer.h"
#include "RPCServer_s.c"
#pragma comment(lib, "Rpcrt4.lib")

long StartService();

int main(int argc, char* argv[])
{
    StartService();
    return 0;
}

void __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return (malloc(len));
}
void __RPC_USER midl_user_free(void __RPC_FAR *ptr)
{
    free(ptr);
}

long StartService()
{
    unsigned char pszProtocolSequence[] = "ncacn_np";
    unsigned char *pszSecurity = NULL;
    unsigned char pszEndPoint[] = "\\pipe\\RPCServer"; //命名管道

    RPC_STATUS rpcStats = RpcServerUseProtseqEpA(pszProtocolSequence,
        RPC_C_LISTEN_MAX_CALLS_DEFAULT,
        pszEndPoint,
        pszSecurity);
    if (rpcStats)
        exit(rpcStats);

    rpcStats = RpcServerRegisterIf(RPCServer_v1_0_s_ifspec, NULL, NULL);
    if (rpcStats)
        exit(rpcStats);

    unsigned int fDontWait = false;
    rpcStats = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, fDontWait);
    if (rpcStats)
        exit(rpcStats);

    return 0;
}

void StopService()
{
    RPC_STATUS rpcStatus;
    rpcStatus = RpcMgmtStopServerListening(NULL);
    rpcStatus = RpcServerUnregisterIf(NULL, NULL, FALSE);
}
#define _CRT_SECURE_NO_WARNINGS

error_status_t GetServerName(
    /* [size_is][out] */ unsigned char *pszName,
    /* [in] */ long len)
{
    strncpy_s((char*)pszName, 10, "RPCServer", len);
    pszName[len - 1] = '\0';
    printf("服务已经启动！\n");
    return 0;
}

error_status_t Add(
    /* [in] */ long num1,
    /* [in] */ long num2,
    /* [out] */ long *rtn)
{
    *rtn = num1 + num2;
    return 0;
}

error_status_t ShutDown(void)
{
    StopService();
    printf("服务已经关闭！\n");
    return 0;
}