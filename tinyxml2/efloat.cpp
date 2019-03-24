#include "efloat.h"
#include "floatparts.h"
#include <limits>
#include <stdexcept>
#include <cmath>
#include <iomanip>
#include <string>


static const Efloat Zero;
static const Efloat one(1.f, 0.f, Efloat::NumberType::PowerOf2);
static const Efloat Mone(-1.f, 0.f,  Efloat::NumberType::PowerOf2);
static constexpr float gamma_1 = Efloat::gamma(1);
Efloat::Efloat():Efloat(0.0f, 0.0f, NumberType::PowerOf2){}
Efloat::Efloat(float vf, NumberType nn): Efloat(vf, 0.0f,  nn) {}
// if the number is exact, no rounding added.  if the number is normal rounding added
// idea is that vf + errf may be off by one eps because the sum is exactly midway
// between two floats.  
// we are going to sum the error to the float to get the bounds. If the sum does not
// cause a power of 2 increase, this error won't change the final float.  If it does,
// then this additional error will make sure the upper and lower bounds are still
// correct.
Efloat::Efloat(float vf, float errf, NumberType nn): v{vf}, err{errf}, n{nn}
#ifndef NDEBUG
    ,ld {v}
#endif
{ 
       
	if (n == NumberType::Normal) 
	{
		err += roundingError();
	}
}
// handles addition: if the sum of the two numbers or if either one is exactly zero,
// no extra error is added. PowerOf2 gets degraded no normal for addition
Efloat  operator+(const Efloat f0, const Efloat f1)
{
	// add error from PowerOf2 numbers
	float errn { ( f0.n == Efloat::PowerOf2) ? Efloat::roundingError(f0.v, 0.f): f0.err};
	errn += (f1.n == Efloat::PowerOf2)? Efloat::roundingError(f1.v, 0.f) : f1.err;
	float val {f0.v + f1.v};
	// this line assures that if val + errn is at the midpoint, it the boundary
	// will be included.
#ifndef NDEBUG
	long double ldv {f0.ld + f1.ld};
	return Efloat(val, ldv, errn,  Efloat::Normal);
#else
	return Efloat(val, errn,  Efloat::Normal);
#endif
}
//	if ( f0.isZero() ) {
//		return f1;
//	}
//	else if (f1.isZero()) {
//		return f0;
//	}
// handles addition: if the sum of the two numbers or if either one is exactly zero,
// no extra error is added.
// check if adding a number and its exact inverse
Efloat&  Efloat::operator+=(const Efloat f1)
{
	if ( n == Efloat::PowerOf2) {
	    err = Efloat::roundingError(v, 0.f);
	}
  	err += (f1.n == Efloat::PowerOf2)? Efloat::roundingError(f1.v, 0.f): f1.err;
  	v  += f1.v;
	// this line assures that if val + errn is at the midpoint, it the boundary
	// will be included.
        err +=Efloat::roundingError();
	// if either one is exact need to add Rounding Error to sum
	n = Efloat::Normal;
#ifndef NDEBUG
  	ld += f1.ld;
#endif
	return *this;
}
Efloat&  Efloat::operator-=(const Efloat f1)
{
	if ( n == Efloat::PowerOf2) {
		err = Efloat::roundingError(v, 0.f);
	}
  	err += (f1.n == Efloat::PowerOf2)? Efloat::roundingError(f1.v, 0.f): f1.err;
  	v -= f1.v;
    err +=Efloat::roundingError();
	n = Efloat::Normal;
#ifndef NDEBUG
  	ld -= f1.ld;
#endif
	return *this;
}
Efloat  operator-(const Efloat f0, const Efloat f1)
{
	// add error from PowerOf2 numbers
	float errn { ( f0.n == Efloat::PowerOf2) ? Efloat::roundingError(f0.v, 0.f): f0.err};
	errn += (f1.n == Efloat::PowerOf2)? Efloat::roundingError(f1.v, 0.f) : f1.err;
	float val {f0.v - f1.v};
#ifndef NDEBUG
	long double ldv {f0.ld - f1.ld};
	return Efloat(val, ldv, errn,  Efloat::Normal);
#else
	return Efloat(val, errn,  Efloat::Normal);
#endif
}
// this is *= but it has no call to reciprocalError and produces a Normal Efloat so 
// will break recursive chain.
Efloat Efloat::timesEquals(const Efloat f1)
{
	// check if the numbers are exact inverses
	err = std::fabs(f1.v) * err + std::fabs(v) * f1.err;
	v *= f1.v;
	err += roundingError();
#ifndef NDEBUG
	ld *= f1.ld;
#endif
	return *this;
}


