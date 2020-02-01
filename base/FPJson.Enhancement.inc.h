/*================================================================================

								Notice

	This file is a inc file, which is an enhancement of class fpnn::Json in FPJson.h.

================================================================================*/


/*
	1. 因为可变参数的 push(...) 和 push(path, ...) 会相互混淆，并先于其他重载函数被适配，
		所以改名为：
		fill(...)
		fillTo(...)
*/

//====================================================================//
//--            Advanced set functions
//====================================================================//

private:
	void pushMemberToArray() {}

	template<typename T, typename... Args>
	void pushMemberToArray(T member, Args... args)
	{
		push(member);
		pushMemberToArray(args...);
	}

	template<typename Tuple, size_t N>
	struct PushTupleToArray
	{
		static void pushMember(Json* json, const Tuple& tup)
		{
			PushTupleToArray<Tuple, N-1>::pushMember(json, tup);
			json->push(std::get<N-1>(tup)); 
		}
	};

	template<typename Tuple>
	struct PushTupleToArray<Tuple, 1>
	{
		static void pushMember(Json* json, const Tuple& tup)
		{
			json->push(std::get<0>(tup)); 
		}
	};

public:
	template<typename... Args>
	void setArray(Args... args)
	{
		setArray();
		pushMemberToArray(args...);
	}

	template<class... Args>
	void setArray(const std::tuple<Args...>& tup)
	{
		setArray();
		PushTupleToArray<decltype(tup), sizeof...(Args)>::pushMember(this, tup);
	}

	template < class T, size_t N >
	void setArray(const std::array<T, N>& arr)
	{
		setArray();
		for (auto& data: arr)
			push(data);
	}

	template < class T, class Alloc = std::allocator<T> >
	void setArray(const std::deque<T, Alloc>& deq)
	{
		setArray();
		for (auto& data: deq)
			push(data);
	}

	// template < class T, class Alloc = allocator<T> > class list;
	// template < class T, class Alloc = allocator<T> > class vector;
	template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
	void setArray(const Container<T, Alloc>& container)
	{
		setArray();
		for (auto& data: container)
			push(data);
	}
	template <class Alloc = std::allocator<bool>>
	void setArray(const std::vector<bool, Alloc>& container)
	{
		setArray();
		for (size_t i = 0; i < container.size(); i++)
			push(container.at(i));
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	void setArray(const std::set<T, Compare, Alloc>& collections)
	{
		setArray();
		for (auto& data: collections)
			push(data);
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	void setArray(const std::unordered_set<Key, Hash, Pred, Alloc>& collections)
	{
		setArray();
		for (auto& data: collections)
			push(data);
	}

	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	void setDict(const std::map<std::string, T, Compare, Alloc>& theMap)
	{
		setDict();
		for (auto& dataPair: theMap)
			addBasicValue(dataPair.first, dataPair.second);
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	void setDict(const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap)
	{
		setDict();
		for (auto& dataPair: theMap)
			addBasicValue(dataPair.first, dataPair.second);
	}

//====================================================================//
//--            Advanced Assign functions
//====================================================================//
public:
	template<class... Args>
	Json& operator = (const std::tuple<Args...>& tup)
	{
		setArray(tup);
		return *this;
	}

	template < class T, size_t N >
	Json& operator = (const std::array<T, N>& arr)
	{
		setArray(arr);
		return *this;
	}

	template < class T, class Alloc = std::allocator<T> >
	Json& operator = (const std::deque<T, Alloc>& deq)
	{
		setArray(deq);
		return *this;
	}

	// template < class T, class Alloc = allocator<T> > class list;
	// template < class T, class Alloc = allocator<T> > class vector;
	template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
	Json& operator = (const Container<T, Alloc>& container)
	{
		setArray(container);
		return *this;
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	Json& operator = (const std::set<T, Compare, Alloc>& collections)
	{
		setArray(collections);
		return *this;
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	Json& operator = (const std::unordered_set<Key, Hash, Pred, Alloc>& collections)
	{
		setArray(collections);
		return *this;
	}

	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	Json& operator = (const std::map<std::string, T, Compare, Alloc>& theMap)
	{
		setDict(theMap);
		return *this;
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	Json& operator = (const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap)
	{
		setDict(theMap);
		return *this;
	}

//====================================================================//
//--            Special push & merge functions
//====================================================================//
private:
	bool canBeArray()
	{
		if (_type == JSON_Array) return true;
		if (_type == JSON_Uninit)
		{
			setArray();
			return true;
		}
		return false;
	}

	bool canBeObject()
	{
		if (_type == JSON_Object) return true;
		if (_type == JSON_Uninit)
		{
			setDict();
			return true;
		}
		return false;
	}

public:
	template<typename... Args>
	bool fill(Args... args)
	{
		bool res = canBeArray();
		if (res)
		{
			pushMemberToArray(args...);
		}
		return res;
	}

	//-- ** The following functions will throw exception: FpnnJsonNodeTypeMissMatchError **
	template<typename... Args>
	void fillTo(const std::string& path, Args... args)
	{
		JsonPtr node = createNode(path, "./", true);
		if (node->fill(args...))
			return;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

//====================================================================//
//--            Advanced push functions
//====================================================================//
public:
	template<class... Args>
	bool push(const std::tuple<Args...>& tup)
	{
		bool res = canBeArray();
		if (res)
		{
			JsonPtr node(new Json());
			node->setArray(tup);
			pushNode(node);
		}
		return res;
	}

	template < class T, size_t N >
	bool push(const std::array<T, N>& arr)
	{
		bool res = canBeArray();
		if (res)
		{
			JsonPtr node(new Json());
			node->setArray(arr);
			pushNode(node);
		}
		return res;
	}

	template < class T, class Alloc = std::allocator<T> >
	bool push(const std::deque<T, Alloc>& deq)
	{
		bool res = canBeArray();
		if (res)
		{
			JsonPtr node(new Json());
			node->setArray(deq);
			pushNode(node);
		}
		return res;
	}

	// template < class T, class Alloc = allocator<T> > class list;
	// template < class T, class Alloc = allocator<T> > class vector;
	template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
	bool push(const Container<T, Alloc>& container)
	{
		bool res = canBeArray();
		if (res)
		{
			JsonPtr node(new Json());
			node->setArray(container);
			pushNode(node);
		}
		return res;
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	bool push(const std::set<T, Compare, Alloc>& collections)
	{
		bool res = canBeArray();
		if (res)
		{
			JsonPtr node(new Json());
			node->setArray(collections);
			pushNode(node);
		}
		return res;
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	bool push(const std::unordered_set<Key, Hash, Pred, Alloc>& collections)
	{
		bool res = canBeArray();
		if (res)
		{
			JsonPtr node(new Json());
			node->setArray(collections);
			pushNode(node);
		}
		return res;
	}

	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	bool push(const std::map<std::string, T, Compare, Alloc>& theMap)
	{
		bool res = canBeArray();
		if (res)
		{
			JsonPtr node(new Json());
			node->setDict(theMap);
			pushNode(node);
		}
		return res;
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	bool push(const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap)
	{
		bool res = canBeArray();
		if (res)
		{
			JsonPtr node(new Json());
			node->setDict(theMap);
			pushNode(node);
		}
		return res;
	}

//====================================================================//
//--            Advanced merge functions
//====================================================================//
public:
	template<class... Args>
	bool merge(const std::tuple<Args...>& tup)
	{
		bool res = canBeArray();
		if (res)
		{
			PushTupleToArray<decltype(tup), sizeof...(Args)>::pushMember(this, tup);
		}
		return res;
	}

	template < class T, size_t N >
	bool merge(const std::array<T, N>& arr)
	{
		bool res = canBeArray();
		if (res)
		{
			for (auto& data: arr)
				push(data);
		}
		return res;
	}

	template < class T, class Alloc = std::allocator<T> >
	bool merge(const std::deque<T, Alloc>& deq)
	{
		bool res = canBeArray();
		if (res)
		{
			for (auto& data: deq)
			 push(data);
		}
		return res;
	}

	// template < class T, class Alloc = allocator<T> > class list;
	// template < class T, class Alloc = allocator<T> > class vector;
	template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
	bool merge(const Container<T, Alloc>& container)
	{
		bool res = canBeArray();
		if (res)
		{
			for (auto& data: container)
				push(data);
		}
		return res;
	}
	template <class Alloc = std::allocator<bool>>
	bool merge(const std::vector<bool, Alloc>& container)
	{
		bool res = canBeArray();
		if (res)
		{
			for (size_t i = 0; i < container.size(); i++)
				push(container.at(i));
		}
		return res;
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	bool merge(const std::set<T, Compare, Alloc>& collections)
	{
		bool res = canBeArray();
		if (res)
		{
			for (auto& data: collections)
				push(data);
		}
		return res;
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	bool merge(const std::unordered_set<Key, Hash, Pred, Alloc>& collections)
	{
		bool res = canBeArray();
		if (res)
		{
			for (auto& data: collections)
				push(data);
		}
		return res;
	}

	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	bool merge(const std::map<std::string, T, Compare, Alloc>& theMap)
	{
		bool res = canBeObject();
		if (!res) return false;
		
		for (auto& dataPair: theMap)
		{
			if (addBasicValue(dataPair.first, dataPair.second) == false)
				return false;
		}
		return true;
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	bool merge(const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap)
	{
		bool res = canBeObject();
		if (!res) return false;
		for (auto& dataPair: theMap)
		{
			if (addBasicValue(dataPair.first, dataPair.second) == false)
				return false;
		}
		return true;
	}


//====================================================================//
//--            Pathlized Advanced push functions
//-- ** The following functions will throw exception: FpnnJsonNodeTypeMissMatchError **
//====================================================================//
public:
	template<class... Args>
	bool push(const std::string& path, const std::tuple<Args...>& tup, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->push(tup))
			return true;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template < class T, size_t N >
	bool push(const std::string& path, const std::array<T, N>& arr, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->push(arr))
			return true;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template < class T, class Alloc = std::allocator<T> >
	bool push(const std::string& path, const std::deque<T, Alloc>& deq, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->push(deq))
			return true;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	// template < class T, class Alloc = allocator<T> > class list;
	// template < class T, class Alloc = allocator<T> > class vector;
	template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
	bool push(const std::string& path, const Container<T, Alloc>& container, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->push(container))
			return true;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	bool push(const std::string& path, const std::set<T, Compare, Alloc>& collections, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->push(collections))
			return true;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	bool push(const std::string& path, const std::unordered_set<Key, Hash, Pred, Alloc>& collections, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->push(collections))
			return true;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	bool push(const std::string& path, const std::map<std::string, T, Compare, Alloc>& theMap, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->push(theMap))
			return true;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	bool push(const std::string& path, const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->push(theMap))
			return true;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

//====================================================================//
//--            Pathlized Advanced add functions
//-- ** The following functions will throw exception: FpnnJsonNodeTypeMissMatchError & FpnnJosnNodeExistError **
//====================================================================//
public:
	template<class... Args>
	bool add(const std::string& path, const std::tuple<Args...>& tup, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->type() == JSON_Uninit)
		{
			node->setArray(tup);
			return true;
		}
		else
			throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
	}

	template < class T, size_t N >
	bool add(const std::string& path, const std::array<T, N>& arr, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->type() == JSON_Uninit)
		{
			node->setArray(arr);
			return true;
		}
		else
			throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
	}

	template < class T, class Alloc = std::allocator<T> >
	bool add(const std::string& path, const std::deque<T, Alloc>& deq, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->type() == JSON_Uninit)
		{
			node->setArray(deq);
			return true;
		}
		else
			throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
	}

	// template < class T, class Alloc = allocator<T> > class list;
	// template < class T, class Alloc = allocator<T> > class vector;
	template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
	bool add(const std::string& path, const Container<T, Alloc>& container, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->type() == JSON_Uninit)
		{
			node->setArray(container);
			return true;
		}
		else
			throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	bool add(const std::string& path, const std::set<T, Compare, Alloc>& collections, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->type() == JSON_Uninit)
		{
			node->setArray(collections);
			return true;
		}
		else
			throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	bool add(const std::string& path, const std::unordered_set<Key, Hash, Pred, Alloc>& collections, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->type() == JSON_Uninit)
		{
			node->setArray(collections);
			return true;
		}
		else
			throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
	}

	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	bool add(const std::string& path, const std::map<std::string, T, Compare, Alloc>& theMap, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->type() == JSON_Uninit)
		{
			node->setDict(theMap);
			return true;
		}
		else
			throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	bool add(const std::string& path, const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->type() == JSON_Uninit)
		{
			node->setDict(theMap);
			return true;
		}
		else
			throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
	}

//====================================================================//
//--            Pathlized Advanced merge functions
//-- ** The following functions will throw exception: FpnnJsonNodeTypeMissMatchError **
//====================================================================//
public:
	template<class... Args>
	void merge(const std::string& path, const std::tuple<Args...>& tup, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->merge(tup))
			return;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template < class T, size_t N >
	void merge(const std::string& path, const std::array<T, N>& arr, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->merge(arr))
			return;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template < class T, class Alloc = std::allocator<T> >
	void merge(const std::string& path, const std::deque<T, Alloc>& deq, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->merge(deq))
			return;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	// template < class T, class Alloc = allocator<T> > class list;
	// template < class T, class Alloc = allocator<T> > class vector;
	template <template <typename, typename> class Container, typename T, class Alloc = std::allocator<T>>
	void merge(const std::string& path, const Container<T, Alloc>& container, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->merge(container))
			return;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	void merge(const std::string& path, const std::set<T, Compare, Alloc>& collections, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->merge(collections))
			return;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	void merge(const std::string& path, const std::unordered_set<Key, Hash, Pred, Alloc>& collections, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->merge(collections))
			return;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	void merge(const std::string& path, const std::map<std::string, T, Compare, Alloc>& theMap, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->merge(theMap))
			return;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	void merge(const std::string& path, const std::unordered_map<std::string, T, Hash, Pred, Alloc>& theMap, const std::string& delim = "./")
	{
		JsonPtr node = createNode(path, delim, true);
		if (node->merge(theMap))
			return;

		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
	}

//====================================================================//
//--            Advanced Want functions
//-- ** The following functions will throw exception: FpnnJsonNodeTypeMissMatchError **
//====================================================================//
private:
	template<size_t N, class... Args>
	struct WantTupleValues
	{
		static void wantValue(std::list<JsonPtr>::iterator it, size_t targetOffset, std::tuple<Args...>& tup)
		{
			if (N > targetOffset)
			{
				WantTupleValues<N-1, Args...>::wantValue(it, targetOffset, tup);
			}
			else
			{
				auto realIt = it;
				for (size_t i = 0; i < targetOffset - 1; i++)
					realIt++;

				JsonPtr node = *(realIt);

				typename std::tuple_element<N-1, std::tuple<Args...>>::type v;
				std::get<N-1>(tup) = (decltype(v))(*node);

				WantTupleValues<N-1, Args...>::wantValue(it, targetOffset - 1, tup);
			}
		}
	};

	template<class... Args>
	struct WantTupleValues<1, Args...>
	{
		static void wantValue(std::list<JsonPtr>::iterator it, size_t targetOffset, std::tuple<Args...>& tup)
		{
			if (1 == targetOffset)
			{
				JsonPtr node = *it;

				typename std::tuple_element<0, std::tuple<Args...>>::type v;
				std::get<0>(tup) = (decltype(v))(*node);
			}
		}
	};

public:
	template<class... Args>
	std::tuple<Args...> wantTuple(std::tuple<Args...>& tup, bool compatibleMode = false)
	{
		if (_type != JSON_Array)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
		size_t unitCount = data->size();

		if (!compatibleMode && unitCount != sizeof...(Args))
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		if (unitCount > sizeof...(Args))
			unitCount = sizeof...(Args);

		WantTupleValues<sizeof...(Args), Args...>::wantValue(data->begin(), unitCount, tup);
		return tup;
	}

	template < class T, size_t N >
	std::array<T, N> wantArray(bool compatibleMode = false)
	{
		if (_type != JSON_Array)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;

		if (!compatibleMode && data->size() != N)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::array<T, N> res;

		size_t idx = 0;
		for (auto& node: *data)
		{
			if (idx == N)
				return res;

			res[idx] = (T)(*node);
			idx += 1;
		}

		return res;
	}

	template <class T, class Container = std::deque<T> >
	std::queue<T, Container> wantQueue()
	{
		if (_type != JSON_Array)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
		std::queue<T, Container> res;

		for (auto& node: *data)
			res.push((T)(*node));

		return res;
	}

	template < class T, class Alloc = std::allocator<T> >
	std::deque<T, Alloc> wantDeque()
	{
		if (_type != JSON_Array)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
		std::deque<T, Alloc> res;

		for (auto& node: *data)
			res.push_back((T)(*node));

		return res;
	}

	template < class T, class Alloc = std::allocator<T> >
	std::list<T, Alloc> wantList()
	{
		if (_type != JSON_Array)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
		std::list<T, Alloc> res;

		for (auto& node: *data)
			res.push_back((T)(*node));

		return res;
	}

	template < class T, class Alloc = std::allocator<T> >
	std::vector<T, Alloc> wantVector()
	{
		if (_type != JSON_Array)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
		std::vector<T, Alloc> res;
		res.reserve(data->size());

		for (auto& node: *data)
			res.push_back((T)(*node));

		return res;
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	std::set<T, Compare, Alloc> wantSet()
	{
		if (_type != JSON_Array)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
		std::set<T, Compare, Alloc> res;

		for (auto& node: *data)
			res.insert((T)(*node));

		return res;
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	std::unordered_set<Key, Hash, Pred, Alloc> wantUnorderedSet()
	{
		if (_type != JSON_Array)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
		std::unordered_set<Key, Hash, Pred, Alloc> res;

		for (auto& node: *data)
			res.insert((Key)(*node));

		return res;
	}
	
	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	std::map<std::string, T, Compare, Alloc> wantDict()
	{
		if (_type != JSON_Object)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
		std::map<std::string, T, Compare, Alloc> res;

		for (auto& npair: *data)
			res[npair.first] = (T)(*(npair.second));

		return res;
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	std::unordered_map<std::string, T, Hash, Pred, Alloc> wantUnorderedDict()
	{
		if (_type != JSON_Object)
			throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

		std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
		std::unordered_map<std::string, T, Hash, Pred, Alloc> res;

		for (auto& npair: *data)
			res[npair.first] = (T)(*(npair.second));

		return res;
	}

public:
	template<class... Args>
	std::tuple<Args...> wantTuple(const std::string& path, std::tuple<Args...>& tup, bool compatibleMode = false, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantTuple<Args...>(tup, compatibleMode);
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}

	template < class T, size_t N >
	std::array<T, N> wantArray(const std::string& path, bool compatibleMode = false, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantArray<T, N>(compatibleMode);
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}

	template <class T, class Container = std::deque<T> >
	std::queue<T, Container> wantQueue(const std::string& path, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantQueue<T, Container>();
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}

	template < class T, class Alloc = std::allocator<T> >
	std::deque<T, Alloc> wantDeque(const std::string& path, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantDeque<T, Alloc>();
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}

	template < class T, class Alloc = std::allocator<T> >
	std::list<T, Alloc> wantList(const std::string& path, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantList<T, Alloc>();
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}

	template < class T, class Alloc = std::allocator<T> >
	std::vector<T, Alloc> wantVector(const std::string& path, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantVector<T, Alloc>();
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	std::set<T, Compare, Alloc> wantSet(const std::string& path, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantSet<T, Compare, Alloc>();
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	std::unordered_set<Key, Hash, Pred, Alloc> wantUnorderedSet(const std::string& path, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantUnorderedSet<Key, Hash, Pred, Alloc>();
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}
	
	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	std::map<std::string, T, Compare, Alloc> wantDict(const std::string& path, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantDict<T, Compare, Alloc>();
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	std::unordered_map<std::string, T, Hash, Pred, Alloc> wantUnorderedDict(const std::string& path, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->wantUnorderedDict<T, Hash, Pred, Alloc>();
		
		throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
	}
