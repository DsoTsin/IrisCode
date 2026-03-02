#include "ntsocket.h"
#include <windows.h>

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID64 Pointer;
  } DUMMYUNIONNAME;
  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

#if _WIN64
typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  ULONG Reserved;
  PCWSTR Buffer;
} UNICODE_STRING;
#elif _WIN32
typedef struct _UNICODE_STRING {
  USHORT Length;
  USHORT MaximumLength;
  PCWSTR Buffer;
} UNICODE_STRING;
#endif

typedef UNICODE_STRING* PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
  ULONG Length;
  HANDLE RootDirectory;
  PUNICODE_STRING ObjectName;
  ULONG Attributes;
  PVOID SecurityDescriptor;       // Points to type SECURITY_DESCRIPTOR
  PVOID SecurityQualityOfService; // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES* POBJECT_ATTRIBUTES;

#define OBJ_INHERIT 0x00000002L
#define OBJ_PERMANENT 0x00000010L
#define OBJ_EXCLUSIVE 0x00000020L
#define OBJ_CASE_INSENSITIVE 0x00000040L
#define OBJ_OPENIF 0x00000080L
#define OBJ_OPENLINK 0x00000100L
#define OBJ_KERNEL_HANDLE 0x00000200L
#define OBJ_FORCE_ACCESS_CHECK 0x00000400L
#define OBJ_VALID_ATTRIBUTES 0x000007F2L
#define FILE_OPEN_IF 0x00000003
#define STATUS_SUCCESS 0x0
#define AF_INET6 0x17
#define InitializeObjectAttributes(p, n, a, r, s)                                                  \
  {                                                                                                \
    (p)->Length = sizeof(OBJECT_ATTRIBUTES);                                                       \
    (p)->RootDirectory = (r);                                                                      \
    (p)->Attributes = (a);                                                                         \
    (p)->ObjectName = (n);                                                                         \
    (p)->SecurityDescriptor = (s);                                                                 \
    (p)->SecurityQualityOfService = NULL;                                                          \
  }

typedef enum _EVENT_TYPE { NotificationEvent, SynchronizationEvent } EVENT_TYPE;
#if _WIN64
typedef NTSTATUS (*NtDeviceIoControlFile)(HANDLE FileHandle,
                                          HANDLE Event,
                                          PVOID ApcRoutine,
                                          PVOID ApcContext,
                                          PIO_STATUS_BLOCK IoStatusBlock,
                                          ULONG IoControlCode,
                                          PVOID InputBuffer,
                                          ULONG InputBufferLength,
                                          PVOID OutputBuffer,
                                          ULONG OutputBufferLength);
typedef NTSTATUS (*NtCreateEvent)(PHANDLE EventHandle,
                                  ACCESS_MASK DesiredAccess,
                                  POBJECT_ATTRIBUTES ObjectAttributes,
                                  EVENT_TYPE EventType,
                                  BOOLEAN InitialState);
typedef NTSTATUS (*NtCreateFile)(PHANDLE FileHandle,
                                 ACCESS_MASK DesiredAccess,
                                 POBJECT_ATTRIBUTES ObjectAttributes,
                                 PIO_STATUS_BLOCK IoStatusBlock,
                                 PLARGE_INTEGER AllocateSize,
                                 ULONG FileAttributes,
                                 ULONG ShareAccess,
                                 ULONG CreateDisposition,
                                 ULONG CreateOptions,
                                 PVOID EaBuffer,
                                 ULONG EaLength);
typedef NTSTATUS (*RtlIpv6StringToAddressExA)(PVOID AddressString,
                                              byte* Address,
                                              PULONG ScopeId,
                                              PUSHORT Port);
#elif _WIN32
typedef NTSTATUS(__stdcall* NtDeviceIoControlFile)(HANDLE FileHandle,
                                                   HANDLE Event,
                                                   PVOID ApcRoutine,
                                                   PVOID ApcContext,
                                                   PIO_STATUS_BLOCK IoStatusBlock,
                                                   ULONG IoControlCode,
                                                   PVOID InputBuffer,
                                                   ULONG InputBufferLength,
                                                   PVOID OutputBuffer,
                                                   ULONG OutputBufferLength);
typedef NTSTATUS(__stdcall* NtCreateEvent)(PHANDLE EventHandle,
                                           ACCESS_MASK DesiredAccess,
                                           POBJECT_ATTRIBUTES ObjectAttributes,
                                           EVENT_TYPE EventType,
                                           BOOLEAN InitialState);
typedef NTSTATUS(__stdcall* NtCreateFile)(PHANDLE FileHandle,
                                          ACCESS_MASK DesiredAccess,
                                          POBJECT_ATTRIBUTES ObjectAttributes,
                                          PIO_STATUS_BLOCK IoStatusBlock,
                                          PLARGE_INTEGER AllocateSize,
                                          ULONG FileAttributes,
                                          ULONG ShareAccess,
                                          ULONG CreateDisposition,
                                          ULONG CreateOptions,
                                          PVOID EaBuffer,
                                          ULONG EaLength);
