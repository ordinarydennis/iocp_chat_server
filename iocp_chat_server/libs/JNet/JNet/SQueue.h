#pragma once
#include <Windows.h>
#include "Logger.h"

namespace JNet
{
	template <typename T>
	class SQueue
	{
	public:
		SQueue()
		{
			mRecvPacketSListHead = (PSLIST_HEADER)_aligned_malloc(sizeof(SLIST_HEADER), MEMORY_ALLOCATION_ALIGNMENT);

			if (NULL == mRecvPacketSListHead)
			{
				JCommon::Logger::Error("mRecvPacketSListHead Memeory Allocate Fail _aligned_malloc");
				return;
			}
			
			InitializeSListHead(mRecvPacketSListHead);
		}
		~SQueue()
		{
			_aligned_free(mRecvPacketSListHead);
		}

		void Push(const T& element)
		{
			if (NULL == mRecvPacketSListHead)
			{
				//log
				return; 
			}

			T* pEntry = (T*)_aligned_malloc(sizeof(T), MEMORY_ALLOCATION_ALIGNMENT);
			if (NULL == pEntry)
			{
				//log
				return;
			}

			*pEntry = element;

			InterlockedPushEntrySList(mRecvPacketSListHead, reinterpret_cast<PSLIST_ENTRY>(pEntry));
		}

		T* GetHeader()
		{
			if (NULL == mRecvPacketSListHead)
			{
				//log
				return nullptr;
			}

			PSLIST_ENTRY header = InterlockedFlushSList(mRecvPacketSListHead);
			PSLIST_ENTRY reversedHeader = nullptr;
			while (nullptr != header)
			{
				PSLIST_ENTRY tmp = reversedHeader;
				reversedHeader = header;
				header = reversedHeader->Next;
				reversedHeader->Next = tmp;
			}
			
			return reinterpret_cast<T*>(reversedHeader);
		}

		static void PopAll(PSLIST_ENTRY header)
		{
			while (nullptr != header)
			{
				PSLIST_ENTRY clear = header;
				header = clear->Next;
				_aligned_free(clear);
			}
		}

	private:
		PSLIST_HEADER				mRecvPacketSListHead;
	};
}