// check if the error on f0 is from f1 * f1 * err0 = err1.  Done with timesEquals so
// that the error on the error can also be tracked.`
bool Efloat::reciprocalError(const Efloat f0, const Efloat f1) const
{
     // f0 can't be more accurate than its roundingError. 
     //  the absoluteError on f0 is no more accurate than the scaled rounnding error
     Efloat err0( f0.getAbsoluteError(), f0.roundingError(), Efloat::Normal);
     err0.timesEquals(f1);
     err0.timesEquals(f1);
     Efloat err1(f1.getAbsoluteError(), f1.roundingError(),  Efloat::Normal);
     return err1 == err0;
}
// here if one is a power of two it still has no error that term doesn't add
// uncertainty. Power of 2 just changes the exponential so the error scales
Efloat&  Efloat::operator*= (const Efloat f1)
{
	// check if both numbers have different type; if N,P, rounding error needed
	// err is abs(f1.v) * err + abs(v) * f1.erru
	float origerr = err;
        err *= std::abs(f1.v);
	if (f1.n == Efloat::Normal) {
		err += std::abs(v) * f1.err;
		err += f1.err * origerr; 
	}
        n = (n == Efloat::PowerOf2 && 
			  f1.n == Efloat::PowerOf2 ) ? Efloat::PowerOf2 :
			  Efloat::Normal;
	v *= f1.v;
	// unless ans is a power of 2 we still need rounding because the sum of two error
	// terms may be off by one half a eps.
	if (n == Efloat::Normal) {
        	err +=Efloat::roundingError();
	}
#ifndef NDEBUG
	ld *= f1.ld;
#endif
	return *this;
}
// This was an attempt to return an exact 1 if the two numbers are reciprocal.
//  reciprocalError works too.  If the error is large then even if the product is/
//  consistent with one, it is a mistake to return an exact number
	//if ( *this == one) {
	//  	if (reciprocalError(cpy, f1)){
	//	      *this = Efloat(1.0f, 0.f, false, Efloat::PowerOf2);
	//  	}
	//}
	//else if (*this == Mone){
	//  	if (reciprocalError(cpy, f1))   {
	//		*this = Efloat(v, 0.0f, false, Efloat::PowerOf2);
        //  	}
	//}
Efloat  operator*(const Efloat f0, const Efloat f1)
{
	float val {f0.v * f1.v};
	// check if the numbers are exact inverses
	Efloat::NumberType nn { ( f0.n == Efloat::PowerOf2 && 
			  f1.n == Efloat::PowerOf2 ) ? Efloat::PowerOf2 : Efloat::Normal};
	float errn {std::abs(f1.v) * f0.err};
	if ( f1.n == Efloat::Normal) {
		errn += std::abs(f0.v) * f1.err;
		errn += f1.err * f0.err;
	}
#ifndef NDEBUG
	long double ldv {f0.ld * f1.ld};
        Efloat prod(val, ldv, errn,  nn);
#else
        Efloat prod(val, errn,  nn);
#endif
	return prod;
}
	//if ( prod == one){ 
	//    if (f0.reciprocalError(f0, f1)) {
	//	   prod = Efloat(1.0f, 0.f, false, Efloat::PowerOf2);
        //    }
	//}
	//else if ( prod == Mone){ 
        //if (f0.reciprocalError(f0, f1)) {
	//	   prod = Efloat(-1.0f, 0.f, false, Efloat::PowerOf2);
        //    }
	//}
