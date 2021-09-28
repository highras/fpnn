#include "MemoryPool.h"

using namespace fpnn;
/*===============================================================================
FUNCTION DEFINITIONS: General Size Fixed Memory Pool Functions
=============================================================================== */
/*===========================================================================

FUNCTION: TMemoryPool::GainNode

DESCRIPTION:
  Gain the available node from memory buffer.

PARAMETERS:
	Mutex [in] - The mutex belonged to a specified sub pool.
	ppRecycleLinkHead [in] - The pointer point to the head pointer of recycle list belonged to a specified sub pool.
	ppActiveLinkHead [in] - The pointer point to the head pointer of active list belonged to a specified sub pool.

RETURN VALUE:
   T* - Succeed;
   NULL - Failed.
===========================================================================*/
MemoryData*  MemoryPool::GainNode()
{
	MemoryData* pPointer = NULL;
	{
		std::lock_guard<std::mutex> lck (_mutex);
		pPointer = _recycleLink;
		if (!pPointer)
			return NULL;

		_recycleLink = _recycleLink->Next;
		if (_recycleLink)
			_recycleLink->Pre = NULL;

		//-- Active Link
		pPointer->Pre = NULL;
		pPointer->Next = _activeLink;
		if (_activeLink)
			_activeLink->Pre = pPointer;
		_activeLink = pPointer;

		_usedCount += 1;
		_freeCount -= 1;

	}
	
	//new((K*)pPointer) K;  //-- just Object Pool doing.
	return pPointer;
}

/*===========================================================================

FUNCTION: TMemoryPool::RecycleNode

DESCRIPTION:
  Recycle memory block, and hang it over the recycle list. 

PARAMETERS:
	Mutex [in] - The mutex belonged to a specified sub pool.
	ppRecycleLinkHead [in] - The pointer point to the head pointer of recycle list belonged to a specified sub pool.
	ppActiveLinkHead [in] - The pointer point to the head pointer of active list belonged to a specified sub pool.
	ppNode [in] - The block will be recycled.

RETURN VALUE:
   None
===========================================================================*/
void MemoryPool::RecycleNode(MemoryData* memoryData)
{
	//((K*)(*ppNode))->~K();  //-- just Object Pool doing.

	std::lock_guard<std::mutex> lck (_mutex);
	{
		//-- Active Link
		if (memoryData->Pre)
			memoryData->Pre->Next = memoryData->Next;
		else
		{
			// *ppNode is the pActiveLinkHead.
			_activeLink = memoryData->Next;
		}
		if (memoryData->Next)
			memoryData->Next->Pre = memoryData->Pre;

		//-- Recycle Link
		memoryData->Pre = NULL;
		memoryData->Next = _recycleLink;
		if (_recycleLink)
			_recycleLink->Pre = memoryData;
		_recycleLink = memoryData;

		_usedCount -= 1;
		_freeCount += 1;
	}
}

/*===========================================================================

FUNCTION: TMemoryPool::RecycleOverdraftNode

DESCRIPTION:
  Recycle overdraft memory block. 

PARAMETERS:
	Mutex [in] - The mutex belonged to a specified sub pool.
	ppActiveLinkHead [in] - The pointer point to the head pointer of active overdraft list belonged to a specified sub pool.
	ppNode [in] - The block will be recovered.

RETURN VALUE:
   None
===========================================================================*/
void MemoryPool::RecycleOverdraftNode(MemoryData* memoryData)
{
	//((K*)(*ppNode))->~K();  //-- just Object Pool doing.

	std::lock_guard<std::mutex> lck (_mutex);
	{
		//-- Active Link
		if (memoryData->Pre)
			memoryData->Pre->Next = memoryData->Next;
		else
		{
			// *ppNode is the pActiveOverdraftLinkHead.
			_activeOverdraftLink = memoryData->Next;
		}
		if (memoryData->Next)
			memoryData->Next->Pre = memoryData->Pre;

		_overdraftCount -= 1;
		_totalCount -= 1;

		free(memoryData);
	}
}

/*===========================================================================

FUNCTION: TMemoryPool::AppendNode

DESCRIPTION:
  Append new memory blocks in the pool.

PARAMETERS:
	Mutex [in] - The mutex belonged to a specified sub pool.
	ppRecycleLinkHead [in] - The pointer point to the head pointer of recycle list belonged to a specified sub pool.
	ulNumber [in] - The number of memory blocks which will be added.

RETURN VALUE:
   E_SUCCESS - Succeed;
   E_MEMORY_POOL__APPEND_MEMORY - Append Failed;
===========================================================================*/
bool MemoryPool::AppendNode(int32_t needBlocks)
{
	MemoryData*			pTempHeader = NULL;
	MemoryData*			pPointer = NULL;

	int32_t count = 0;
	for (; count < needBlocks; count++)
	{
		pPointer = (MemoryData*)malloc(sizeof(MemoryData) + _blockSize);
		if (!pPointer)
			break;

		pPointer->Pre = NULL;
		pPointer->Next = NULL;
		pPointer->bOverdraft = NULL;

		if (pTempHeader)
		{
			pTempHeader->Pre = pPointer;
			pPointer->Next = pTempHeader;
		}
		pTempHeader = pPointer;
	}

	if (!pTempHeader)
		return false;

	{
		std::lock_guard<std::mutex> lck (_mutex);

		_freeCount += count;
		_totalCount += count;

		pPointer = _recycleLink;
		if (_recycleLink)
		{
			while (pPointer->Next)
			{
				pPointer = pPointer->Next;
			}

			pPointer->Next = pTempHeader;
			pTempHeader->Pre = pPointer;
		}
		else
			_recycleLink = pTempHeader;
	}

	return true;
}

