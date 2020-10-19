#pragma once

#include <basetsd.h>
#include <winsock2.h>

namespace JCommon
{
	const UINT32 MAX_SOCKBUF = 1024;	//��Ŷ ũ��

	enum class ERROR_CODE
	{
		NONE,

		ARGS_INVALID = 1000,
		ARGS_PORT,
		ARGS_MAX_CLINENT_COUNT,
		ARGS_PORT_ROOM_START_INDEX,
		ARGS_PORT_ROOM_MAX_USER_COUNT,

		WSASTARTUP,
		SOCKET_CREATE_LISTEN,
		SOCKET_CREATE,
		IOCP_CREATE,
		SOCKET_BIND,
		SOCKET_LISTEN,
		SOCKET_REGISTER_IOCP,
		SOCKET_ASYNC_ACCEPT,
		REDIS_CONNECT,
	};

	enum class CLIENT_ERROR_CODE : UINT16
	{
		NONE,
		LOGIN_USER_INVALID_PW = 33,
		INVALID_ROOM_NUMBER,
		FULL_ROOM_USER,
	};

	enum class PACKET_ID : UINT16
	{
		DEV_ECHO = 1,

		LOGIN_REQ = 201,
		LOGIN_RES = 202,

		ROOM_ENTER_REQ = 206,
		ROOM_ENTER_RES = 207,
		ROOM_NEW_USER_NTF = 208,
		ROOM_USER_LIST_NTF = 209,

		ROOM_LEAVE_REQ = 215,
		ROOM_LEAVE_RES = 216,
		ROOM_LEAVE_USER_NTF = 217,

		ROOM_CHAT_REQ = 221,
		ROOM_CHAT_RES = 222,
		ROOM_CHAT_NOTIFY = 223,
	};

	UINT64 GetCurTimeSec();
}