typedef NTSTATUS(__stdcall* RtlIpv6StringToAddressExA)(PVOID AddressString,
                                                       byte* Address,
                                                       PULONG ScopeId,
                                                       PUSHORT Port);
#endif
/* AFD Packet Endpoint Flags */
#define AFD_ENDPOINT_CONNECTIONLESS 0x1
#define AFD_ENDPOINT_MESSAGE_ORIENTED 0x10
#define AFD_ENDPOINT_RAW 0x100
#define AFD_ENDPOINT_MULTIPOINT 0x1000
#define AFD_ENDPOINT_C_ROOT 0x10000
#define AFD_ENDPOINT_D_ROOT 0x100000

/* AFD TDI Query Flags */
#define AFD_ADDRESS_HANDLE 0x1L
#define AFD_CONNECTION_HANDLE 0x2L

/* AFD event bits */
#define AFD_EVENT_RECEIVE_BIT 0
#define AFD_EVENT_OOB_RECEIVE_BIT 1
#define AFD_EVENT_SEND_BIT 2
#define AFD_EVENT_DISCONNECT_BIT 3
#define AFD_EVENT_ABORT_BIT 4
#define AFD_EVENT_CLOSE_BIT 5
#define AFD_EVENT_CONNECT_BIT 6
#define AFD_EVENT_ACCEPT_BIT 7
#define AFD_EVENT_CONNECT_FAIL_BIT 8
#define AFD_EVENT_QOS_BIT 9
#define AFD_EVENT_GROUP_QOS_BIT 10
#define AFD_EVENT_ROUTING_INTERFACE_CHANGE_BIT 11
#define AFD_EVENT_ADDRESS_LIST_CHANGE_BIT 12
#define AFD_MAX_EVENT 13
#define AFD_ALL_EVENTS ((1 << AFD_MAX_EVENT) - 1)

/* AFD Info Flags */
#define AFD_INFO_INLINING_MODE 0x01L
#define AFD_INFO_BLOCKING_MODE 0x02L
#define AFD_INFO_SENDS_IN_PROGRESS 0x04L
#define AFD_INFO_RECEIVE_WINDOW_SIZE 0x06L
#define AFD_INFO_SEND_WINDOW_SIZE 0x07L
#define AFD_INFO_GROUP_ID_TYPE 0x10L
#define AFD_INFO_RECEIVE_CONTENT_SIZE 0x11L

/* AFD Share Flags */
#define AFD_SHARE_UNIQUE 0x0L
#define AFD_SHARE_REUSE 0x1L
#define AFD_SHARE_WILDCARD 0x2L
#define AFD_SHARE_EXCLUSIVE 0x3L

/* AFD Disconnect Flags */
#define AFD_DISCONNECT_SEND 0x01L
#define AFD_DISCONNECT_RECV 0x02L
#define AFD_DISCONNECT_ABORT 0x04L
#define AFD_DISCONNECT_DATAGRAM 0x08L

/* AFD Event Flags */
#define AFD_EVENT_RECEIVE (1 << AFD_EVENT_RECEIVE_BIT)
#define AFD_EVENT_OOB_RECEIVE (1 << AFD_EVENT_OOB_RECEIVE_BIT)
#define AFD_EVENT_SEND (1 << AFD_EVENT_SEND_BIT)
#define AFD_EVENT_DISCONNECT (1 << AFD_EVENT_DISCONNECT_BIT)
#define AFD_EVENT_ABORT (1 << AFD_EVENT_ABORT_BIT)
#define AFD_EVENT_CLOSE (1 << AFD_EVENT_CLOSE_BIT)
#define AFD_EVENT_CONNECT (1 << AFD_EVENT_CONNECT_BIT)
#define AFD_EVENT_ACCEPT (1 << AFD_EVENT_ACCEPT_BIT)
#define AFD_EVENT_CONNECT_FAIL (1 << AFD_EVENT_CONNECT_FAIL_BIT)
#define AFD_EVENT_QOS (1 << AFD_EVENT_QOS_BIT)
#define AFD_EVENT_GROUP_QOS (1 << AFD_EVENT_GROUP_QOS_BIT)
#define AFD_EVENT_ROUTING_INTERFACE_CHANGE (1 << AFD_EVENT_ROUTING_INTERFACE_CHANGE_BIT)
#define AFD_EVENT_ADDRESS_LIST_CHANGE (1 << AFD_EVENT_ADDRESS_LIST_CHANGE_BIT)

/* AFD SEND/RECV Flags */
#define AFD_SKIP_FIO 0x1L
#define AFD_OVERLAPPED 0x2L
#define AFD_IMMEDIATE 0x4L