Efloat  operator/(const Efloat f0, const Efloat f1)
{
	if ( f1.v == 0.0f)
	{
		throw std::runtime_error("Divide by zero");
	}
	Efloat::NumberType nn { ( f0.n == Efloat::PowerOf2 && 
			  f1.n == Efloat::PowerOf2 ) ? Efloat::PowerOf2 :
			  Efloat::Normal};
	float invV = 1 / f1.v;
	float invVABS = std::abs(invV);
	float val;
	float errn;
        if (f1.n == Efloat::Normal) {
	        float ratio  {f1.err  * invVABS};
		// do divisions first.
		// 1/(1 - ratio)
		float one_R;
		if  (ratio > 0.999) {
			throw std::runtime_error("Error on Efloat too large in Division");
		}
		else {
			one_R = 1/(1.0f - ratio);
		}
		val = f0.v * invV;
		errn = f0.err;
		// exact formula for the error is invVABS /(1 - ratio) * [ f0 * ratio +
		// f0.err]
	        errn += std::abs(f0.v) * ratio;
	        errn *= one_R;
	}
	else {
		val = f0.v * invV;
		errn = f0.err;
	}
	errn *= invVABS;
#ifndef NDEBUG
	long double ldv{ f0.ld / f1.ld};
	return Efloat(val, ldv, errn,  nn);
#else
	return Efloat(val, errn,  nn);
#endif
}
bool    Efloat::largeError() const
{
    return std::abs(v) * 0.99 < err;
}
Efloat&  Efloat::operator/=(const Efloat f0)
{
	if ( f0.v == 0.0f)
	{
		throw std::runtime_error("Divide by zero");
	}
	n = ( n == Efloat::PowerOf2 && 
			  f0.n == Efloat::PowerOf2 ) ? Efloat::PowerOf2 : Efloat::Normal;
	float invV = 1 / f0.v;
	float invV_abs = std::abs(invV);
	if ( f0.n == Efloat::Normal) {
		float ratio  {f0.err * invV_abs};
		// 1/(1 - ratio)
		float one_R;
		if ( ratio > 0.999) {
	   		throw std::runtime_error("Error on Efloat too Large in Division");
		}
		else {
			one_R = 1.0f/ (1.0f - ratio);
		}
	// exact formula for the error is invVABS /(1 - ratio) * [ f0 * ratio +
	// f0.err]
		float v_abs = std::abs(v);
		v *=  invV;
		err +=  v_abs * ratio;
		err *= one_R;
	}
	else {
		v *= invV;
	}	
	err *= invV_abs;
	if (n == Efloat::Normal) {
		err +=Efloat::roundingError();
	}
#ifndef NDEBUG
	ld /= f0.ld;
#endif
	return *this;
}
//	if (v == f0.v && err == f0.err) {
//	if ( *this == f0) {	
//		v = 1.0f;
//		n = Efloat::PowerOf2;
//		err = 0.0f;
//		return *this;
//	}
//	//if (v == -f0.v && err == f0.err) {
//	if (*this == -f0) {
//		v = -1.0f;
//		n = Efloat::PowerOf2;
//		err = 0.0f;
//		return *this;
//	}
bool    operator==(const Efloat f0, const Efloat f1)
{
    // short cut 
    if (f0.v == f1.v) {
    		return true;
    }
    bool nequal{!(f0 < f1) &&  !(f1 < f0)};	
  /*  float fu {f0.upperBound()};
    float fl {f1.lowerBound()};
    bool nequal{ fu > fl};
    if (nequal) {
        fu = f1.upperBound();
        fl = f0.lowerBound();
        nequal = fu > fl;
    }
    nequal =     nequal && f0.n == f1.n;*/
    return nequal;
}
bool    Efloat::isNeg() const
{
         return std::signbit(v);
}
bool    Efloat::isZero() const
{
	bool iszero(v == 0.f && err == 0.0f);
#ifndef NDEBUG
	iszero == iszero && (ld == 0.0L);
#endif
	return iszero;
}
Efloat  Efloat::abs() const
{
    	Efloat val { *this};
	val.v = std::abs(val.v);
#ifndef NDEBUG
	val.ld = std::abs(val.ld);
#endif
	return val;
}
Efloat  Efloat::sqrt() const
{
    if ( *this < Zero){
        throw std::runtime_error("SQRT  negative");
	}
    else if ( *this == Zero )
	{
        return *this;
	}
	float val { std::sqrt(v)};
	float errn {  err / (2.0f * val)};
	bool addRounding { errn != 0.0f || val * val != v};
	Efloat::NumberType nn {(!addRounding &&  n == NumberType::PowerOf2) ? PowerOf2 :
		             Normal};
#ifndef NDEBUG
	long double lvd {std::sqrt(ld)};
	return Efloat(val, lvd, errn,  nn);
#else
	return Efloat(val, errn,  nn);
#endif
}
Efloat  Efloat::cos() const
{
       
       
	float val { std::cos(v)};
    float errn {  err * std::sin(v)};
	Efloat::NumberType nn {Normal};
#ifndef NDEBUG
	long double lvd {std::cos(ld)};
	return Efloat(val, lvd, errn,  nn);
#else
	return Efloat(val, errn,  nn);
#endif
}
Efloat  Efloat::sin() const
{
       
       
	float val { std::sin(v)};
    float errn {  err * std::cos(v)};
	Efloat::NumberType nn {Normal};
#ifndef NDEBUG
	long double lvd {std::cos(ld)};
	return Efloat(val, lvd, errn,  nn);
#else
	return Efloat(val, errn,  nn);
#endif
}
Efloat::operator float() const
{
	return v;
}

