#pragma once

#include <memory>
#include <optional>
#include <string>

class Node;
class Scope;
class Function;

class Object : public std::enable_shared_from_this<Object>
{
public:
	virtual ~Object() = default;
	virtual bool get(int* val) const { return false; }
	virtual bool get(float* val) const { return false; }
	virtual bool get(bool* val) const { return false; }
	virtual bool get(std::string* val) const { return false; }
	virtual bool get(Scope** val) const { return false; }
	virtual bool get(Function** val) const { return false; }

	template <class T>
	std::optional<T> get_inner() const
	{
		T res;
		if(get(&res))
		{
			return res;
		}
		return {};
	}
};

using ObjectPtr = std::shared_ptr<Object>;

class Integer : public Object
{
public:
	Integer(int v)
		:_value(v)
	{}

	bool get(int* val) const override { (*val) = _value; return true; }

private:
	int _value;
};

class Float : public Object
{
public:
	Float(float v)
		:_value(v)
	{}

	bool get(float* val) const override { (*val) = _value; return true; }

private:
	float _value;
};

class Bool : public Object
{
public:
	Bool(bool v)
		:_value(v)
	{}

	bool get(bool* val) const override { (*val) = _value; return true; }

private:
	bool _value;
};

class String : public Object
{
public:
	String(std::string&& v)
		:_value(std::move(v))
	{}

	bool get(std::string * val) const override { (*val) = _value; return true; }

private:
	std::string _value;
};

class Callable : public Object
{
public:
	Callable(Scope* v)
		:_value(v)
	{}

	bool get(Scope** val) const override;

private:
	Scope* _value;
};