#define TDI_SEND_EXPEDITED ((USHORT)0x0020)            // TSDU is/was urgent/expedited.
#define TDI_SEND_PARTIAL ((USHORT)0x0040)              // TSDU is/was terminated by an EOR.
#define TDI_SEND_NO_RESPONSE_EXPECTED ((USHORT)0x0080) // HINT: no back traffic expected.
#define TDI_SEND_NON_BLOCKING ((USHORT)0x0100)         // don't block if no buffer space in protocol
#define TDI_SEND_AND_DISCONNECT                                                                    \
  ((USHORT)0x0200) // Piggy back disconnect to remote and do not
                   // indicate disconnect from remote
/* IOCTL Generation */
#define FSCTL_AFD_BASE FILE_DEVICE_NETWORK
#define _AFD_CONTROL_CODE(Operation, Method) ((FSCTL_AFD_BASE) << 12 | (Operation << 2) | Method)

/* AFD Commands */
#define AFD_BIND 0
#define AFD_CONNECT 1
#define AFD_START_LISTEN 2
#define AFD_WAIT_FOR_LISTEN 3
#define AFD_ACCEPT 4
#define AFD_RECV 5
#define AFD_RECV_DATAGRAM 6
#define AFD_SEND 7
#define AFD_SEND_DATAGRAM 8
#define AFD_SELECT 9
#define AFD_DISCONNECT 10
#define AFD_GET_SOCK_NAME 11
#define AFD_GET_PEER_NAME 12
#define AFD_GET_TDI_HANDLES 13
#define AFD_SET_INFO 14
#define AFD_GET_CONTEXT_SIZE 15
#define AFD_GET_CONTEXT 16
#define AFD_SET_CONTEXT 17
#define AFD_SET_CONNECT_DATA 18
#define AFD_SET_CONNECT_OPTIONS 19
#define AFD_SET_DISCONNECT_DATA 20
#define AFD_SET_DISCONNECT_OPTIONS 21
#define AFD_GET_CONNECT_DATA 22
#define AFD_GET_CONNECT_OPTIONS 23
#define AFD_GET_DISCONNECT_DATA 24
#define AFD_GET_DISCONNECT_OPTIONS 25
#define AFD_SET_CONNECT_DATA_SIZE 26
#define AFD_SET_CONNECT_OPTIONS_SIZE 27
#define AFD_SET_DISCONNECT_DATA_SIZE 28
#define AFD_SET_DISCONNECT_OPTIONS_SIZE 29
#define AFD_GET_INFO 30
#define AFD_EVENT_SELECT 33
#define AFD_ENUM_NETWORK_EVENTS 34
#define AFD_DEFER_ACCEPT 35
#define AFD_GET_PENDING_CONNECT_DATA 41
#define AFD_VALIDATE_GROUP 42

/* AFD IOCTLs */

#define IOCTL_AFD_BIND _AFD_CONTROL_CODE(AFD_BIND, METHOD_NEITHER)
#define IOCTL_AFD_CONNECT _AFD_CONTROL_CODE(AFD_CONNECT, METHOD_NEITHER)
#define IOCTL_AFD_START_LISTEN _AFD_CONTROL_CODE(AFD_START_LISTEN, METHOD_NEITHER)
#define IOCTL_AFD_WAIT_FOR_LISTEN _AFD_CONTROL_CODE(AFD_WAIT_FOR_LISTEN, METHOD_BUFFERED)
#define IOCTL_AFD_ACCEPT _AFD_CONTROL_CODE(AFD_ACCEPT, METHOD_BUFFERED)
#define IOCTL_AFD_RECV _AFD_CONTROL_CODE(AFD_RECV, METHOD_NEITHER)
#define IOCTL_AFD_RECV_DATAGRAM _AFD_CONTROL_CODE(AFD_RECV_DATAGRAM, METHOD_NEITHER)
#define IOCTL_AFD_SEND _AFD_CONTROL_CODE(AFD_SEND, METHOD_NEITHER)
#define IOCTL_AFD_SEND_DATAGRAM _AFD_CONTROL_CODE(AFD_SEND_DATAGRAM, METHOD_NEITHER)
#define IOCTL_AFD_SELECT _AFD_CONTROL_CODE(AFD_SELECT, METHOD_BUFFERED)
#define IOCTL_AFD_DISCONNECT _AFD_CONTROL_CODE(AFD_DISCONNECT, METHOD_NEITHER)
#define IOCTL_AFD_GET_SOCK_NAME _AFD_CONTROL_CODE(AFD_GET_SOCK_NAME, METHOD_NEITHER)
#define IOCTL_AFD_GET_PEER_NAME _AFD_CONTROL_CODE(AFD_GET_PEER_NAME, METHOD_NEITHER)
#define IOCTL_AFD_GET_TDI_HANDLES _AFD_CONTROL_CODE(AFD_GET_TDI_HANDLES, METHOD_NEITHER)
#define IOCTL_AFD_SET_INFO _AFD_CONTROL_CODE(AFD_SET_INFO, METHOD_NEITHER)
#define IOCTL_AFD_GET_CONTEXT_SIZE _AFD_CONTROL_CODE(AFD_GET_CONTEXT_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_GET_CONTEXT _AFD_CONTROL_CODE(AFD_GET_CONTEXT, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONTEXT _AFD_CONTROL_CODE(AFD_SET_CONTEXT, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONNECT_DATA _AFD_CONTROL_CODE(AFD_SET_CONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONNECT_OPTIONS _AFD_CONTROL_CODE(AFD_SET_CONNECT_OPTIONS, METHOD_NEITHER)
#define IOCTL_AFD_SET_DISCONNECT_DATA _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_SET_DISCONNECT_OPTIONS                                                           \
  _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_OPTIONS, METHOD_NEITHER)
