#pragma once
#include <tuple>
#include <iostream>
#include <limits>
//#include <la.h>
#define NDEBUG


class Efloat {

	public:
                // input the float and the error due to the individual components in vf;
                // if vf = v1 +/- v2, errf = err1 + err2.  This error does not include the 
                // floating point inaccuracy at vf gamma(1) * (vf + errf)-- 
		// Number of realZeros -can be 0, 1, 2, infinity
                enum RealZeros {None, One, Two, Infinity};
                // Normal is a number that may or may not have a zero error
                // PowerOf2 is an exact number that is also a power of 2 or zero-- numbers
                // when multiplied or divided by are exact.
		enum NumberType {Normal, PowerOf2};
                //Coefficients are A, B, C 
                typedef std::tuple<Efloat, Efloat, Efloat> Coefficients;
                typedef std::tuple<RealZeros, Efloat, Efloat> Quadratic;
                //Input is vf floating point number, err (0 for exact), Numbertype {normal, PowerOf2}, 
                // bool AddRounding (false -- true adds gamma(1) * (vf + errf)
                Efloat();
		// create an Exact (zero Error, ) floating point of type Normal or PowerOf2
		// not vf, errf = 0, if Number type is PowerOf2 no extra error added;
		// otherwise rounding Error added
		Efloat(float vf, NumberType n);
		Efloat(float vf, float errf = 0.0f, NumberType n = NumberType::Normal);
                friend Efloat  operator+(const Efloat f0, const Efloat f1);
	        friend Efloat  operator-(const Efloat f0, const Efloat f1);
		friend Efloat  operator*(const Efloat f0, const Efloat f1);
		Efloat&        operator+=(const Efloat f0);
		Efloat&        operator-=(const Efloat f0);
		Efloat&        operator*=(const Efloat f0);
		Efloat&        operator/=(const Efloat f0);
		friend Efloat  operator/(const Efloat f0, const Efloat f1);
		friend bool    operator==(const Efloat f0, const Efloat f1);
                friend bool    lessThan(const Efloat a, const Efloat b);
		bool    isNeg() const;
                Efloat  sqrt() const;
		Efloat  cos()  const;
		Efloat  sin()  const;
		Efloat  abs() const;
		Efloat  operator-() const;
		Efloat  operator+() const;
		bool    isZero() const;
		//  Division fails if the error is larger than the divisor,
		//  largeError() is true in this case. 
		bool    largeError() const;
		explicit operator float() const;
		float   getAbsoluteError() const;
		// the highest float that could be the number greater than or equal
		// to the real number
		float   upperBound() const;
		// Less than or equal to the real number
        	float   upperRealBound() const;
		// the lowest float that could be the number; less than the real
		// number
		float   lowerBound() const;
		// greater than or equal to the real number
		float  lowerRealBound() const;

		NumberType getType() const;
                static void  printSingleEfloat(std::string str, const Efloat ef);
                static constexpr float MachineEpsilon {std::numeric_limits<float>::epsilon() * 0.5f};
                static inline constexpr float gamma(int n) {
	                                   return n * MachineEpsilon/(1 - n * MachineEpsilon);
                // lessThan means that  a.v < b.v
		}

#ifndef NDEBUG
		//  used to pass a long double and to keep a precise copy for debugging
	        Efloat(float vf, long double ldd, float errf = 0.0f,
				NumberType n = NumberType::Normal);
		void updatePreciseValue( long double ldd)
		{  
			ld = ldd;
		}
		float getRelativeError() const;
		long double preciseValue() const;
#endif
		float roundingError() const;
	private:
		float v;
		float err;
		NumberType n;
		// calculates the rounding error
#ifndef NDEBUG
		long double ld;
#endif
		// retrieves the error needed to get 1/e
                Efloat OneOverEoriginalError() const;
		// checks if two efloats are reciprocals
		bool reciprocalError(const Efloat f0, const Efloat f1) const;
		// like *= but without PowerOfTwo and no recursion
		Efloat timesEquals(const Efloat f1);
    static	float roundingError( float val, float error);
};
// true if the float is within the bounds of the long;
// returns the difference over the error
bool errorAccurate(const Efloat ef, float* relError);
// do the  error but compare with an external float.
bool errorAccurate(const Efloat ef, const float AccurateAns,  float* relError);
// retrieves the error needed to get 1/e
std::ostream& operator<<(std::ostream&, const Efloat a);
std::ostream& operator<<(std::ostream&, const Efloat::Quadratic& a);
Efloat::Quadratic quadratic(const Efloat::Coefficients& abc);
// operator< means that the upper bound of a is below the 
// lower bound of b.  Equals based on this operator does not
// respect the transitive property. because if a == b and b == c
// a may not equal c.
bool operator<(const Efloat a, const Efloat b);
bool operator<=(const Efloat a, const Efloat b);
bool operator>(const Efloat a, const Efloat b);
bool operator>=(const Efloat a, const Efloat b);
//makes an evector out of a vec3 adds machine precision to all numbers and treats
//them as normal Efloats not exact.  Optionally add an error to all of the numbers.
// these select the Efloat that produces the min lowerbound or the max upperbound.
Efloat EMin(const Efloat a, const Efloat b);
Efloat EMax(const Efloat a, const Efloat b);


// a vector of Efloats;
class EVector{
	public:
	// constructor will transform a vec3 to a EVector adding an error (default = zero)
	// and adding rounding (default is true).
	       // Axis describes which vector component to consider
	       enum Axis { X, Y, Z};
	       // this will be used to convert vec3 to an EVector
			
	       EVector():x{Efloat()}, y{Efloat()}, z{Efloat()} {}
           EVector(const float x, const float y, const float z, const float error = 0.f);
	       EVector(const Efloat x, const Efloat y, const Efloat z);
	       // convert to a vec3
	       const Efloat operator[](size_t i) const;
	       Efloat& operator[](size_t i);
	       EVector& operator+=(const EVector& vec1);
	       EVector& operator-=(const EVector& vec1);
	       EVector& operator*=(const EVector& vec1);
	       EVector& operator/=(const EVector& vec1);
	       EVector& operator+=(const Efloat vec1);
	       EVector& operator-=(const Efloat vec1);
	       EVector& operator*=(const Efloat vec1);
	       EVector& operator/=(const Efloat vec1);
	       Efloat x, y, z;
};
// takes the dot product of two evectors
Efloat   dot(const EVector& evec0, const EVector& evec1);
// product of a glm::mat4 and an EVector to produce an evector.
// test if all components equal
bool operator ==(const EVector& evec0, const EVector& evec1);
EVector operator-(const EVector& evec0, const EVector& evec1);
EVector operator-(const EVector& evec0);
EVector operator-(const EVector& evec0, const Efloat u);
EVector operator+(const EVector& evec0, const EVector& evec1);
EVector operator+(const EVector& evec0, const Efloat u);
EVector operator/(const EVector& evec0, const Efloat div);
EVector operator*(const EVector& evec0, const Efloat prod);
EVector operator*( const Efloat prod, const EVector& evec0);
std::ostream& operator<<(std::ostream&, const EVector& evec);
