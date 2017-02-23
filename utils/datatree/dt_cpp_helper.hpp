#ifndef __DT_CPP_HELPER_H__
#define __DT_CPP_HELPER_H__

#include <string>

#include "dt_core.h"
#include "dt_query.h"

// These C++ classes are some very simple wrapper of core APIs,
// some operations may be quite convenient in the C++ way

namespace dt {

class DT_API DatatreeParsingException {

public:
	DatatreeParsingException(const char* _msg);

	const char* msg;

};

class DT_API Value {

public:
	Value();
	~Value();

	Value(dt_datatree_t dt, dt_value_t val);
	Value(dt_datatree_t dt, const char* fmt, ...);
	Value(const Value &other);
	Value &operator = (const Value &other);

	bool operator == (const Value &other) const;
	bool operator != (const Value &other) const;
	bool operator < (const Value &other) const;

	bool equals(const dt_value_t val) const;
	bool equals(const Value &other) const;
	int compare(const dt_value_t val, dt_bool_t numRawCmp = DT_TRUE) const;
	int compare(const Value &other, dt_bool_t numRawCmp = DT_TRUE) const;

	bool isNull(void) const;
	void setNull(void);

	void cloneFrom(dt_datatree_t dt, dt_value_t val);
	void cloneFrom(const Value &other);

	void unsafeSwap(dt_value_t val);
	void unsafeMove(dt_value_t val);

	dt_datatree_t getDatatree(void);
	dt_value_t getValue(void);

	operator dt_value_t(void);

	std::string toString(dt_bool_t compact = DT_FALSE) const;
	void toString(std::string &buf, dt_bool_t compact = DT_FALSE) const;

	template<typename T>
	T as(void) const {
		DT_ASSERT(0 && "Impossible");
	}

private:
	void swap(Value &other);
	void release(void);
	void destroy(void);

private:
	dt_datatree_t mDatatree;
	dt_value_t mRep;
	unsigned int* mUseCount;

};

template<> inline dt_bool_t Value::as(void) const { dt_bool_t ret = DT_FALSE; dt_value_data_as(&ret, mRep, DT_BOOL); return ret; }
template<> inline bool Value::as(void) const { dt_bool_t ret = DT_FALSE; dt_value_data_as(&ret, mRep, DT_BOOL); return !!ret; }
template<> inline char Value::as(void) const { char ret = 0; dt_value_data_as(&ret, mRep, DT_BYTE); return ret; }
template<> inline unsigned char Value::as(void) const { unsigned char ret = 0; dt_value_data_as(&ret, mRep, DT_UBYTE); return ret; }
template<> inline short Value::as(void) const { short ret = 0; dt_value_data_as(&ret, mRep, DT_SHORT); return ret; }
template<> inline unsigned short Value::as(void) const { unsigned short ret = 0; dt_value_data_as(&ret, mRep, DT_USHORT); return ret; }
template<> inline int Value::as(void) const { int ret = 0; dt_value_data_as(&ret, mRep, DT_INT); return ret; }
template<> inline unsigned int Value::as(void) const { unsigned int ret = 0; dt_value_data_as(&ret, mRep, DT_UINT); return ret; }
template<> inline long Value::as(void) const { long ret = 0; dt_value_data_as(&ret, mRep, DT_LONG); return ret; }
template<> inline unsigned long Value::as(void) const { unsigned long ret = 0; dt_value_data_as(&ret, mRep, DT_ULONG); return ret; }
template<> inline float Value::as(void) const { float ret = 0; dt_value_data_as(&ret, mRep, DT_SINGLE); return ret; }
template<> inline double Value::as(void) const { double ret = 0; dt_value_data_as(&ret, mRep, DT_DOUBLE); return ret; }
template<> inline std::string Value::as(void) const { char* ret = NULL; dt_value_data_as(&ret, mRep, DT_STRING); return ret; }
template<> inline dt_object_t Value::as(void) const { dt_object_t ret = NULL; dt_value_data_as(&ret, mRep, DT_OBJECT); return ret; }
template<> inline dt_array_t Value::as(void) const { dt_array_t ret = NULL; dt_value_data_as(&ret, mRep, DT_ARRAY); return ret; }

class DT_API Command {

public:
	Command();
	Command(const char* fmt, ...);
	~Command();

	int parse(const char* fmt, ...);

	void clear(void);

	Value query(dt_value_t t);
	dt_query_status_t query(dt_value_t t, Value &ret);
	dt_query_status_t query(dt_value_t t, dt_value_t &ret);

private:
	Command(const Command &);
	Command &operator = (const Command &);

private:
	dt_command_t mCommand;

};

}

#endif // __DT_CPP_HELPER_H__
