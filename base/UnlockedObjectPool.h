#ifndef FPNN_Unlocked_Object_Pool_H
#define FPNN_Unlocked_Object_Pool_H

/*===============================================================================
  INCLUDES AND VARIABLE DEFINITIONS
  =============================================================================== */
#include "IObjectPool.h"

/*===============================================================================
  CLASS & STRUCTURE DEFINITIONS
  =============================================================================== */
namespace fpnn {
template < typename K >
class UnlockedObjectPool: public IObjectPool<K>
{
	private:
		ObjectData<K>*		_recycleLink;
		ObjectData<K>*		_activeLink;
		ObjectData<K>*		_activeOverdraftLink;

		int32_t				_appendCount;			//-- The number of added blocks in each appending operation.
		int32_t				_perfectCount;
		int32_t				_maxCount;

		int32_t				_usedCount;			//-- Not include the Overdraft nodes.
		int32_t				_freeCount;
		int32_t				_overdraftCount;
		int32_t				_totalCount;

		bool				_inited;

	private:
		template <typename T>
			T* GainNode(T** ppRecycleLinkHead, T** ppActiveLinkHead);

		template <typename T>
			void RecycleNode(T** ppRecycleLinkHead, T** ppActiveLinkHead, T** ppNode);

		template <typename T>
			void RecycleOverdraftNode(T** ppActiveOverdraftLinkHead, T** ppNode);

		template <typename T>
			bool AppendNode(T** ppRecycleLinkHead, int32_t needBlocks);

		template <typename T>
			T* AppendOverdraftNode(T** ppActiveOverdraftLinkHead);

		bool Append(int32_t needBlocks, ObjectData<K>** ppOverdraftNode);

		void	ReviseDataRelation(int32_t initCount);
		void	clean();

		bool createPool(int32_t initCount);	//-- Create memory pool.

	public:
		UnlockedObjectPool(): _recycleLink(0), _activeLink(0), _activeOverdraftLink(0),
		_appendCount(10), _perfectCount(200), _maxCount(1000),
		_usedCount(0), _freeCount(0), _overdraftCount(0), _totalCount(0),
		_inited(false)
	{}

		~UnlockedObjectPool()
		{
			clean();
		}

		virtual bool init(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks);
		virtual void release();

		virtual K* gainMemory();											//- Gain the memory block.
		virtual void recycleMemory(K** ppNode);							//- Recycle the memory block.
		virtual void recycleMemory(K* & pNode);							//- Recycle the memory block.

