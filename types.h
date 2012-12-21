#pragma once

// STL
#include <array>
// Boost
#include <boost/functional/hash.hpp>

// declare the functions in predicates.c
#include "predicates.h"

// Coordinate of a point.
typedef std::array<REAL, 3> xyz;

// The key of a point. (point_k is more meaningful than size_t/int)
//typedef size_t point_k;

typedef int point_k;
// A tetra represented by 4 keys of its points. (tetra is more meaningful than int4.)
typedef std::array<point_k, 4> tetra;

// A triangle represented by 3 keys of its points
typedef std::array<point_k, 3> triangle;

// Corresponding hash class
struct tetra_hash {
	static size_t hash(tetra const& x){
		return boost::hash_value(x);
	}
	static bool equal(tetra const& x, tetra const& y){
		return x[0] == y[0] && x[1] == y[1] && x[2] == y[2] && x[3] == y[3];
	}
	typedef size_t result_type;
	size_t operator()(tetra const& x)const {
		return hash(x);
	}
};

// Corresponding hash class for triangle
struct triangle_hash {
	static size_t hash(triangle const& x){
		return boost::hash_value(x);
	}
	static bool equal(triangle const& x, triangle const& y){
		return x[0] == y[0] && x[1] == y[1] && x[2] == y[2];
	}
	typedef size_t result_type;
	size_t operator()(triangle const& x)const {
		return hash(x);
	}
};
