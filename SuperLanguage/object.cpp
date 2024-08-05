#include "object.hpp"


bool Callable::get(Scope** val) const
{
	(*val) = _value;
	return true;
}


