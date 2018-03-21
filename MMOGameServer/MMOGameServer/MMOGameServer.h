#ifndef _GAMESERVER_GAMESERVER_MMOGAMESERVER_H_
#define _GAMESERVER_GAMESERVER_MMOGAMESERVER_H_

#pragma comment(lib, "ws2_32")
#pragma comment(lib, "winmm.lib")

#include "Packet.h"
#include "RingBuffer.h"
#include "MemoryPool.h"
#include "LockFreeStack.h"
#include "LockFreeQueue.h"
#include "Player.h"
#include "Log.h"
#include "Dump.h"

#define		WORKER_THREAD_MAX		3
#define		WSABUF_MAX				300
#define		AUTH_MAX				5
#define		GAME_MAX				3
#define		RELEASE_MAX				100

class CMMOServer
{
public:
	CMMOServer(int iMaxSession);
	virtual ~CMMOServer();

	bool Start(WCHAR *szListenIP, int iPort, int iWorkerThread, bool bEnableNagle, BYTE byPacketCode, BYTE byPacketKey1, BYTE byPacketKey2);
	bool Stop();

	//	외부에서 세션객체 연결
	void SetSessionArray(int iArrayIndex, CNetSession *pSession);

	//	내부 스레드 생성
	bool CreateThread();

	//	Socket IOCP 등록
	bool CreateIOCP_Socket(SOCKET Socket, ULONG_PTR Key);

	//	패킷 보내기, 전체세션 대상
	void SendPacket_GameAll(CPacket *pPacket, unsigned __int64 ExcludeClientID = 0);

	//	패킷 보내기, 특정 클라이언트
	void SendPacket(CPacket *pPacket, unsigned __int64 ClientID);

	bool SessionAcquireLock(int Index);
	bool SessionAcquireFree(int Index);
private:
	void Error(int ErrorCode, WCHAR *szFormatStr, ...);
	void RecvPost(int Index);
	void SendPost(int Index);
	void CompleteRecv(int Index, DWORD Trans);
	void CompleteSend(int Index, DWORD Trans);

	//	Auth, Game 스레드의 처리함수
	void ProcAuth_Accept();
	void ProcAuth_LogoutInAuth();
	void ProcAuth_Logout();
	void ProcAuth_AuthToGame();

	void ProcGame_AuthToGame();
	void ProcGame_Logout();
	void ProcGame_Release();

	//	스레드 함수
	static unsigned __stdcall	AcceptThread(void *pParam);
	bool				AcceptThread_update();

	static unsigned __stdcall	AuthThread(void *pParam);
	bool				AuthThread_update();

	static unsigned __stdcall	GameUpdateThread(void *pParam);
	bool				GameUpdateThread_update();


	static unsigned __stdcall	IOCPWorkerThread(void *pParam);
	bool				IOCPWorkerThread_update();

	static unsigned __stdcall	SendThread(void *pParam);
	bool				SendThread_update();

private:
	virtual void OnConnectionRequest() = 0;
	
	//	AUTH 모드 업데이트 이벤트 로직처리부
	virtual void OnAuth_Update() = 0;
	
	//	GAME 모드 업데이트 이벤트 로직처리부
	virtual void OnGame_Update() = 0;

	virtual void OnError(int iErrorCode, WCHAR *szError) = 0;

public:
	const int _iMaxSession;

private:
	bool _bShutdown;
	bool _bShutdownListen;

	SOCKET _ListenSocket;

	BYTE _byCode;
	bool _bEnableNagle;
	int _iWorkerThread;

	WCHAR _szListenIP[16];
	int _iListenPort;
	unsigned __int64 _iClientIDCnt;

	HANDLE _hAcceptThread;

	CLockFreeQueue<CLIENT_CONNECT_INFO *>	_AccpetSocketQueue;	//	신규접속 Socket 큐
	CMemoryPool<CLIENT_CONNECT_INFO>		*_pMemoryPool_ConnectInfo;

	//	Auth 부
	HANDLE	_hAuthThread;

	CLockFreeStack<int>	_BlankSessionStack;		//	빈 세션 index

	//	GameUpdate 부
	HANDLE	_hGameUpdateThread;

	//	IOCP 부
	HANDLE	_hIOCPWorkerThread[WORKER_THREAD_MAX];
	HANDLE	_hIOCP;

	//	Send 부
	HANDLE	_hSendThread;

	CNetSession	**_pSessionArray;
	SRWLOCK		_Srwlock;
public:
	long long	_Monitor_AcceptTotal;			//	Accept 총 횟수
	long		_Monitor_AcceptSocket;			//	1초 당 Accept 횟수
	long		_Monitor_SessionAllMode;		//	전체 접속자 수
	long		_Monitor_SessionAuthMode;		//	AuthMode 접속자 수
	long		_Monitor_SessionGameMode;		//	GameMode 접속자 수

	long		_Monitor_Counter_AuthUpdate;	//	1초 당 AuthThread 루프 횟수
	long		_Monitor_Counter_GameUpdate;	//	1초 당 GameThread 루프 횟수
	long		_Monitor_Counter_Recv;			//	1초 당 Recv 루프 횟수
	long		_Monitor_Counter_Send;			//	1초 당 Send 루프 횟수
	long		_Monitor_Counter_PacketSend;	//	1초 당 SendThread 루프 횟수

	CSystemLog	*_pLog;
};

#endif
