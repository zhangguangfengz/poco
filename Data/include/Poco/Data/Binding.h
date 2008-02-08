//
// Binding.h
//
// $Id: //poco/Main/Data/include/Poco/Data/Binding.h#6 $
//
// Library: Data
// Package: DataCore
// Module:  Binding
//
// Definition of the Binding class.
//
// Copyright (c) 2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#ifndef Data_Binding_INCLUDED
#define Data_Binding_INCLUDED


#include "Poco/Data/Data.h"
#include "Poco/Data/AbstractBinding.h"
#include "Poco/Data/DataException.h"
#include "Poco/Data/TypeHandler.h"
#include "Poco/SharedPtr.h"
#include <vector>
#include <list>
#include <deque>
#include <set>
#include <map>
#include <cstddef>


namespace Poco {
namespace Data {


template <class T>
class Binding: public AbstractBinding
	/// Binding maps a value or multiple values (see Binding specializations for STL containers as
	/// well as type handlers) to database column(s). Values to be bound can be either mapped 
	/// directly (by reference) or a copy can be created, depending on the value of the copy argument.
	/// To pass a reference to a variable, it is recommended to pass it to the intermediate
	/// utility function use(), which will create the proper binding. In cases when a reference 
	/// is passed to binding, the storage it refers to must be valid at the statement execution time.
	/// To pass a copy of a variable, constant or string literal, use utility function bind().
	/// Variables can be passed as either copies or references (i.e. using either use() or bind()).
	/// Constants, however, can only be passed as copies. this is best achieved using bind() utility 
	/// function. An attempt to pass a constant by reference shall result in compile-time error.
{
public:
	explicit Binding(T& val,
		const std::string& name = "", 
		Direction direction = PD_IN, 
		bool copy = false): 
		AbstractBinding(name, direction),
		_pVal(copy ? new T(val) : 0),
		_val(copy ? *_pVal : val), 
		_bound(false)
		/// Creates the Binding using the passed reference as bound value.
		/// If copy is true, a copy of the value referred to is created.
	{
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return TypeHandler<T>::size();
	}

	std::size_t numOfRowsHandled() const
	{
		return 1;
	}

	bool canBind() const
	{
		return !_bound;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		TypeHandler<T>::bind(pos, _val, getBinder(), getDirection());
		_bound = true;
	}

	void reset ()
	{
		_bound = false;
		AbstractBinder* pBinder = getBinder();
		poco_check_ptr (pBinder);
		pBinder->reset();
	}

private:
	SharedPtr<T> _pVal;
	const T&     _val;
	bool         _bound;
};


template <>
class Binding<const char*>: public AbstractBinding
	/// Binding const char* specialization wraps char pointer into string.
{
public:
	explicit Binding(const char* pVal, 
		const std::string& name = "",
		Direction direction = PD_IN,
		bool copy = true): 
		AbstractBinding(name, direction), 
		_val(pVal ? pVal : throw NullPointerException() ),
		_bound(false)
		/// Creates the Binding by copying the passed string.
	{
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return 1u;
	}

	std::size_t numOfRowsHandled() const
	{
		return 1u;
	}

	bool canBind() const
	{
		return !_bound;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		TypeHandler<std::string>::bind(pos, _val, getBinder(), getDirection());
		_bound = true;
	}

	void reset ()
	{
		_bound = false;
		AbstractBinder* pBinder = getBinder();
		poco_check_ptr (pBinder);
		pBinder->reset();
	}

private:
	std::string _val;
	bool        _bound;
};


template <class T>
class Binding<std::vector<T> >: public AbstractBinding
	/// Specialization for std::vector.
{
public:
	explicit Binding(std::vector<T>& val, 
		const std::string& name = "", 
		Direction direction = PD_IN,
		bool copy = false): 
		AbstractBinding(name, direction),
		_pVal(copy ? new std::vector<T>(val) : 0),
		_val(copy ? *_pVal : val), 
		_begin(_val.begin()), 
		_end(_val.end())
		/// Creates the Binding.
	{
		if (PD_IN == direction && numOfRowsHandled() == 0)
			throw BindingException("It is illegal to bind to an empty data collection");
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return TypeHandler<T>::size();
	}

	std::size_t numOfRowsHandled() const
	{
		return _val.size();
	}

	bool canBind() const
	{
		return _begin != _end;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		poco_assert_dbg(canBind());
		
		TypeHandler<T>::bind(pos, *_begin, getBinder(), getDirection());
		++_begin;
	}

	void reset()
	{
		_begin = _val.begin();
		_end   = _val.end();
	}

private:
	typedef std::vector<T>                Type;
	typedef SharedPtr<Type>               TypePtr;
	typedef typename Type::const_iterator Iterator;

	TypePtr     _pVal;
	const Type& _val;
	Iterator    _begin;
	Iterator    _end;
};


template <>
class Binding<std::vector<bool> >: public AbstractBinding
	/// Specialization for std::vector<bool>.
	/// This specialization is necessary due to the nature of std::vector<bool>.
	/// For details, see the standard library implementation of std::vector<bool> 
	/// or
	/// S. Meyers: "Effective STL" (Copyright Addison-Wesley 2001),
	/// Item 18: "Avoid using vector<bool>."
	/// 
	/// The workaround employed here is using std::deque<bool> as an
	/// internal replacement container. 
	/// 
	/// IMPORTANT:
	/// Only IN binding is supported.
{
public:
	explicit Binding(const std::vector<bool>& val, 
		const std::string& name = "", 
		Direction direction = PD_IN): 
		AbstractBinding(name, direction), 
		_val(val), 
		_deq(_val.begin(), _val.end()),
		_begin(_deq.begin()), 
		_end(_deq.end())
		/// Creates the Binding.
	{
		if (PD_IN != direction)
			throw BindingException("Only IN direction is legal for std:vector<bool> binding.");

		if (numOfRowsHandled() == 0)
			throw BindingException("It is illegal to bind to an empty data collection");
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return 1u;
	}

	std::size_t numOfRowsHandled() const
	{
		return _val.size();
	}

	bool canBind() const
	{
		return _begin != _end;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		poco_assert_dbg(canBind());
		TypeHandler<bool>::bind(pos, *_begin, getBinder(), getDirection());
		++_begin;

	}

	void reset()
	{
		_begin = _deq.begin();
		_end   = _deq.end();
	}

private:
	const std::vector<bool>&         _val;
	std::deque<bool>                 _deq;
	std::deque<bool>::const_iterator _begin;
	std::deque<bool>::const_iterator _end;
};


template <class T>
class Binding<std::list<T> >: public AbstractBinding
	/// Specialization for std::list.
{
public:
	explicit Binding(std::list<T>& val,
		const std::string& name = "",
		Direction direction = PD_IN,
		bool copy = false): 
		AbstractBinding(name, direction), 
		_pVal(copy ? new std::list<T>(val) : 0),
		_val(copy ? *_pVal : val), 
		_begin(_val.begin()), 
		_end(_val.end())
		/// Creates the Binding.
	{
		if (PD_IN == direction && numOfRowsHandled() == 0)
			throw BindingException("It is illegal to bind to an empty data collection");
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return TypeHandler<T>::size();
	}

	std::size_t numOfRowsHandled() const
	{
		return _val.size();
	}

	bool canBind() const
	{
		return _begin != _end;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		poco_assert_dbg(canBind());
		TypeHandler<T>::bind(pos, *_begin, getBinder(), getDirection());
		++_begin;
	}

	void reset()
	{
		_begin = _val.begin();
		_end   = _val.end();
	}

private:
	typedef std::list<T>                  Type;
	typedef SharedPtr<Type>               TypePtr;
	typedef typename Type::const_iterator Iterator;

	TypePtr     _pVal;
	const Type& _val;
	Iterator    _begin;
	Iterator    _end;
};


template <class T>
class Binding<std::deque<T> >: public AbstractBinding
	/// Specialization for std::deque.
{
public:
	explicit Binding(std::deque<T>& val,
		const std::string& name = "",
		Direction direction = PD_IN,
		bool copy = false): 
		AbstractBinding(name, direction), 
		_pVal(copy ? new std::deque<T>(val) : 0),
		_val(copy ? *_pVal : val), 
		_begin(_val.begin()), 
		_end(_val.end())
		/// Creates the Binding.
	{
		if (PD_IN == direction && numOfRowsHandled() == 0)
			throw BindingException("It is illegal to bind to an empty data collection");
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return TypeHandler<T>::size();
	}

	std::size_t numOfRowsHandled() const
	{
		return _val.size();
	}

	bool canBind() const
	{
		return _begin != _end;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		poco_assert_dbg(canBind());
		TypeHandler<T>::bind(pos, *_begin, getBinder(), getDirection());
		++_begin;
	}

	void reset()
	{
		_begin = _val.begin();
		_end   = _val.end();
	}

private:
	typedef std::deque<T>                 Type;
	typedef SharedPtr<Type>               TypePtr;
	typedef typename Type::const_iterator Iterator;

	TypePtr     _pVal;
	const Type& _val;
	Iterator    _begin;
	Iterator    _end;
};


template <class T>
class Binding<std::set<T> >: public AbstractBinding
	/// Specialization for std::set.
{
public:
	explicit Binding(std::set<T>& val,
		const std::string& name = "",
		Direction direction = PD_IN,
		bool copy = false): 
		AbstractBinding(name, direction), 
		_pVal(copy ? new std::set<T>(val) : 0),
		_val(copy ? *_pVal : val), 
		_begin(_val.begin()), 
		_end(_val.end())
		/// Creates the Binding.
	{
		if (PD_IN == direction && numOfRowsHandled() == 0)
			throw BindingException("It is illegal to bind to an empty data collection");
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return TypeHandler<T>::size();
	}

	std::size_t numOfRowsHandled() const
	{
		return _val.size();
	}

	bool canBind() const
	{
		return _begin != _end;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		poco_assert_dbg(canBind());
		TypeHandler<T>::bind(pos, *_begin, getBinder(), getDirection());
		++_begin;
	}

	void reset()
	{
		_begin = _val.begin();
		_end   = _val.end();
	}

private:
	typedef std::set<T>                   Type;
	typedef SharedPtr<Type>               TypePtr;
	typedef typename Type::const_iterator Iterator;

	TypePtr     _pVal;
	const Type& _val;
	Iterator    _begin;
	Iterator    _end;
};


template <class T>
class Binding<std::multiset<T> >: public AbstractBinding
	/// Specialization for std::multiset.
{
public:
	explicit Binding(std::multiset<T>& val,
		const std::string& name = "",
		Direction direction = PD_IN,
		bool copy = false): 
		AbstractBinding(name, direction), 
		_pVal(copy ? new std::multiset<T>(val) : 0),
		_val(copy ? *_pVal : val), 
		_begin(_val.begin()), 
		_end(_val.end())
		/// Creates the Binding.
	{
		if (PD_IN == direction && numOfRowsHandled() == 0)
			throw BindingException("It is illegal to bind to an empty data collection");
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return TypeHandler<T>::size();
	}

	std::size_t numOfRowsHandled() const
	{
		return _val.size();
	}

	bool canBind() const
	{
		return _begin != _end;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		poco_assert_dbg(canBind());
		TypeHandler<T>::bind(pos, *_begin, getBinder(), getDirection());
		++_begin;
	}

	void reset()
	{
		_begin = _val.begin();
		_end   = _val.end();
	}

private:
	typedef std::multiset<T>              Type;
	typedef SharedPtr<Type>               TypePtr;
	typedef typename Type::const_iterator Iterator;

	TypePtr     _pVal;
	const Type& _val;
	Iterator    _begin;
	Iterator    _end;
};


template <class K, class V>
class Binding<std::map<K, V> >: public AbstractBinding
	/// Specialization for std::map.
{
public:
	explicit Binding(std::map<K, V>& val,
		const std::string& name = "",
		Direction direction = PD_IN,
		bool copy = false): 
		AbstractBinding(name, direction), 
		_pVal(copy ? new std::map<K, V>(val) : 0),
		_val(copy ? *_pVal : val), 
		_begin(_val.begin()), 
		_end(_val.end())
		/// Creates the Binding.
	{
		if (PD_IN == direction && numOfRowsHandled() == 0)
			throw BindingException("It is illegal to bind to an empty data collection");
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return TypeHandler<V>::size();
	}

	std::size_t numOfRowsHandled() const
	{
		return _val.size();
	}

	bool canBind() const
	{
		return _begin != _end;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		poco_assert_dbg(canBind());
		TypeHandler<V>::bind(pos, _begin->second, getBinder(), getDirection());
		++_begin;
	}

	void reset()
	{
		_begin = _val.begin();
		_end   = _val.end();
	}

private:
	typedef std::map<K, V>                Type;
	typedef SharedPtr<Type>               TypePtr;
	typedef typename Type::const_iterator Iterator;

	TypePtr     _pVal;
	const Type& _val;
	Iterator    _begin;
	Iterator    _end;
};


template <class K, class V>
class Binding<std::multimap<K, V> >: public AbstractBinding
	/// Specialization for std::multimap.
{
public:
	explicit Binding(std::multimap<K, V>& val,
		const std::string& name = "",
		Direction direction = PD_IN,
		bool copy = false): 
		AbstractBinding(name, direction), 
		_pVal(copy ? new std::multimap<K, V>(val) : 0),
		_val(copy ? *_pVal : val), 
		_begin(_val.begin()), 
		_end(_val.end())
		/// Creates the Binding.
	{
		if (PD_IN == direction && numOfRowsHandled() == 0)
			throw BindingException("It is illegal to bind to an empty data collection");
	}

	~Binding()
		/// Destroys the Binding.
	{
	}

	std::size_t numOfColumnsHandled() const
	{
		return TypeHandler<V>::size();
	}

	std::size_t numOfRowsHandled() const
	{
		return _val.size();
	}

	bool canBind() const
	{
		return _begin != _end;
	}

	void bind(std::size_t pos)
	{
		poco_assert_dbg(getBinder() != 0);
		poco_assert_dbg(canBind());
		TypeHandler<V>::bind(pos, _begin->second, getBinder(), getDirection());
		++_begin;
	}

	void reset()
	{
		_begin = _val.begin();
		_end   = _val.end();
	}

private:
	typedef std::multimap<K, V>           Type;
	typedef SharedPtr<Type>               TypePtr;
	typedef typename Type::const_iterator Iterator;

	TypePtr     _pVal;
	const Type& _val;
	Iterator    _begin;
	Iterator    _end;
};


template <typename T> 
inline Binding<T>* use(T& t, const std::string& name = "")
	/// Convenience function for a more compact Binding creation.
{
	return new Binding<T>(t, name, AbstractBinding::PD_IN);
}


inline Binding<NullData>* use(const NullData& t, const std::string& name = "")
	/// NullData overload.
{
	return new Binding<NullData>(const_cast<NullData&>(t), name, AbstractBinding::PD_IN);
}


template <typename T> 
inline Binding<T>* in(T& t, const std::string& name = "")
	/// Convenience function for a more compact Binding creation.
{
	return new Binding<T>(t, name, AbstractBinding::PD_IN);
}


inline Binding<NullData>* in(const NullData& t, const std::string& name = "")
	/// NullData overload.
{
	return new Binding<NullData>(const_cast<NullData&>(t), name, AbstractBinding::PD_IN);
}


template <typename T> 
inline Binding<T>* out(T& t)
	/// Convenience function for a more compact Binding creation.
{
	return new Binding<T>(t, "", AbstractBinding::PD_OUT);
}


template <typename T> 
inline Binding<T>* io(T& t)
	/// Convenience function for a more compact Binding creation.
{
	return new Binding<T>(t, "", AbstractBinding::PD_IN_OUT);
}


inline AbstractBindingVec& use(AbstractBindingVec& bv)
	/// Convenience dummy function (for syntax purposes only).
{
	return bv;
}


inline AbstractBindingVec& in(AbstractBindingVec& bv)
	/// Convenience dummy function (for syntax purposes only).
{
	return bv;
}


inline AbstractBindingVec& out(AbstractBindingVec& bv)
	/// Convenience dummy function (for syntax purposes only).
{
	return bv;
}


inline AbstractBindingVec& io(AbstractBindingVec& bv)
	/// Convenience dummy function (for syntax purposes only).
{
	return bv;
}


template <typename T> 
inline Binding<T>* bind(T t, const std::string& name)
	/// Convenience function for a more compact Binding creation.
	/// This funtion differs from use() in its value copy semantics.
{
	return new Binding<T>(t, name, AbstractBinding::PD_IN, true);
}


template <typename T> 
inline Binding<T>* bind(T t)
	/// Convenience function for a more compact Binding creation.
	/// This funtion differs from use() in its value copy semantics.
{
	return bind(t, "");
}


} } // namespace Poco::Data


#endif // Data_Binding_INCLUDED