#define IOCTL_AFD_GET_CONNECT_DATA _AFD_CONTROL_CODE(AFD_GET_CONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_GET_CONNECT_OPTIONS _AFD_CONTROL_CODE(AFD_GET_CONNECT_OPTIONS, METHOD_NEITHER)
#define IOCTL_AFD_GET_DISCONNECT_DATA _AFD_CONTROL_CODE(AFD_GET_DISCONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_GET_DISCONNECT_OPTIONS                                                           \
  _AFD_CONTROL_CODE(AFD_GET_DISCONNECT_OPTIONS, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONNECT_DATA_SIZE _AFD_CONTROL_CODE(AFD_SET_CONNECT_DATA_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_SET_CONNECT_OPTIONS_SIZE                                                         \
  _AFD_CONTROL_CODE(AFD_SET_CONNECT_OPTIONS_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_SET_DISCONNECT_DATA_SIZE                                                         \
  _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_DATA_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_SET_DISCONNECT_OPTIONS_SIZE                                                      \
  _AFD_CONTROL_CODE(AFD_SET_DISCONNECT_OPTIONS_SIZE, METHOD_NEITHER)
#define IOCTL_AFD_GET_INFO _AFD_CONTROL_CODE(AFD_GET_INFO, METHOD_NEITHER)
#define IOCTL_AFD_EVENT_SELECT _AFD_CONTROL_CODE(AFD_EVENT_SELECT, METHOD_NEITHER)
#define IOCTL_AFD_DEFER_ACCEPT _AFD_CONTROL_CODE(AFD_DEFER_ACCEPT, METHOD_NEITHER)
#define IOCTL_AFD_GET_PENDING_CONNECT_DATA                                                         \
  _AFD_CONTROL_CODE(AFD_GET_PENDING_CONNECT_DATA, METHOD_NEITHER)
#define IOCTL_AFD_ENUM_NETWORK_EVENTS _AFD_CONTROL_CODE(AFD_ENUM_NETWORK_EVENTS, METHOD_NEITHER)
#define IOCTL_AFD_VALIDATE_GROUP _AFD_CONTROL_CODE(AFD_VALIDATE_GROUP, METHOD_NEITHER)

struct sockaddr_in4 {
  short sin_family;
  u_short sin_port;
  ULONG sin_addr;
  char sin_zero[8];
};

struct sockaddr_in6 {
  USHORT sin6_family;
  USHORT sin6_port;
  ULONG sin6_flowinfo;
  byte sin6_addr[16];
  ULONG sin6_scope_id;
};

struct AFD_BindData {
  ULONG ShareType;
  sockaddr Address;
};

struct AFD_BindData_IPv6 {
  ULONG ShareType;
  sockaddr_in6 Address;
};

struct AFD_ConnctInfo {
  size_t UseSAN;
  size_t Root;
  size_t Unknown;
  sockaddr Address;
};

struct AFD_ConnctInfo_IPv6 {
  size_t UseSAN;
  size_t Root;
  size_t Unknown;
  sockaddr_in6 Address;
};

struct AFD_Wsbuf {
  UINT len;
  PVOID buf;
};

struct AFD_SendRecvInfo {
  PVOID BufferArray;
  ULONG BufferCount;
  ULONG AfdFlags;
  ULONG TdiFlags;
};

struct AFD_ListenInfo {
  BOOLEAN UseSAN;
  ULONG Backlog;
  BOOLEAN UseDelayedAcceptance;
};

struct AFD_EventSelectInfo {
  HANDLE EventObject;
  ULONG Events;
};

struct AFD_EnumNetworkEventsInfo {
  ULONG Event;
  ULONG PollEvents;
  NTSTATUS EventStatus[12];
};

struct AFD_AcceptData {
  ULONG UseSAN;
  ULONG SequenceNumber;
  HANDLE ListenHandle;
};

struct AFD_PollInfo {
  LARGE_INTEGER Timeout;
  ULONG HandleCount;
  ULONG Exclusive;
  SOCKET Handle;
  ULONG Events;
  NTSTATUS Status;
};

struct AFD_AsyncData {
  SOCKET NowSocket;
  PVOID UserContext;
  IO_STATUS_BLOCK IOSB;
  AFD_PollInfo PollInfo;
};

NtDeviceIoControlFile MyNtDeviceIoControlFile;
NtCreateEvent MyNtCreateEvent;
NtCreateFile MyNtCreateFile;
RtlIpv6StringToAddressExA MyRtlIpv6StringToAddressExA;

void __nt_socket_init() {
  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (ntdll == 0) {
    return;
  }
  MyNtDeviceIoControlFile = (NtDeviceIoControlFile)GetProcAddress(ntdll, "NtDeviceIoControlFile");
  MyNtCreateEvent = (NtCreateEvent)GetProcAddress(ntdll, "NtCreateEvent");
  MyNtCreateFile = (NtCreateFile)GetProcAddress(ntdll, "NtCreateFile");
  MyRtlIpv6StringToAddressExA
      = (RtlIpv6StringToAddressExA)GetProcAddress(ntdll, "RtlIpv6StringToAddressExA");
}

uintptr_t __nt_socket_create(int AddressFamily, int SocketType, int Protocol) {
  /// <summary>
  /// 类似于Socket函数，可以创建一个Socket文件句柄
  /// </summary>
  /// <param name="AddressFamily">Address family(Support IPv6)</param>
  /// <param name="SocketType">Socket Type</param>
  /// <param name="Protocol">Protocol type</param>
  /// <returns>如果失败返回INVALID_SOCKET，成功返回Socket文件句柄</returns>
  if (AddressFamily == AF_UNSPEC && SocketType == 0 && Protocol == 0) {
    return INVALID_SOCKET;
  }
  //进行基础数据设置
  if (AddressFamily == AF_UNSPEC) {
    AddressFamily = AF_INET;
  }
  if (SocketType == 0) {
    switch (Protocol) {
    case IPPROTO_TCP:
      SocketType = SOCK_STREAM;
      break;
    case IPPROTO_UDP:
      SocketType = SOCK_DGRAM;
      break;
    case IPPROTO_RAW:
      SocketType = SOCK_RAW;
      break;
    default:
      SocketType = SOCK_STREAM;
      break;
    }
  }
  if (Protocol == 0) {
    switch (SocketType) {
    case SOCK_STREAM:
      Protocol = IPPROTO_TCP;
      break;
    case SOCK_DGRAM:
      Protocol = IPPROTO_UDP;
      break;
    case SOCK_RAW:
      Protocol = IPPROTO_RAW;
      break;
    default:
      Protocol = IPPROTO_TCP;
      break;
    }
  }
  byte EaBuffer[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x1E, 0x00, 0x41, 0x66, 0x64, 0x4F, 0x70,
                     0x65, 0x6E, 0x50, 0x61, 0x63, 0x6B, 0x65, 0x74, 0x58, 0x58, 0x00, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
                     0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x01, 0x00, 0x00,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  memmove((PVOID)((__int64)EaBuffer + 32), &AddressFamily, 0x4);
  memmove((PVOID)((__int64)EaBuffer + 36), &SocketType, 0x4);
  memmove((PVOID)((__int64)EaBuffer + 40), &Protocol, 0x4);
  if (Protocol == IPPROTO_UDP) {
    memmove((PVOID)((__int64)EaBuffer + 24), &Protocol, 0x4);
  }
  //初始化UNICODE_STRING：
  UNICODE_STRING AfdName;
  AfdName.Buffer = L"\\Device\\Afd\\Endpoint";
  AfdName.Length = 2 * wcslen(AfdName.Buffer);
  AfdName.MaximumLength = AfdName.Length + 2;
  OBJECT_ATTRIBUTES Object;
  IO_STATUS_BLOCK IOSB;
  //初始化OBJECT_ATTRIBUTES
  InitializeObjectAttributes(&Object, &AfdName, OBJ_CASE_INSENSITIVE | OBJ_INHERIT, 0, 0);
  HANDLE MySock;
  NTSTATUS Status;
  //创建AfdSocket：
  Status
      = ((NtCreateFile)MyNtCreateFile)(&MySock, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &Object,
                                       &IOSB, NULL, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       FILE_OPEN_IF, 0, EaBuffer, sizeof(EaBuffer));
  if (Status != STATUS_SUCCESS) {
    return INVALID_SOCKET;
  } else {
    return (SOCKET)MySock;
  }
}

int __nt_socket_close(uintptr_t Handle) {
  /// <summary>
  /// 关闭一个socket
  /// </summary>
  /// <param name="Handle">socket句柄</param>
  /// <returns>返回NTSTATUS，表明函数的实际调用情况</returns>
  if (Handle == 0) {
    return -1;
  }
  NTSTATUS Status;
  HANDLE SockEvent = NULL;
  //创建一个等待事件
  Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent, EVENT_ALL_ACCESS, NULL,
                                            SynchronizationEvent, FALSE);
  if (Status != STATUS_SUCCESS) {
    //创建Event失败！
    return Status;
  }
  byte DisconnetInfo[] = {0x00, 0x00, 0x00, 0x00, 0xC0, 0xBD, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  //设置断开信息
  int DisconnectType
      = AFD_DISCONNECT_SEND | AFD_DISCONNECT_RECV | AFD_DISCONNECT_ABORT | AFD_DISCONNECT_DATAGRAM;
  //全部操作都断开
  IO_STATUS_BLOCK IOSB;
  memmove(&DisconnetInfo, &DisconnectType, 0x4);
  Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle, SockEvent, NULL, NULL,
                                                              &IOSB, IOCTL_AFD_DISCONNECT,
                                                              &DisconnetInfo, sizeof(DisconnetInfo),
                                                              NULL, 0);
  //发送请求
  if (Status == STATUS_PENDING) {
    WaitForSingleObject(SockEvent, INFINITE);
    //这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
    Status = IOSB.Status;
  }
  //关闭事件句柄
  CloseHandle((HANDLE)Handle);
  //关闭sokcet句柄
  CloseHandle(SockEvent);
  return Status;
}

int __nt_socket_bind(uintptr_t Handle, const void* SocketAddress, int ShareType) {
  /// <summary>
  /// Bind一个地址
  /// </summary>
  /// <param name="Handle">socket句柄</param>
  /// <param name="SocketAddress">绑定的IPv4地址</param>
  /// <param name="ShareType">请填写AFD_SHARE_开头常量，若无特殊需求请填AFD_SHARE_WILDCARD</param>
  /// <returns></returns>
  if (Handle == 0) {
    return -1;
  }
  NTSTATUS Status;
  HANDLE SockEvent = NULL;
  //创建一个等待事件
  Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent, EVENT_ALL_ACCESS, NULL,
                                            SynchronizationEvent, FALSE);
  if (Status != STATUS_SUCCESS) {
    //创建Event失败！
    return Status;
  }
  AFD_BindData BindConfig;
  IO_STATUS_BLOCK IOSB;
  memset(&BindConfig, 0, sizeof(BindConfig));
  BindConfig.ShareType = ShareType;
  BindConfig.Address = *(sockaddr*)SocketAddress;
  byte OutputBlock[40];
  //作反馈消息
  memset(&OutputBlock, 0, sizeof(OutputBlock));
  //发送IOCTL_AFD_BIND消息
  Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle, SockEvent, NULL, NULL,
                                                              &IOSB, IOCTL_AFD_BIND, &BindConfig,
                                                              sizeof(BindConfig), &OutputBlock,
                                                              sizeof(OutputBlock));
  if (Status == STATUS_PENDING) {
    WaitForSingleObject(SockEvent, INFINITE);
    //这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
    Status = IOSB.Status;
  }
  //关闭事件句柄
  CloseHandle(SockEvent);
  return Status;
}

int __nt_socket_connect(uintptr_t Handle, const void* SocketAddress) {
  /// <summary>
  /// 连接一个IPv4地址，大致用法同Connect函数
  /// </summary>
  /// <param name="Handle">socket句柄</param>
  /// <param name="SocketAddress">连接的地址</param>
  /// <returns></returns>
  if (Handle == 0) {
    return -1;
  }
  sockaddr SockAddr;
  memset(&SockAddr, 0, sizeof(SockAddr));
  SockAddr.sa_family = AF_INET;
  NTSTATUS Status;
  Status = __nt_socket_bind(Handle, &SockAddr, AFD_SHARE_REUSE);
  if (Status != ERROR_SUCCESS) {
    return Status;
  }
  //创建一个等待事件
  HANDLE SockEvent = NULL;
  Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent, EVENT_ALL_ACCESS, NULL,
                                            SynchronizationEvent, FALSE);
  if (Status != STATUS_SUCCESS) {
    //创建Event失败！
    return Status;
  }
  AFD_ConnctInfo ConnectInfo;
  IO_STATUS_BLOCK IOSB;
  ConnectInfo.UseSAN = 0;
  ConnectInfo.Root = 0;
  ConnectInfo.Unknown = 0;
  ConnectInfo.Address = *(sockaddr*)SocketAddress;
  //发送IOCTL_AFD_BIND消息
  Status
      = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle, SockEvent, NULL, NULL,
                                                           &IOSB, IOCTL_AFD_CONNECT, &ConnectInfo,
                                                           sizeof(ConnectInfo), NULL, 0);
  if (Status == STATUS_PENDING) {
    WaitForSingleObject(SockEvent, INFINITE);
    //这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
    Status = IOSB.Status;
  }
  //关闭事件句柄
  CloseHandle(SockEvent);
  return Status;
}

