#include "object.hpp"


bool Callable::get(data::Scope** val) const
{
	(*val) = _value;
	return true;
}


