#ifndef FPNN_Object_Pool_Interface_H
#define FPNN_Object_Pool_Interface_H

#include <utility>
#include "IMemoryPool.h"
namespace fpnn {
/*===============================================================================
  CLASS & STRUCTURE DEFINITIONS
  =============================================================================== */
template < typename K >
class IObjectPool
{
	public:
		virtual				~IObjectPool() {}

		virtual bool		init(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks) = 0;
		virtual void		release() = 0;

		//-- Just for memory. NO constructor be called, and NO destructor be called.
		virtual K*			gainMemory() = 0;
		virtual void		recycleMemory(K**) = 0;
		virtual void		recycleMemory(K*&) = 0;

		virtual bool		inited() = 0;
		virtual void		status(struct MemoryPoolStatus&) = 0;

		//-- Just for Object. Both constructor and destructor will be called.
		template<typename... Params>
			K* gain(Params&&... values)
			{
				K* memoryBlock = gainMemory();
				if (memoryBlock)
					new(memoryBlock) K(std::forward<Params>(values)...);

				return memoryBlock;
			}
		void recycle(K** obj) { (*obj)->~K(); recycleMemory(obj); }
		void recycle(K*& obj) { obj->~K(); recycleMemory(obj); }
};


template < typename T >
struct ObjectData
{
	T							Data;						//-- Must be the first field. Facilitate for the explicit conversion.
	struct ObjectData<T>		*Pre;
	struct ObjectData<T>		*Next;
	//bool						bOverdraft;					//-- The compilers will align data types on a natural boundary.
	void*						bOverdraft;					//Using pointer type rather than bool type just for memory alignment. NULL <=> false, !NULL <=> true.
};
}
#endif
