#pragma once

#include "Common.h"

namespace JNet
{
	const UINT64 RE_USE_SESSION_WAIT_TIMESEC = 3;

	enum class IOOperation
	{
		NONE,
		ACCEPT,
		RECV,
		SEND
	};

	//WSAOVERLAPPED구조체를 확장 시켜서 필요한 정보를 더 넣었다.
	struct stOverlappedEx
	{
		WSAOVERLAPPED	m_wsaOverlapped;			//구조체의 가장 첫번째로 와야 한다. 제대로 데이터를 받을 수 있다.	
		WSABUF			m_wsaBuf;
		IOOperation		m_eOperation;
		UINT32			m_clientId;
		stOverlappedEx()
		{
			ZeroMemory(&m_wsaOverlapped, sizeof(m_wsaOverlapped));
			m_eOperation = IOOperation::NONE;
			m_wsaBuf.buf = nullptr;
			m_wsaBuf.len = 0;
			m_clientId = 0;
		}
	};
}