/*===========================================================================

FUNCTION: TMemoryPool::AppendOverdraftNode

DESCRIPTION:
  Append the overdraft memory block in the pool.

PARAMETERS:
	ppActiveOverdraftLinkHead [in] - The pointer point to the head pointer of active overdraft list belonged to a specified sub pool.

RETURN VALUE:
   T* - Succeed;
   NULL - Failed.
===========================================================================*/
MemoryData* MemoryPool::AppendOverdraftNode()
{
	MemoryData* pPointer = NULL;

	pPointer = (MemoryData*)malloc(sizeof(MemoryData) + _blockSize);

	if (!pPointer)
		return NULL;

	pPointer->Pre = NULL;
	pPointer->bOverdraft = this;

	_overdraftCount += 1;
	_totalCount += 1;

	pPointer->Next = _activeOverdraftLink;
	if (_activeOverdraftLink)
		_activeOverdraftLink->Pre = pPointer;
	_activeOverdraftLink = pPointer;

	return pPointer;
}

/*===========================================================================

FUNCTION: TMemoryPool::Append

DESCRIPTION:
  Append new memory blocks in the pool.

PARAMETERS:
	ulNumber [in] - The number of memory blocks which will be added.
	ppOverdraftNode [out] - Output the overdraft block after the memory pool catched the perfect restriction.

RETURN VALUE:
   E_SUCCESS - Succeed;
   E_MEMORY_POOL__APPEND_MEMORY - Append Failed;
   E_MEMORY_POOL__CATCH_LIMITATION - The total number of memory blocks catched the maximum restriction of the pool.
   E_MEMORY_POOL__CATCH_PERFECT_LIMITATION - The number of standard memory blocks (not include the overdraft blocks) catched the perfect restriction of the pool.
===========================================================================*/
bool MemoryPool::Append(int32_t needBlocks, MemoryData** ppOverdraftNode)
{
	{
		std::lock_guard<std::mutex> lck (_mutex);

		int32_t currentCount = _totalCount - _overdraftCount;
		if (currentCount >= _perfectCount)
		{
			if (_maxCount != 0)
			{
				if (_maxCount == _totalCount)
					return false;
			}

			*ppOverdraftNode = AppendOverdraftNode();

			//if( *ppOverdraftNode )
			//	new((K*)(*ppOverdraftNode)) K;	//-- just Object Pool doing.

			return true;
		}
		else
		{
			int32_t	remainCount = _perfectCount - currentCount;
			if( remainCount < needBlocks )
				needBlocks = remainCount;
		}
	}

	return AppendNode(needBlocks);
}

/*===========================================================================

FUNCTION: TMemoryPool::ReviseDataRelation

DESCRIPTION:
  Revise the relation of data which are used to initialize the memory pool.

PARAMETERS:
	ulInitNum [in] - The number for memory block allocation after the pool created.

RETURN VALUE:
   None.
===========================================================================*/
void MemoryPool::ReviseDataRelation(int32_t initCount)
{
	if (_maxCount > 0)
	{
		if(_maxCount < initCount)
			_maxCount = initCount;

		if (_perfectCount > _maxCount)
			_perfectCount = _maxCount;
	}

	
	if (_perfectCount < initCount)
		_perfectCount = initCount;
}

/*===========================================================================

FUNCTION: TMemoryPool::Init

DESCRIPTION:
  Initialize the memory pool.

PARAMETERS:
	ulInitNumber [in] - The number for memory block allocation after the pool created.
	ulMaxNumber [in] - The maximum number of blocks held by memory pool. If don't use the restriction, please let the value is ZERO.
	ulPerIncNumber [in] - The number of memory blocks appended in the appending operation.
	ulPerfectNumber [in] - The soft restriction. If the number of memory blocks beyond this restriction, the pool will return the 
	                       excess memories to the operating system rather than continue holding the blocks.

RETURN VALUE:
   true - Succeed;
   false - Failed.   
===========================================================================*/
bool MemoryPool::init(size_t blockSize, int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks)
{
	if (_inited)
		return true;

	_blockSize = roundedSize(blockSize);
	if (_blockSize < sizeof(void*))
		return false;
		
	_maxCount = maxBlocks;
	_appendCount = perAppendBlocks;
	_perfectCount = perfectBlocks;
	ReviseDataRelation(initBlocks);

	_inited = createPool(initBlocks);
	return _inited;
}