		virtual bool inited() { return _inited; }
		virtual void status(struct MemoryPoolStatus& mps);
};

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
template <typename K>
	template <typename T>
T*  UnlockedObjectPool<K>::GainNode(T** ppRecycleLinkHead, T** ppActiveLinkHead)
{
	T*	pPointer = NULL;
	{
		//std::lock_guard<std::mutex> lck (mutex);
		pPointer = *ppRecycleLinkHead;
		if (!pPointer)
			return NULL;

		*ppRecycleLinkHead = (*ppRecycleLinkHead)->Next;
		if( *ppRecycleLinkHead )
			(*ppRecycleLinkHead)->Pre = NULL;

		//-- Active Link
		pPointer->Pre = NULL;
		pPointer->Next = *ppActiveLinkHead;
		if( *ppActiveLinkHead )
			(*ppActiveLinkHead)->Pre = pPointer;
		*ppActiveLinkHead = pPointer;

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
template <typename K>
	template <typename T>
void UnlockedObjectPool<K>::RecycleNode(T** ppRecycleLinkHead, T** ppActiveLinkHead, T** ppNode)
{
	//((K*)(*ppNode))->~K();  //-- just Object Pool doing.

	//std::lock_guard<std::mutex> lck (mutex);
	{
		//-- Active Link
		if( (*ppNode)->Pre )
			(*ppNode)->Pre->Next = (*ppNode)->Next;
		else
		{
			// *ppNode is the pActiveLinkHead.
			*ppActiveLinkHead = (*ppNode)->Next;
		}
		if( (*ppNode)->Next )
			(*ppNode)->Next->Pre = (*ppNode)->Pre;

		//-- Recycle Link
		(*ppNode)->Pre = NULL;
		(*ppNode)->Next = *ppRecycleLinkHead;
		if( *ppRecycleLinkHead )
			(*ppRecycleLinkHead)->Pre = *ppNode;
		*ppRecycleLinkHead = *ppNode;

		_usedCount -= 1;
		_freeCount += 1;
	}

	*ppNode = NULL;
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
template <typename K>
	template <typename T>
void UnlockedObjectPool<K>::RecycleOverdraftNode(T** ppActiveOverdraftLinkHead, T** ppNode)
{
	//((K*)(*ppNode))->~K();  //-- just Object Pool doing.

	//std::lock_guard<std::mutex> lck (mutex);
	{
		//-- Active Link
		if( (*ppNode)->Pre )
			(*ppNode)->Pre->Next = (*ppNode)->Next;
		else
		{
			// *ppNode is the pActiveOverdraftLinkHead.
			*ppActiveOverdraftLinkHead = (*ppNode)->Next;
		}
		if( (*ppNode)->Next )
			(*ppNode)->Next->Pre = (*ppNode)->Pre;

		_overdraftCount -= 1;
		_totalCount -= 1;

		free(*ppNode);
	}

	*ppNode = NULL;
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
template <typename K>
	template <typename T>
bool UnlockedObjectPool<K>::AppendNode(T** ppRecycleLinkHead, int32_t needBlocks)
{
	T*			pTempHeader = NULL;
	T*			pPointer = NULL;

	int32_t count = 0;
	for( ; count < needBlocks; count++ )
	{
		pPointer = (T*)malloc(sizeof(T));
		if( !pPointer )
			break;

		pPointer->Pre = NULL;
		pPointer->Next = NULL;
		pPointer->bOverdraft = NULL;

		if( pTempHeader )
		{
			pTempHeader->Pre = pPointer;
			pPointer->Next = pTempHeader;
		}
		pTempHeader = pPointer;
	}

	if( !pTempHeader )
		return false;

	{
		//std::lock_guard<std::mutex> lck (mutex);

		_freeCount += count;
		_totalCount += count;

		pPointer = *ppRecycleLinkHead;
		if( *ppRecycleLinkHead )
		{
			while( pPointer->Next )
			{
				pPointer = pPointer->Next;
			}

			pPointer->Next = pTempHeader;
			pTempHeader->Pre = pPointer;
		}
		else
			*ppRecycleLinkHead = pTempHeader;
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
template <typename K>
	template <typename T>
T* UnlockedObjectPool<K>::AppendOverdraftNode( T** ppActiveOverdraftLinkHead )
{
	T*			pPointer = NULL;

	pPointer = (T*)malloc(sizeof(T));

	if( !pPointer )
	{
		return NULL;
	}

	pPointer->Pre = NULL;
	pPointer->bOverdraft = this;

	_overdraftCount += 1;
	_totalCount += 1;

	pPointer->Next = *ppActiveOverdraftLinkHead;
	if( *ppActiveOverdraftLinkHead )
		(*ppActiveOverdraftLinkHead)->Pre = pPointer;
	*ppActiveOverdraftLinkHead = pPointer;

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
	template <typename K>
bool UnlockedObjectPool<K>::Append(int32_t needBlocks, ObjectData<K>** ppOverdraftNode )
{
	{
		//std::lock_guard<std::mutex> lck (_mutex);

		int32_t currentCount = _totalCount - _overdraftCount;
		if (currentCount >= _perfectCount)
		{
			if (_maxCount != 0)
			{
				if (_maxCount == _totalCount)
					return false;
			}

			*ppOverdraftNode = AppendOverdraftNode(&_activeOverdraftLink);

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

	return AppendNode(&_recycleLink, needBlocks);
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
	template <typename K>
void UnlockedObjectPool<K>::ReviseDataRelation(int32_t initCount)
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
	template <typename K>
bool UnlockedObjectPool<K>::init(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks)
{
	if (_inited)
		return true;

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
	template <typename K>
bool UnlockedObjectPool<K>::createPool(int32_t initCount)
{
	//std::lock_guard<std::mutex> lck (_mutex);

	if (_recycleLink || _activeLink)
		clean();

	int32_t count = 0;
	for (; count < initCount; count++)
	{
		ObjectData<K>* node = (ObjectData<K>*)malloc(sizeof(ObjectData<K>));
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
	template <typename K>
void UnlockedObjectPool<K>::clean()
{

	ObjectData<K>*		node = NULL;
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
	template <typename K>
void UnlockedObjectPool<K>::release()
{
	//if (!_inited)
	//	return;

	//std::lock_guard<std::mutex> lck (_mutex);
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
	template <typename K>
K* UnlockedObjectPool<K>::gainMemory()
{
	if (_inited == false)
		return NULL;

	K*	pNode = (K*)GainNode(&_recycleLink, &_activeLink);
	if (pNode)
		return pNode;

	ObjectData<K>*		pMemPoolNode = NULL;
	if (Append(_appendCount, &pMemPoolNode))
	{
		if (pMemPoolNode)
			return (K*)pMemPoolNode;
		else
			return (K*)(GainNode(&_recycleLink, &_activeLink));
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
	template <typename K>
void UnlockedObjectPool<K>::recycleMemory(K** ppNode)
{
	if( !_inited || !ppNode || !(*ppNode) )
		return;

	if( (*((ObjectData<K>**)ppNode))->bOverdraft != NULL )
		RecycleOverdraftNode(&_activeOverdraftLink, (ObjectData<K>**)ppNode );
	else
		RecycleNode(&_recycleLink, &_activeLink, (ObjectData<K>**)ppNode );
}

	template <typename K>
void UnlockedObjectPool<K>::recycleMemory(K* & pNode)
{
	if( !_inited || !pNode )
		return;

	if( ((ObjectData<K>*)pNode)->bOverdraft != NULL )
		RecycleOverdraftNode(&_activeOverdraftLink, (ObjectData<K>**)&pNode );
	else
		RecycleNode(&_recycleLink, &_activeLink, (ObjectData<K>**)&pNode );
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
	template <typename K>
void UnlockedObjectPool<K>::status(MemoryPoolStatus& mps)
{
	if (_inited)
	{
		mps.usedCount = _usedCount;
		mps.freeCount = _freeCount;
		mps.overdraftCount = _overdraftCount;
		mps.totalUsedCount = _usedCount + _overdraftCount;
		mps.totalCount = _totalCount;
	}
}
}
#endif