int __nt_socket_shutdown(uintptr_t Handle, int HowTo) {
  /// <summary>
  /// Shutdown指定操作，同WSAShutdown使用
  /// </summary>
  /// <param name="Handle">socket句柄</param>
  /// <param name="HowTo">请填写AFD Disconnect Flags相关常量（AFD_DISCONNECT开头常量）</param>
  /// <returns></returns>
  if (Handle == 0) {
    return -1;
  }
  HANDLE SockEvent = NULL;
  NTSTATUS Status;
  Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent, EVENT_ALL_ACCESS, NULL,
                                            SynchronizationEvent, FALSE);
  if (Status != STATUS_SUCCESS) {
    //创建Event失败！
    return Status;
  }
  byte DisconnetInfo[] = {0x00, 0x00, 0x00, 0x00, 0xC0, 0xBD, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  //设置断开信息
  memmove(&DisconnetInfo, &HowTo, 0x4);
  IO_STATUS_BLOCK IOSB;
  Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle, SockEvent, NULL, NULL,
                                                              &IOSB, IOCTL_AFD_DISCONNECT,
                                                              &DisconnetInfo, sizeof(DisconnetInfo),
                                                              NULL, 0);
  if (Status == STATUS_PENDING) {
    WaitForSingleObject(SockEvent, INFINITE);
    //这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
    Status = IOSB.Status;
  }
  //关闭事件句柄
  CloseHandle(SockEvent);
  return Status;
}

