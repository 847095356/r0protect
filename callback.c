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

void mydUnload(IN PDRIVER_OBJECT DriverObject)   //��һЩ��β ����Ĺ���
{
	//CmUnRegisterCallback(cookie);
	ObUnRegisterCallbacks(_HANDLE);
}
																					
NTSTATUS RegistryCallback(PVOID callbackcontext, PVOID arg1, PVOID arg2)			//�����һ������Ϊ���Ǵ����������
{																					// arg1 �����˲�������  arg2 �����˲�������
	//DbgPrint("---%p---\n", callbackcontext);

	NTSTATUS status = STATUS_SUCCESS;

	REG_NOTIFY_CLASS tempclass = (REG_NOTIFY_CLASS)arg1;		//ע����ɾ�� �����ȶ���
																//ǰ�������ڴ򿪻��߶�ȡ �����������������

	UNICODE_STRING tempname = { 0 };

	RtlInitUnicodeString(&tempname, L"*���� #1");
	

	switch (tempclass)
	{
	case RegNtPreOpenKeyEx:						//��Vista��ʼ ע��������û�в���Ex�������ﻹ��ֻ��Ϊ�˼��� RegNtPreOpenKey:
	{
													//DbgPrint("Open key!\n");

													break;
	}
	case RegNtPreCreateKeyEx:					//win32�� openֻ�ܴ�ע��� Ȼ�� createkey �ȿ��Դ��ֿ��Դ�������
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

	//status = CmRegisterCallback(RegistryCallback, (PVOID)0x123456, &cookie);		//�ڶ�������Ϊ������ �ǻص������ĵ�һ������

	//
	// windows��һ�����ڶ���Ĳ���ϵͳ
	//

	PLDR_DATA_TABLE_ENTRY ldr = (PLDR_DATA_TABLE_ENTRY)DriverObject->DriverSection;

	ldr->Flags |= 0x20;

	OB_CALLBACK_REGISTRATION ob = { 0 };		//�ص�

	OB_OPERATION_REGISTRATION oor = { 0 };		//����ǻص�����

	UNICODE_STRING atitude = { 0 };		//���̻ص�������֪ͨ˭ 

	ob.Version = ObGetFilterVersion();

	ob.OperationRegistrationCount = 1;

	ob.OperationRegistration = &oor;

	RtlInitUnicodeString(&atitude, L"350000");

	ob.Altitude = atitude;

	oor.ObjectType = ExEventObjectType;			//ExEventObjectType

	oor.Operations = OB_OPERATION_HANDLE_CREATE | OB_OPERATION_HANDLE_DUPLICATE;		//�������о������ ���� ���� ��ʱ�� ֪ͨ
																						//���︴�Ƶ���˼�� �����Դ���������� ĳ����������Ҫ�Ľ�����ô ĳ��������Դ���Դ�������и��� ���̾����ʹ��
	oor.PreOperation = ProcessPreCallback;

	oor.PostOperation = NULL;

	status = ObRegisterCallbacks(&ob, &_HANDLE);

	return status;
}
