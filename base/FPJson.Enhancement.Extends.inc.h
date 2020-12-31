/*================================================================================

								Notice

	This file is a inc file, which is an extend of enhancement of class fpnn::Json in FPJson.h.

================================================================================*/

//====================================================================//
//--            Advanced Get functions
//-- ** The following functions will not throw exception, just return original value or default value **
//====================================================================//

public:
	//-- Parameter tup maybe modified partly.
	template<class... Args>
	std::tuple<Args...> getTuple(std::tuple<Args...>& tup, bool compatibleMode = false)
	{
		try
		{
			return wantTuple<Args...>(tup, compatibleMode);
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return tup;
		}
	}

	template < class T, size_t N >
	std::array<T, N> getArray(const std::array<T, N>& dft = std::array<T, N>(), bool compatibleMode = false)
	{
		try
		{
			return wantArray<T, N>(compatibleMode);
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return dft;
		}
	}

	template <class T, class Container = std::deque<T> >
	std::queue<T, Container> getQueue(const std::queue<T, Container>& dft = std::queue<T, Container>())
	{
		try
		{
			return wantQueue<T, Container>();
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return dft;
		}
	}

	template < class T, class Alloc = std::allocator<T> >
	std::deque<T, Alloc> getDeque(const std::deque<T, Alloc>& dft = std::deque<T, Alloc>())
	{
		try
		{
			return wantDeque<T, Alloc>();
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return dft;
		}
	}

	template < class T, class Alloc = std::allocator<T> >
	std::list<T, Alloc> getList(const std::list<T, Alloc>& dft = std::list<T, Alloc>())
	{
		try
		{
			return wantList<T, Alloc>();
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return dft;
		}
	}

	template < class T, class Alloc = std::allocator<T> >
	std::vector<T, Alloc> getVector(const std::vector<T, Alloc>& dft = std::vector<T, Alloc>())
	{
		try
		{
			return wantVector<T, Alloc>();
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return dft;
		}
	}
	
	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	std::set<T, Compare, Alloc> getSet(const std::set<T, Compare, Alloc>& dft = std::set<T, Compare, Alloc>())
	{
		try
		{
			return wantSet<T, Compare, Alloc>();
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return dft;
		}
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	std::unordered_set<Key, Hash, Pred, Alloc> getUnorderedSet(const std::unordered_set<Key, Hash, Pred, Alloc>& dft = std::unordered_set<Key, Hash, Pred, Alloc>())
	{
		try
		{
			return wantUnorderedSet<Key, Hash, Pred, Alloc>();
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return dft;
		}
	}
	
	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	std::map<std::string, T, Compare, Alloc> getDict(const std::map<std::string, T, Compare, Alloc>& dft = std::map<std::string, T, Compare, Alloc>())
	{
		try
		{
			return wantDict<T, Compare, Alloc>();
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return dft;
		}
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	std::unordered_map<std::string, T, Hash, Pred, Alloc> getUnorderedDict(const std::unordered_map<std::string, T, Hash, Pred, Alloc>& dft = std::unordered_map<std::string, T, Hash, Pred, Alloc>())
	{
		try
		{
			return wantUnorderedDict<T, Hash, Pred, Alloc>();
		}
		catch (const FpnnJsonNodeTypeMissMatchError& ex)
		{
			return dft;
		}
	}

public:
	template<class... Args>
	std::tuple<Args...> getTuple(const std::string& path, std::tuple<Args...>& tup, bool compatibleMode = false, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getTuple<Args...>(tup, compatibleMode);
		
		return tup;
	}

	template < class T, size_t N >
	std::array<T, N> getArray(const std::string& path, const std::array<T, N>& dft = std::array<T, N>(), bool compatibleMode = false, const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getArray<T, N>(dft, compatibleMode);
		
		return dft;
	}

	template <class T, class Container = std::deque<T> >
	std::queue<T, Container> getQueue(const std::string& path, const std::queue<T, Container>& dft = std::queue<T, Container>(), const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getQueue<T, Container>(dft);
		
		return dft;
	}

	template < class T, class Alloc = std::allocator<T> >
	std::deque<T, Alloc> getDeque(const std::string& path, const std::deque<T, Alloc>& dft = std::deque<T, Alloc>(), const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getDeque<T, Alloc>(dft);
		
		return dft;
	}

	template < class T, class Alloc = std::allocator<T> >
	std::list<T, Alloc> getList(const std::string& path, const std::list<T, Alloc>& dft = std::list<T, Alloc>(), const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getList<T, Alloc>(dft);
		
		return dft;
	}

	template < class T, class Alloc = std::allocator<T> >
	std::vector<T, Alloc> getVector(const std::string& path, const std::vector<T, Alloc>& dft = std::vector<T, Alloc>(), const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getVector<T, Alloc>(dft);
		
		return dft;
	}

	template < class T, class Compare = std::less<T>, class Alloc = std::allocator<T> >
	std::set<T, Compare, Alloc> getSet(const std::string& path, const std::set<T, Compare, Alloc>& dft = std::set<T, Compare, Alloc>(), const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getSet<T, Compare, Alloc>(dft);
		
		return dft;
	}

	template < class Key, class Hash = std::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
	std::unordered_set<Key, Hash, Pred, Alloc> getUnorderedSet(const std::string& path, const std::unordered_set<Key, Hash, Pred, Alloc>& dft = std::unordered_set<Key, Hash, Pred, Alloc>(), const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getUnorderedSet<Key, Hash, Pred, Alloc>(dft);
		
		return dft;
	}
	
	template <class T, class Compare = std::less<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	std::map<std::string, T, Compare, Alloc> getDict(const std::string& path, const std::map<std::string, T, Compare, Alloc>& dft = std::map<std::string, T, Compare, Alloc>(), const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getDict<T, Compare, Alloc>(dft);
		
		return dft;
	}

	template <class T, class Hash = std::hash<std::string>, class Pred = std::equal_to<std::string>, class Alloc = std::allocator<std::pair<const std::string, T> > >
	std::unordered_map<std::string, T, Hash, Pred, Alloc> getUnorderedDict(const std::string& path, const std::unordered_map<std::string, T, Hash, Pred, Alloc>& dft = std::unordered_map<std::string, T, Hash, Pred, Alloc>(), const std::string& delim = "./")
	{
		JsonPtr node = getNode(path, delim);
		if (node)
			return node->getUnorderedDict<T, Hash, Pred, Alloc>(dft);
		
		return dft;
	}