float Efloat::getAbsoluteError() const
{
	return err;
}
float Efloat::upperBound() const
{
//	FloatParts fp(v + err);
//	return fp.nextFloatUp();
        // v + err should be the max
        return v + err;
}
// Less than or equal to the real number
// idea: For normal numbers we added the roundingError in every case
// which adds to the real error. For testing, we want a upper bound but one that
// could be reached by the number, not one artificially large.
// The upperBound is the rounded sum of two numbers and that may be larger than the
// actual sum. Subtracting rounding Error will put this back into the attainable
// range.

float   Efloat::upperRealBound() const
{
    float b{ upperBound()};
	if (n == NumberType::Normal) {
        b -=roundingError();
	}
    return b;
}
float Efloat::lowerBound() const
{
//	FloatParts fp(v - err);
//	return fp.nextFloatDown();
	return v - err;
}
// see comment for upperRealBound -same applies here.
float   Efloat::lowerRealBound() const
{
	float b{ lowerBound()};
	if (n == NumberType::Normal){
        b +=roundingError();
	}
	return b;
}
Efloat::NumberType Efloat::getType() const
{
	return n;
}
float Efloat::roundingError(float val, float error )
{
	return gamma_1 * (std::fabs(val) + error);
}
float Efloat::roundingError() const
{
	return gamma_1 * (std::fabs(v) + err);
}
#ifndef NDEBUG
Efloat::Efloat(float vf, long double ldd, float errf,  NumberType nn ):  v{vf}, err{errf},
	n {nn} ,ld {ldd}
{
	if (nn == Efloat::Normal){
		err += roundingError();
	}
}
float Efloat::getRelativeError() const
{
	if (ld == 0.0)
	{
		std::runtime_error("Divide by zero");
	}
	return std::abs((ld - v)/ld);
}
long double Efloat::preciseValue() const
{
	return ld;
}
#endif
Efloat Efloat::operator-() const
{
       Efloat neg { *this};
       neg.v = -v;
#ifndef NDEBUG
       neg.ld = - ld;
#endif
      return neg;	
}
Efloat Efloat::operator+() const
{
	return *this;
}
// finds number of zeros when both a and b are zero
static Efloat::Quadratic abZero(const Efloat::Coefficients& abc){
	Efloat::RealZeros rz { (std::get<2>(abc) == Zero) ? Efloat::Infinity: Efloat::None};
	return Efloat::Quadratic(rz, std::numeric_limits<float>::infinity(), 
			std::numeric_limits<float>::infinity());
}
static Efloat::Quadratic aZero(const Efloat::Coefficients& abc) {
	if ( std::get<1>(abc) == Zero) {
		return abZero(abc);
	}
	Efloat t { -std::get<2>(abc)/std::get<1>(abc)};
	return Efloat::Quadratic(Efloat::One, t, t);
}
// testi if the efloat is accurate
bool errorAccurate(const Efloat ef, float* relError)
{
#ifndef NDEBUG
    long double diff = std::abs(float(ef) - ef.preciseValue());
    float absError {ef.getAbsoluteError()};
    bool withinBounds {diff <= absError};
	if (withinBounds)
    {
        if  (absError == 0){
            *relError = 0.f;
        }
        else {
                *relError = static_cast<float>(diff/ef.getAbsoluteError());
        }
	}
    return withinBounds;
#else
	return true;
#endif
	
}
bool errorAccurate(const Efloat ef, const float accurate, float* relError)
{
    float diff = std::abs(float(ef) - accurate);
    FloatParts absError { ef.getAbsoluteError()};
    // equals primarily for the 0 error 0 difference case
    bool withinBounds {diff <= absError.getFloat()};
    if (absError.getFloat() == 0) {
             *relError = 0.f;
    }
    else {
           *relError = diff/ef.getAbsoluteError();
    }
    return withinBounds;
	
}

