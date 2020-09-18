#pragma once

#include <basetsd.h>

const UINT32 MAX_SOCKBUF = 256;	//패킷 크기
//TODO: 쓰레드 수는 동적으로 가져와야 한다.
const UINT32 MAX_WORKERTHREAD = 4;  //쓰레드 풀에 넣을 쓰레드 수

const UINT16 MAX_CLIENT = 100;