int __nt_socket_recv(uintptr_t Handle, void* lpBuffers, uint32_t* lpNumberOfBytesRead) {
  /// <summary>
  /// 接收数据，基本实现了recv的基础功能
  /// </summary>
  /// <param name="Handle">socket句柄</param>
  /// <param name="lpBuffers">缓冲区，记得自行分配内存！！</param>
  /// <param name="lpNumberOfBytesRead">读取的数据量，本参数会同时返回实际读取的长度</param>
  /// <returns></returns>
  if (Handle == 0) {
    return -1;
  }
  HANDLE SockEvent = NULL;
  NTSTATUS Status;
  Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent, EVENT_ALL_ACCESS, NULL,
                                            SynchronizationEvent, FALSE);
  if (Status != STATUS_SUCCESS) {
    //创建Event失败！
    return Status;
  }
  AFD_SendRecvInfo SendRecvInfo;
  memset(&SendRecvInfo, 0, sizeof(SendRecvInfo));
  //设置发送的BufferCount
  SendRecvInfo.BufferCount = 1;
  SendRecvInfo.AfdFlags = 0;
  SendRecvInfo.TdiFlags = 0x20;
  //设置发送的Buffer
  AFD_Wsbuf SendRecvBuffer;
  SendRecvBuffer.len = *lpNumberOfBytesRead;
  SendRecvBuffer.buf = lpBuffers;
  SendRecvInfo.BufferArray = &SendRecvBuffer;
  IO_STATUS_BLOCK IOSB;
  //发送IOCTL_AFD_RECV
  Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle, SockEvent, NULL, NULL,
                                                              &IOSB, IOCTL_AFD_RECV, &SendRecvInfo,
                                                              sizeof(SendRecvInfo), NULL, 0);
  if (Status == STATUS_PENDING) {
    WaitForSingleObject(SockEvent, INFINITE);
    //这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
    Status = IOSB.Status;
    *lpNumberOfBytesRead = IOSB.Information;
  }
  if (Status == STATUS_SUCCESS) {
    *lpNumberOfBytesRead = IOSB.Information;
  }
  //关闭事件句柄
  CloseHandle(SockEvent);
  return Status;
}