Efloat::Quadratic quadratic(const Efloat::Coefficients& abc)
{

	if (std::get<0>(abc) == Zero) {
		return aZero(abc);
	}
	// std::get<0>(abc) is a, 1 is b, 2 is c
	Efloat radical { std::get<1>(abc) * std::get<1>(abc) 
        - Efloat(4.0f, Efloat::PowerOf2) * std::get<0>(abc) * std::get<2>(abc)};
	if (radical < Zero)
	{
		return Efloat::Quadratic(Efloat::None, Zero, Zero);
	}
	Efloat twoA =  Efloat(2.0f, Efloat::PowerOf2) * std::get<0>(abc);
	Efloat minusB = Efloat(-1.0f, Efloat::PowerOf2) *  std::get<1>(abc);
	if (radical == Zero) {
		Efloat t { minusB/twoA};
		return Efloat::Quadratic(Efloat::One, t, t);
	}
    radical = radical.sqrt();
	Efloat t0{ (minusB - radical)/twoA};
	Efloat t1{ (minusB + radical)/twoA};
	return Efloat::Quadratic(Efloat::Two, t0, t1);
}
bool lessThan(const Efloat a, const Efloat b)
{
	return a.v < b.v;
}
bool operator<(const Efloat a, const Efloat b)
{
          
	float aupper { a.upperBound()};
	float blower { b.lowerBound()};
	return  aupper < blower;
}
bool operator>(const Efloat a, const Efloat b)
{
	return b < a;
}
bool operator<=(const Efloat a, const Efloat b)
{
	return !(b < a);
}
bool operator>=(const Efloat a, const Efloat b)
{
	return !(a < b);
}
// selects efloat with lower lowerBound
Efloat EMin(const Efloat a, const Efloat b)
{
	return (b.lowerBound() < a.lowerBound())? b: a;
}
//selects efloat with higher upperbound.
Efloat EMax(const Efloat a, const Efloat b)
{
	return (b.upperBound() > a.upperBound())? b:a;
}
Efloat Efloat::OneOverEoriginalError() const
{
        Efloat vabs { 1/std::fabs(v)};
        Efloat errorScaled { err * vabs};
	if (err != 0.0f) {
	     errorScaled = errorScaled - gamma(1);
	}
        errorScaled = errorScaled * vabs;
	return errorScaled;
}
std::ostream& operator<<(std::ostream& ostr, const Efloat b)
{
    ostr << std::setw(8) << float(b);
    ostr << std::setw(8)  <<  b.getAbsoluteError() / gamma_1;
#ifndef NDEBUG
	if (b.preciseValue() != 0.0L) {
        ostr << std::setw(8) << b.getRelativeError()/ gamma_1;
	}
	else {
            ostr << std::setw(8) << "Zero";
	}

#endif
	if (b.getType() == Efloat::PowerOf2){
	      ostr  << std::setw(3) << "P2";
	}
	else{
	      ostr <<  std::setw(3) << 'N';
	}
	return ostr;
}



EVector::EVector(const float x, const float y, const float z, const float error) :
x{Efloat(x, error,  Efloat::Normal)}, y{Efloat(y, error, Efloat::Normal)},
z{Efloat(z, error, Efloat::Normal)} {}

EVector::EVector(const Efloat xx, const Efloat yy, const Efloat zz): x{xx}, y{yy},
z{zz} {}
       
const Efloat EVector::operator[](size_t i) const
{
	switch(i) {
	case 0:
		return x;
		break;
	case 1:
		return y;
		break;
	case 2: 
		return z;
		break;
	default:
		throw std::runtime_error(" EVector index out of bounds");
		break;
	}
}

Efloat& EVector::operator[](size_t i)
{
	switch(i) {
	case 0:
		return x;
		break;
	case 1:
		return y;
		break;
	case 2: 
		return z;
		break;
	default:
		throw std::runtime_error(" EVector index out of bounds");
		break;
	}
}

