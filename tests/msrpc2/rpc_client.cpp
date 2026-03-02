// rpc_client.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <stdio.h>
#pragma comment(lib, "Rpcrt4.lib")
#include "RPCServer_c.c"

void CallServerFuntions();

int main(int argc, char* argv[])
{
    unsigned char *pszUuid = NULL;
    unsigned char pszProtocolSequence[] = "ncacn_np";
    unsigned char *pszNetworkAddress = NULL;
    unsigned char pszEndpoint[] = "\\pipe\\RPCServer";
    unsigned char *pszOptions = NULL;
    unsigned char *pszStringBinding = NULL;

    RPC_STATUS rpcStatus = RpcStringBindingComposeA(pszUuid,
        pszProtocolSequence,
        pszNetworkAddress,
        pszEndpoint,
        pszOptions,
        &pszStringBinding);
    if (rpcStatus)
        exit(rpcStatus);

    rpcStatus = RpcBindingFromStringBindingA(pszStringBinding,
        &RPCServer_IfHandle);
    if (rpcStatus)
        exit(rpcStatus);

    RpcTryExcept
    {
        CallServerFuntions();
    }
        RpcExcept(1)
    {
        unsigned long ulCode = RpcExceptionCode();
        printf("抛出异常0x%lx = %ld。\n", ulCode, ulCode);
    }
    RpcEndExcept

    rpcStatus = RpcStringFreeA(&pszStringBinding);
    if (rpcStatus)
        exit(rpcStatus);

    rpcStatus = RpcBindingFree(&RPCServer_IfHandle);
    if (rpcStatus)
        exit(rpcStatus);
    return 0;

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

void CallServerFuntions()
{
    unsigned char pszName[64] = { 0 };
    GetServerName(pszName, 64); //获取服务名称
    printf("Server Name is: %s\n", pszName);

    long num1 = 2;
    long num2 = 2;
    long nSum = 0;
    Add(num1, num2, &nSum); //加法求值c
    printf("%d + %d = %d\n", num1, num2, nSum);

    ShutDown(); //关闭服务
}