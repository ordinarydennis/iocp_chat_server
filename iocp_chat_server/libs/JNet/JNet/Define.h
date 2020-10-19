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

	//WSAOVERLAPPED����ü�� Ȯ�� ���Ѽ� �ʿ��� ������ �� �־���.
	struct stOverlappedEx
	{
		WSAOVERLAPPED	m_wsaOverlapped;			//����ü�� ���� ù��°�� �;� �Ѵ�. ����� �����͸� ���� �� �ִ�.	
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