int __nt_socket_send(uintptr_t Handle,
                     const void* lpBuffers,
                     uint32_t lpNumberOfBytesSent,
                     int iFlags) {
  /// <summary>
  /// 发送数据，基本实现send功能
  /// </summary>
  /// <param name="Handle">socket句柄</param>
  /// <param name="lpBuffers">要发送的数据的指针</param>
  /// <param name="lpNumberOfBytesSent">要发送的字节数目</param>
  /// <param name="iFlags">Flags，设置发送标识（比如MSG_OOB一类）</param>
  /// <returns></returns>
  if (Handle == 0) {
    return -1;
  }
  HANDLE SockEvent = NULL;
  NTSTATUS Status;
  Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent, EVENT_ALL_ACCESS, NULL,
                                            SynchronizationEvent, FALSE);
  if (Status != STATUS_SUCCESS) {
    //创建Event失败！
    return Status;
  }
  AFD_SendRecvInfo SendRecvInfo;
  memset(&SendRecvInfo, 0, sizeof(SendRecvInfo));
  SendRecvInfo.BufferCount = 1;
  SendRecvInfo.AfdFlags = 0;
  SendRecvInfo.TdiFlags = 0;
  //设置TDIFlag
  if (iFlags) {
    if (iFlags & MSG_OOB) {
      SendRecvInfo.TdiFlags |= TDI_SEND_EXPEDITED;
    }
    if (iFlags & MSG_PARTIAL) {
      SendRecvInfo.TdiFlags |= TDI_SEND_PARTIAL;
    }
  }
  IO_STATUS_BLOCK IOSB;
  DWORD lpNumberOfBytesAlreadySend = 0;
  AFD_Wsbuf SendRecvBuffer;
  //这里需要循环发送，send一次性是不能发送太大的数据的
  do {
    //计算应该发送的数据长度和偏移
    SendRecvBuffer.buf = (PVOID)((__int64)lpBuffers + lpNumberOfBytesAlreadySend);
    SendRecvBuffer.len = lpNumberOfBytesSent - lpNumberOfBytesAlreadySend;
    SendRecvInfo.BufferArray = &SendRecvBuffer;
    //发送数据
    Status
        = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle, SockEvent, NULL, NULL,
                                                             &IOSB, IOCTL_AFD_SEND, &SendRecvInfo,
                                                             sizeof(SendRecvInfo), NULL, 0);
    if (Status == STATUS_PENDING) {
      WaitForSingleObject(SockEvent, INFINITE);
      //这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
    }
    Status = IOSB.Status;
    //获取实际成功发送的长度
    lpNumberOfBytesAlreadySend = IOSB.Information;
    if (Status != STATUS_SUCCESS) {
      CloseHandle(SockEvent);
      return Status;
    }
  } while (lpNumberOfBytesAlreadySend < lpNumberOfBytesSent);
  CloseHandle(SockEvent);
  return Status;
}