EVector& EVector::operator+=(const EVector& vec1)
{
	x += vec1.x;
	y += vec1.y;
	z += vec1.z;
	return *this;
}
EVector& EVector::operator-=(const EVector& vec1)
{
	x -= vec1.x;
	y -= vec1.y;
	z -= vec1.z;
	return *this;
}
EVector& EVector::operator*=(const EVector& vec1)
{
	x *= vec1.x;
	y *= vec1.y;
	z *= vec1.z;
	return *this;
}
EVector& EVector::operator/=(const EVector& vec1)
{
	x /= vec1.x;
	y /= vec1.y;
	z /= vec1.z;
	return *this;
}
EVector& EVector::operator+=(const Efloat u)
{
	x += u;
	y += u;
	z += u;
	return *this;
}
EVector& EVector::operator-=(const Efloat u)
{
	x -= u;
	y -= u;
	z -= u;
	return *this;
}
EVector& EVector::operator*=(const Efloat u)
{
	x *= u;
	y *= u;
	z *= u;
	return *this;
}
EVector& EVector::operator/=(const Efloat u)
{
	x /= u;
	y /= u;
	z /= u;
	return *this;
}
EVector operator*(const EVector& evec0, const Efloat prod)
{
    
    return EVector{ evec0.x *prod,
			    evec0.y * prod,
			    evec0.z * prod};
}
EVector operator*(const Efloat prod, const EVector& evec0)
{
    return evec0 * prod;
}
EVector operator/(const EVector& evec0, const Efloat div)
{
    
    Efloat invDiv { one/div};
    return evec0 * invDiv;
}
EVector operator+(const EVector& evec0, const EVector& evec1)
{
    return EVector{ evec0.x + evec1.x,
			    evec0.y + evec1.y,
			    evec0.z + evec1.z};
}
EVector operator+(const EVector& evec0, const Efloat u)
{
    return EVector{ evec0.x + u,
			    evec0.y + u,
			    evec0.z + u};
}
EVector operator-(const EVector& evec0)
{
    return EVector{ -evec0.x,
			    -evec0.y,
			    -evec0.z};
}
EVector operator-(const EVector& evec0, const Efloat u)
{
    return EVector{ evec0.x - u,
			    evec0.y - u,
			    evec0.z - u};
}
EVector operator-(const EVector& evec0, const EVector& evec1)
{
    return EVector{ evec0.x - evec1.x,
			    evec0.y - evec1.y,
			    evec0.z - evec1.z};
}
// takes the dot product of two evectors
Efloat   dot(const EVector& evec0, const EVector& evec1)
{
	Efloat dotp1 { evec0.x * evec1.x};
	Efloat dotp2 {  evec0.y * evec1.y};
	Efloat dotp3 { evec0.z * evec1.z};
	return dotp1 + dotp2 + dotp3;
}
bool operator ==(const EVector& evec0, const EVector& evec1)
{
	bool equal { evec0.x == evec1.x};
	equal = equal && evec0.y == evec1.y;
	equal = equal && evec0.z == evec1.z;
	return equal;
}
void Efloat::printSingleEfloat(std::string str, const Efloat ef)
{
	std::cout << "Efloat : "  << str << ' ' << ef << std::endl;
	FloatParts fu(ef.upperBound());
	FloatParts fl(ef.lowerBound());
        FloatParts fpp{FloatParts(float(ef))};
	std::cout << fu << std::endl;
	std::cout  << fpp << std::endl;
	std::cout << fl << std::endl << std::endl;
}

std::ostream& operator<<(std::ostream& ostr, const EVector& evec)
{
    ostr << evec.x << evec.y  << evec.z;
	return ostr;
}
std::ostream& operator<<(std::ostream& ostr, const Efloat::Quadratic& q)
{
	switch(std::get<0>(q)){
		case Efloat::None:
			ostr << "No Zeros: ";
			break;
		case Efloat::One:
			ostr << "One zero: " << std::get<1>(q);
			break;
		case Efloat::Two:
			ostr << "Two: Zero 1: " << std::get<1>(q) << std::endl;
			ostr << "Two: Zero 2: " << std::get<2>(q);
			break;
		case Efloat::Infinity:
            ostr << "Infinitely many (all numbers):  ";
			break;
	}
	return ostr;
}

