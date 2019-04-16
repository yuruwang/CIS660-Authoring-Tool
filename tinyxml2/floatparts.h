#include <cstdint>
#include <iostream>
// this class stores a float and a pointer which to 
// an integer; that pointer allows one to get the next float greater or 
// less than a given float.
class FloatParts
{
public:
	FloatParts(float vf); 
	// the int version  of a float and convert to a float parts
//	FloatParts(int intin);
	std::uint32_t getSign() const;
	// get the sign of the float as a float
	float signFloat() const;
	std::uint32_t getExp() const;
	std::uint32_t getSignificand() const;
    	float getFloat() const;
	const float& f() const;
	std::uint32_t* getFloatInt() const;
	// convert to an int
	// this converts to an int but the int is signed abs value so 
	// so magnitudes must be compared not with the sign
//	operator int() const;
	void operator++();
	void operator--();
	float nextFloatUp() const;
	float nextFloatDown() const;
	private:
	float vf;
	std::uint32_t* vp; 
	//std::uint32_t sig;
	//std::uint32_t exp;
	//std::uint32_t sgn;
};
inline float bitsToFloat(std::uint32_t);
std::ostream& operator<<(std::ostream& ostr, FloatParts fp);
void printOne(float a);
void testFloatParts();
bool testFloatPartsComparison();
bool operator<(const FloatParts a, const FloatParts b);
bool operator>(const FloatParts a, const FloatParts b);
bool operator<=(const FloatParts a, const FloatParts b);
bool operator>=(const FloatParts a, const FloatParts b);
bool operator!=(const FloatParts a, const FloatParts b);
bool operator== (const FloatParts a, const FloatParts b);
