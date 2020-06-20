#include <ntifs.h>
#include <wdm.h>
#include <windef.h>

NTKERNELAPI UCHAR* PsGetProcessImageFileName(__in PEPROCESS Process);

typedef struct _LDR_DATA_TABLE_ENTRY {
	// Start from Windows XP
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	USHORT LoadCount;
	USHORT TlsIndex;
	union {
		LIST_ENTRY HashLinks;
		struct {
			PVOID SectionPointer;
			ULONG CheckSum;
		};
	};
	union {
		ULONG TimeDateStamp;
		PVOID LoadedImports;
	};
	PVOID EntryPointActivationContext;        //_ACTIVATION_CONTEXT *
	PVOID PatchInformation;

	// Start from Windows Vista
	LIST_ENTRY ForwarderLinks;
	LIST_ENTRY ServiceTagLinks;
	LIST_ENTRY StaticLinks;
	PVOID ContextInformation;
	PVOID OriginalBase;
	LARGE_INTEGER LoadTime;

} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

LARGE_INTEGER cookie = { 0 };
PVOID _HANDLE = NULL;

void mydUnload(IN PDRIVER_OBJECT DriverObject)   //做一些收尾 清理的工作
{
	//CmUnRegisterCallback(cookie);
	ObUnRegisterCallbacks(_HANDLE);
}
																					
NTSTATUS RegistryCallback(PVOID callbackcontext, PVOID arg1, PVOID arg2)			//这里第一个参数为我们传入的上下文
{																					// arg1 决定了操作类型  arg2 决定了操作数据
	//DbgPrint("---%p---\n", callbackcontext);

	NTSTATUS status = STATUS_SUCCESS;

	REG_NOTIFY_CLASS tempclass = (REG_NOTIFY_CLASS)arg1;		//注册表的删除 创建等动作
																//前操作用于打开或者读取 后操作可以用于拦截

	UNICODE_STRING tempname = { 0 };

	RtlInitUnicodeString(&tempname, L"*新项 #1");
	

	switch (tempclass)
	{
	case RegNtPreOpenKeyEx:						//从Vista开始 注册表操作就没有不带Ex的了这里还有只是为了兼容 RegNtPreOpenKey:
	{
													//DbgPrint("Open key!\n");

													break;
	}
	case RegNtPreCreateKeyEx:					//win32中 open只能打开注册表 然而 createkey 既可以打开又可以创建所以
	{													//DbgPrint("Open key or Create key!\n");

													PREG_CREATE_KEY_INFORMATION pkeyinfo = (PREG_CREATE_KEY_INFORMATION)arg2;

													__try
													{
														DbgPrint("target reg ---%wZ---", tempname);
														DbgPrint("-----%wZ------\n", pkeyinfo->CompleteName);

														if (FsRtlIsNameInExpression(&tempname, pkeyinfo->CompleteName,TRUE,NULL))
														{
															DbgPrint("bad create!\n");
															status = STATUS_UNSUCCESSFUL;
														}

														DbgPrint("-----%wZ------\n",pkeyinfo->CompleteName);
														
													}
													__except (1)
													{
														DbgPrint("error!\n");
													}

													break;
	}

	default:
		break;
	}

	return status;
}



OB_PREOP_CALLBACK_STATUS ProcessPreCallback(PVOID  RegistrationContext,POB_PRE_OPERATION_INFORMATION  OperationInformation)
{
	
	//PUCHAR imagefilename = PsGetProcessImageFileName(OperationInformation->Object);
	
	/*if (strstr(imagefilename,"calc"))
	{
		OperationInformation->Parameters->CreateHandleInformation.DesiredAccess = 0;
		OperationInformation->Parameters->DuplicateHandleInformation.DesiredAccess = 0;

	}*/

	DbgPrint("!!!!!");
	
	return OB_PREOP_SUCCESS;
}



NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING  RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

	DriverObject->DriverUnload = mydUnload;

	//status = CmRegisterCallback(RegistryCallback, (PVOID)0x123456, &cookie);		//第二个参数为上下文 是回调函数的第一个参数

	//
	// windows是一个基于对象的操作系统
	//

	PLDR_DATA_TABLE_ENTRY ldr = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;

	ldr->Flags |= 0x20;

	OB_CALLBACK_REGISTRATION ob = { 0 };		//回调

	OB_OPERATION_REGISTRATION oor = { 0 };		//这个是回调例程

	UNICODE_STRING atitude = { 0 };		//进程回调对象先通知谁 

	ob.Version = ObGetFilterVersion();

	ob.OperationRegistrationCount = 1;

	ob.OperationRegistration = &oor;

	RtlInitUnicodeString(&atitude, L"350000");

	ob.Altitude = atitude;

	oor.ObjectType = ExEventObjectType;			//ExEventObjectType

	oor.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;		//这里是有句柄创建 或者 复制 的时候 通知
																						//这里复制的意思是 如果资源管理器打开了 某个程序所需要的进程那么 某个程序可以从资源管理器中复制 进程句柄来使用
	oor.PreOperation = ProcessPreCallback;

	oor.PostOperation = NULL;

	status = ObRegisterCallbacks(&ob, &_HANDLE);

	return status;
}