namespace iris {
namespace nt {
uint16_t __htons(uint16_t x) {
#if REG_DWORD == REG_DWORD_BIG_ENDIAN
  return x;
#elif REG_DWORD == REG_DWORD_LITTLE_ENDIAN
  return _byteswap_ushort(x);
#else
#error "What kind of system is this?"
#endif
}

u_long __htonl(_In_ u_long hostlong) {
#if REG_DWORD == REG_DWORD_BIG_ENDIAN
  return hostlong;
#elif REG_DWORD == REG_DWORD_LITTLE_ENDIAN
  return _byteswap_ulong(hostlong);
#else
#error "What kind of system is this?"
#endif
}

int __inet_aton(register const char* cp, struct in_addr* addr);

u_long __inet_addr(register const char* cp) {
  struct in_addr val;

  if (__inet_aton(cp, &val))
    return (val.s_addr);
  return (INADDR_NONE);
}

socket::socket() : _(NULL) {
  _ = __nt_socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

socket::~socket() {
  std::lock_guard<std::mutex> lock(mutex);
  __nt_socket_close(_);
}

bool socket::connect(const char* ipv4, int port) {
  sockaddr_in4 address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = __htons(port);
  address.sin_addr = __inet_addr(ipv4);
  std::lock_guard<std::mutex> lock(mutex);
  return __nt_socket_connect(_, &address) == STATUS_SUCCESS;
}

bool socket::send(const string& data) {
  std::lock_guard<std::mutex> lock(mutex);
  return __nt_socket_send(_, data.data(), data.size(), 0) == STATUS_SUCCESS;
}

bool socket::recv(void* data, uint32_t& length) {
  std::lock_guard<std::mutex> lock(mutex);
  return __nt_socket_recv(_, data, &length) == STATUS_SUCCESS;
}

bool socket::shutdown(int how) {
  std::lock_guard<std::mutex> lock(mutex);
  return __nt_socket_shutdown(_, how) == STATUS_SUCCESS;
}

struct api_instance {
  api_instance() { __nt_socket_init(); }
};

__declspec(dllexport) api_instance instance;

int __inet_aton(register const char* cp, struct in_addr* addr) {
  register u_long val;
  register int base, n;
  register char c;
  u_int parts[4];
  register u_int* pp = parts;

  for (;;) {
    /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, other=decimal.
     */
    val = 0;
    base = 10;
    if (*cp == '0') {
      if (*++cp == 'x' || *cp == 'X')
        base = 16, cp++;
      else
        base = 8;
    }
    while ((c = *cp) != '\0') {
      if (isascii(c) && isdigit(c)) {
        val = (val * base) + (c - '0');
        cp++;
        continue;
      }
      if (base == 16 && isascii(c) && isxdigit(c)) {
        val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
        cp++;
        continue;
      }
      break;
    }
    if (*cp == '.') {
      /*
       * Internet format:
       *	a.b.c.d
       *	a.b.c	(with c treated as 16-bits)
       *	a.b	(with b treated as 24 bits)
       */
      if (pp >= parts + 3 || val > 0xff)
        return (0);
      *pp++ = val, cp++;
    } else
      break;
  }
  /*
   * Check for trailing characters.
   */
  if (*cp && (!isascii(*cp) || !isspace(*cp)))
    return (0);
  /*
   * Concoct the address according to
   * the number of parts specified.
   */
  n = pp - parts + 1;
  switch (n) {

  case 1: /* a -- 32 bits */
    break;

  case 2: /* a.b -- 8.24 bits */
    if (val > 0xffffff)
      return (0);
    val |= parts[0] << 24;
    break;

  case 3: /* a.b.c -- 8.8.16 bits */
    if (val > 0xffff)
      return (0);
    val |= (parts[0] << 24) | (parts[1] << 16);
    break;

  case 4: /* a.b.c.d -- 8.8.8.8 bits */
    if (val > 0xff)
      return (0);
    val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
    break;
  }
  if (addr)
    addr->s_addr = __htonl(val);
  return (1);
}
} // namespace nt
} // namespace iris
