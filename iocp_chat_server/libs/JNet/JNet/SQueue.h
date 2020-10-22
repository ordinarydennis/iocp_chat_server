#pragma once
#include "Logger.h"
#include <Windows.h>

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
				//TODO log
				return; 
			}

			T* pEntry = (T*)_aligned_malloc(sizeof(T), MEMORY_ALLOCATION_ALIGNMENT);
			if (NULL == pEntry)
			{
				//TODO log
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

		T* Front()
		{
			if (nullptr == mReversedHeader)
			{
				PSLIST_ENTRY header = InterlockedFlushSList(mRecvPacketSListHead);
				while (nullptr != header)
				{
					PSLIST_ENTRY tmp = mReversedHeader;
					mReversedHeader = header;
					header = mReversedHeader->Next;
					mReversedHeader->Next = tmp;
				}
			}

			if (nullptr == mReversedHeader)
			{
				return nullptr;
			}

			return reinterpret_cast<T*>(mReversedHeader);
		}

		void Pop()
		{
			if (nullptr == mReversedHeader)
			{
				return;
			}

			auto deleteHeader = mReversedHeader;
			mReversedHeader = mReversedHeader->Next;
			_aligned_free(deleteHeader);
		}

		static void PopAll(PSLIST_ENTRY reversedHeader)
		{
			while (nullptr != reversedHeader)
			{
				PSLIST_ENTRY clear = reversedHeader;
				reversedHeader = clear->Next;
				_aligned_free(clear);
			}
		}

	private:
		PSLIST_HEADER				mRecvPacketSListHead = nullptr;
		PSLIST_ENTRY				mReversedHeader = nullptr;
	};
}