/*===========================================================================

FUNCTION: TMemoryPool::CreatePool

DESCRIPTION:
  Create the memory pool.

PARAMETERS:
	ulInitNumber [in] - The number for memory block allocation after the pool created.

RETURN VALUE:
   true - Succeed;
   false - Failed.
===========================================================================*/
bool MemoryPool::createPool(int32_t initCount)
{
	std::lock_guard<std::mutex> lck (_mutex);

	if (_recycleLink || _activeLink)
		clean();

	int32_t count = 0;
	for (; count < initCount; count++)
	{
		MemoryData* node = (MemoryData*)malloc(sizeof(MemoryData) + _blockSize);
		if (node == NULL)
		{
			if (count == 0)
				return false;
			else
				break;
		}
		node->Pre = NULL;
		node->Next = _recycleLink;
		if (_recycleLink)
			_recycleLink->Pre = node;
		_recycleLink = node;

		node->bOverdraft = NULL;
	}

	_totalCount = count;
	_freeCount = count;

	return true;
}

/*===========================================================================

FUNCTION: TMemoryPool::FreeOriginal

DESCRIPTION:
   Free the memory pool without safeguard.

PARAMETERS:
	None

RETURN VALUE:
   None
===========================================================================*/
void MemoryPool::clean()
{
	MemoryData*		node = NULL;
	while (_recycleLink)
	{
		node = _recycleLink;
		_recycleLink = _recycleLink->Next;
		free(node);
	}
	while (_activeLink)
	{
		node = _activeLink;
		_activeLink = _activeLink->Next;
		//((K*)node)->~K();  //-- just Object Pool doing.
		free(node);
	}
	while (_activeOverdraftLink)
	{
		node = _activeOverdraftLink;
		_activeOverdraftLink = _activeOverdraftLink->Next;
		//((K*)node)->~K();  //-- just Object Pool doing.
		free(node);
	}

	_recycleLink = NULL;
	_activeLink = NULL;
	_activeOverdraftLink = NULL;

	_usedCount = 0;
	_freeCount = 0;
	_overdraftCount = 0;
	_totalCount = 0;

	_inited = false;
}

/*===========================================================================

FUNCTION: TMemoryPool::Free

DESCRIPTION:
   Free the memory pool with safeguard.

PARAMETERS:
	None

RETURN VALUE:
   None
===========================================================================*/
void MemoryPool::release()
{
	if (!_inited)
		return;

	std::lock_guard<std::mutex> lck (_mutex);
	if (!_inited)			//-- Ensure multiple threads safety.
		return;

	clean();
}

/*===========================================================================

FUNCTION: TMemoryPool::Gain

DESCRIPTION:
	Function series: Gain the memory block from buffer.

PARAMETERS:
	None

RETURN VALUE:
   NULL - Failed.
   Others - Succeed.
===========================================================================*/
void* MemoryPool::gain()
{
	if (_inited == false)
		return NULL;

	MemoryData* memoryData = GainNode();
	if (memoryData)
		return getMemoryPtr(memoryData);

	if (Append(_appendCount, &memoryData))
	{
		if (memoryData)
			return getMemoryPtr(memoryData);
		else
		{
			memoryData = GainNode();
			return getMemoryPtr(memoryData);
		}
	}
	else
		return NULL;
}

/*===========================================================================

FUNCTION: TMemoryPool::Recycle

DESCRIPTION:
	Function series: Recycle memory block, and hang it over the recycle list.

PARAMETERS:
	pNode [in] - The block will be recycled.

RETURN VALUE:
   None
===========================================================================*/
void MemoryPool::recycle(void** memory)
{
	if (!_inited || !memory || !(*memory))
		return;
		
	MemoryData* memoryData = getMemoryDataPtr(*memory);

	if (memoryData->bOverdraft != NULL)
		RecycleOverdraftNode(memoryData);
	else
		RecycleNode(memoryData);
		
	*memory = NULL;
}

void MemoryPool::recycle(void* & memory)
{
	if( !_inited || !memory )
		return;
		
	MemoryData* memoryData = getMemoryDataPtr(memory);

	if (memoryData->bOverdraft != NULL)
		RecycleOverdraftNode(memoryData);
	else
		RecycleNode(memoryData);
		
	memory = NULL;
}

/*===========================================================================

FUNCTION: TMemoryPoolLite::GetStatus

DESCRIPTION:
	Get the memory blocks counters.

PARAMETERS:
	strMPS [in] - The memory blocks counters unit.

RETURN VALUE:
   None
===========================================================================*/
void MemoryPool::status(MemoryPoolStatus& mps)
{
	if (_inited)
	{
		std::lock_guard<std::mutex> lck (_mutex);

		mps.usedCount = _usedCount;
		mps.freeCount = _freeCount;
		mps.overdraftCount = _overdraftCount;
		mps.totalUsedCount = _usedCount + _overdraftCount;
		mps.totalCount = _totalCount;
	}
}

