#pragma once
#include <iostream>
#include <array>
#include <vector>

template <class T, unsigned long N>
std::istream & readbin(std::istream & is, std::array<T,N> & p)
{
	is.read((char*)&p[0],N*sizeof(T));
	return is;
}

template <class T>
std::istream & readbin(std::istream & is, std::vector<T> & r, int n)
{
	is.read((char*)&r[0],r.size()*sizeof(T));
	return is;
}

namespace glpp
{
template<class T,unsigned long N>
std::ostream & operator<<(std::ostream & os, const std::array<T,N> & p)
{
	os << '[';
	for(int i = 0; i < N; i++)
	{
		if(i > 0)
			os << ',';
		os << p[i];
	}
	os <<']';
	return os;
}

}