#include "floatparts.h"
#include <iomanip>
#include <limits>
#include <cmath>
std::uint32_t constexpr sigMask{ static_cast<uint32_t>(0x007fffff)};
FloatParts::FloatParts(float vin) : vf{ vin },  vp{reinterpret_cast<std::uint32_t*>(&vf) }
{
	//sig = getSignificand();
	//exp = getExp();
	//sgn = getSign();
}
//FloatParts::FloatParts(int intin):vf { *reinterpret_cast<float*>(&intin)},
//vp{reinterpret_cast<std::uint32_t*>(&vf) } {}

std::uint32_t FloatParts::getSign() const
{
	return *vp >> 31;
}
float FloatParts::signFloat() const
{
	if (getSign()) {
		return -1.f;
	}
	else{
		return 1.f;
	}
}
std::uint32_t FloatParts::getExp() const
{
	return (*vp >> 23) & 255;
}
void FloatParts::operator++()
{
	++*vp;
	//sig = getSignificand();
	//exp = getExp();
	//sgn = getSign();
}
void FloatParts::operator--()
{
	--*vp;
	//sig = getSignificand();
	//exp = getExp();
	//sgn = getSign();
}
std::uint32_t FloatParts::getSignificand() const
{
	return sigMask & *vp;
}
float FloatParts::getFloat() const
{
	return bitsToFloat(*getFloatInt());
}
const float& FloatParts::f() const
{
	return vf;
}
std::uint32_t* FloatParts::getFloatInt() const
{
	return vp;
}
//FloatParts::operator  int() const
//{
//	return static_cast<int>(*getFloatInt());
//}
float FloatParts::nextFloatUp() const
{
    if ((std::isinf(vf) && vf > 0) || std::isnan(vf))
	{
		return vf;
	}
	FloatParts vt(vf);
	if (vt.f() == -0.f) {
		vt = FloatParts(0.f);
	}

	if (vt.f() >= 0.f)
	{
		++vt;
	}
	else {
		--vt;
	}
	return vt.getFloat();
}
float FloatParts::nextFloatDown() const
{
    if ((std::isinf(vf) && vf < 0) || std::isnan(vf))
	{
		return vf;
	}
	FloatParts vt(vf);
	if (vt.f() == 0.f) {
		vt = FloatParts(-0.f);
	}

	if (vt.f() > 0.f)
	{
		--vt;
	}
	else {
		++vt;
	}
	//std::cout << vt << std::endl;
	//std::cout << bitsToFloat(*vt.vp) << std::endl;
	return vt.getFloat();
}
std::ostream& operator<<(std::ostream& ostr, const FloatParts fp)
{
	if (fp.getSign()) {
		ostr << '-';
	}
	else {
		ostr << '+';
	}
	ostr << ' ';
	ostr << std::showbase << std::setw(5) << std::hex;
	ostr << fp.getExp() << ' ';
	ostr << std::showbase << std::hex;
	ostr << std::setw(9) << fp.getSignificand() << ' ';
	ostr << std::boolalpha << (fp.getFloat() == fp.f()) << ' ';
	ostr << std::setprecision(9)  << std::setw(13) <<  std::left << fp.getFloat();
        ostr << std::dec;	
	return ostr;
}
float bitsToFloat(std::uint32_t ui)
{
	float v {reinterpret_cast<float&>(ui)};
	return v;
}
bool operator<(const FloatParts a, const FloatParts b)
{
	bool aNeg { static_cast<bool>(a.getSign())};
	bool bNeg { static_cast<bool>(a.getSign())};
	if ( (aNeg && !bNeg) || (!aNeg && bNeg)) {
		return aNeg;
	}
	if (aNeg) {
	      return *a.getFloatInt() > *b.getFloatInt();
	}
	return *a.getFloatInt() < *b.getFloatInt();
}

bool operator>(const FloatParts a, const FloatParts b)
{
	return b < a;
}
bool operator<=(const FloatParts a, const FloatParts b)
{
	return !(b < a);
}
bool operator>=(const FloatParts a, const FloatParts b)
{
	return ! (a < b);
}
bool operator== (const FloatParts a, const FloatParts b)
{
	return !(a < b) && !(b < a);
}
bool operator!=(const FloatParts a, const FloatParts b)
{
	return !(a == b);
}
void  printOne(float val)
{

	FloatParts fp(val);
	std::cout << "Value: " << fp << std::endl;
	float u(fp.nextFloatDown());
	FloatParts N(u);
	std::cout << "Lower: " << N << std::endl;
	u = N.nextFloatDown();
	N = FloatParts(u);
	std::cout << "Lower: " << N << std::endl;
    u = fp.nextFloatUp();
	N = FloatParts(u);
	std::cout << "Upper: " << N << std::endl;
	u = N.nextFloatUp();
	N = FloatParts(u);
	std::cout << "Upper: " << N << std::endl << std::endl;
}

bool testFloatPartsComparison()
{
     FloatParts a(0.f);
     bool success {false}; 
     if ( a < a || a != a) {
     	return success;
     }
     FloatParts b { -1.f};
     if (a <= b || b > a )
     	return success;
    a = FloatParts{0.01f};
    if (a <= b || b > a) 
    	return success;
    a = FloatParts {-0.0001};
    if ( b >= a || (a < b))
    	return success;
    a = FloatParts {0.001f};
    b = FloatParts {0.0101f};
    if ( b < a || a >= b)
    	return success;
    b = FloatParts{1.01f};
    a = FloatParts{ -.01f};
    if ( b< a || a >= b)
      return success;
    return true;
}
void testFloatParts(){
	float inf {std::numeric_limits<float>::infinity()};
	float nan{std::numeric_limits<float>::quiet_NaN()};
	std::cout << " Float and 32 bit same size : "; 
	std::cout << std::boolalpha << (sizeof(float) == sizeof(uint32_t)) << std::endl;
	printOne(1.4e-45);
	printOne(-1.4e-45);
	printOne(0.5f);
	printOne(3.75f);
	printOne(inf);
	printOne(nan);
	printOne(-inf);
}

