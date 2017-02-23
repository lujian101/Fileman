#include "dt_cpp_helper.hpp"

#ifdef _MSC_VER
#	pragma warning(disable : 4996)
#endif /* _MSC_VER */

namespace dt {

static void _on_parse_error(dt_enum_compatible_t status, const char* msg, const char* pos, size_t row, size_t col) {
	printf(
		"Parsing error.\nError code: %d, error message: %s\nRow: %d, col: %d\nText: %s...\n",
		status,
		msg,
		row,
		col,
		pos
	);
}

DatatreeParsingException::DatatreeParsingException(const char* _msg) : msg(_msg) {
}

Value::Value() : mDatatree(NULL), mRep(NULL), mUseCount(NULL) {
}

Value::~Value() {
	release();
}

Value::Value(dt_datatree_t dt, dt_value_t val) : mDatatree(dt), mRep(val), mUseCount(new unsigned int(1)) {
}

Value::Value(const Value &other) {
	mDatatree = other.mDatatree;
	mRep = other.mRep;
	mUseCount = other.mUseCount;
	if(mUseCount)
		++(*mUseCount);
}

Value::Value(dt_datatree_t dt, const char* fmt, ...) : mUseCount(new unsigned int(1)) {
	mDatatree = dt;

	va_list argptr;
	va_start(argptr, fmt);
	if(fmt) {
		char buf[DT_STR_LEN];
		vsprintf(buf, fmt, argptr);
		dt_create_value(dt, &mRep, _on_parse_error, buf);
	} else {
		dt_type_t t = (dt_type_t)va_arg(argptr, dt_enum_compatible_t);
		switch(t) {
		case DT_NULL:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, DT_NULL);
			break;
		case DT_BOOL:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (dt_bool_t)va_arg(argptr, dt_enum_compatible_t));
			break;
		case DT_BYTE:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (char)(int)va_arg(argptr, int/*char*/));
			break;
		case DT_UBYTE:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (unsigned char)(int)va_arg(argptr, int/*unsigned char*/));
			break;
		case DT_SHORT:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (short)(int)va_arg(argptr, int/*short*/));
			break;
		case DT_USHORT:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (unsigned short)(int)va_arg(argptr, int/*unsigned short*/));
			break;
		case DT_INT:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (int)va_arg(argptr, int));
			break;
		case DT_UINT:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (unsigned int)va_arg(argptr, unsigned int));
			break;
		case DT_LONG:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (long)va_arg(argptr, long));
			break;
		case DT_ULONG:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (unsigned long)va_arg(argptr, unsigned long));
			break;
		case DT_SINGLE:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, (float)va_arg(argptr, double/*float*/));
			break;
		case DT_DOUBLE:
			dt_create_value(dt, &mRep, _on_parse_error, NULL, t, va_arg(argptr, double));
			break;
		case DT_STRING: {
				const char* str = va_arg(argptr, const char*);
				dt_create_value(dt, &mRep, _on_parse_error, NULL, t, str);
			}
			break;
		case DT_OBJECT:
			DT_ASSERT(0 && "Not supported");
			break;
		case DT_ARRAY:
			DT_ASSERT(0 && "Not supported");
			break;
		default:
			DT_ASSERT(0 && "Invalid type");
		}
	}
	va_end(argptr);
}

Value &Value::operator = (const Value &other) {
	if(mRep == other.mRep)
        return *this;
    Value tmp(other);
    swap(tmp);

    return *this;
}

bool Value::operator == (const Value &other) const {
	return mRep == other.mRep;
}

bool Value::operator != (const Value &other) const {
	return mRep != other.mRep;
}

bool Value::operator < (const Value &other) const {
	return mRep < other.mRep;
}

bool Value::equals(const dt_value_t val) const {
	return dt_value_compare(mRep, val, DT_TRUE) == 0;
}

bool Value::equals(const Value &other) const {
	return dt_value_compare(mRep, other.mRep, DT_TRUE) == 0;
}

int Value::compare(const dt_value_t val, dt_bool_t numRawCmp/* = DT_TRUE*/) const {
	return dt_value_compare(mRep, val, DT_TRUE);
}

int Value::compare(const Value &other, dt_bool_t numRawCmp/* = DT_TRUE*/) const {
	return dt_value_compare(mRep, other.mRep, DT_TRUE);
}

bool Value::isNull(void) const {
	return mRep == 0;
}

void Value::setNull(void) {
    if(mRep) {
        release();
        mRep = 0;
        mUseCount = 0;
    }
}

void Value::cloneFrom(dt_datatree_t dt, dt_value_t val) {
	release();
	mDatatree = dt;
	dt_create_value(mDatatree, &mRep, _on_parse_error, NULL, DT_NULL);
	dt_clone_value(mDatatree, val, mRep);
	mUseCount = new unsigned int(1);
}

void Value::cloneFrom(const Value &other) {
	release();
	cloneFrom(other.mDatatree, other.mRep);
}

void Value::unsafeSwap(dt_value_t val) {
	dt_value_mem_swap(mRep, val);
}

void Value::unsafeMove(dt_value_t val) {
	dt_value_mem_move(mRep, val);
}

dt_datatree_t Value::getDatatree(void) {
	return mDatatree;
}

dt_value_t Value::getValue(void) {
	return mRep;
}

Value::operator dt_value_t(void) {
	return mRep;
}

std::string Value::toString(dt_bool_t compact/* = DT_FALSE*/) const {
	std::string ret;
	toString(ret, compact);

	return ret;
}

void Value::toString(std::string &buf, dt_bool_t compact/* = DT_FALSE*/) const {
	if(!mRep) return;
	char* p = NULL;
	dt_format_value(mDatatree, mRep, &p, (compact ? DT_TRUE : DT_FALSE));
	buf = p;
	dt_free((void**)&p);
}

void Value::swap(Value &other) {
	std::swap(mDatatree, other.mDatatree);
	std::swap(mRep, other.mRep);
	std::swap(mUseCount, other.mUseCount);
}

void Value::release(void) {
	bool destroyThis = false; {
        if(mUseCount) {
            if(--(*mUseCount) == 0) {
                destroyThis = true;
            }
        }
    }
    if(destroyThis)
        destroy();
}

void Value::destroy(void) {
	dt_destroy_value(mDatatree, mRep);
	delete mUseCount;
}

Command::Command() {
	dt_create_command(&mCommand);
}

Command::Command(const char* fmt, ...) {
	char buf[DT_CMD_STR_LEN];
	va_list argptr;
	DT_ASSERT(fmt);
	va_start(argptr, fmt);
	vsprintf(buf, fmt, argptr);
	va_end(argptr);
	dt_create_command(&mCommand);
	dt_parse_command(mCommand, _on_parse_error, buf);
}

Command::~Command() {
	dt_destroy_command(mCommand);
}

int Command::parse(const char* fmt, ...) {
	char buf[DT_CMD_STR_LEN];
	va_list argptr;
	DT_ASSERT(fmt);
	va_start(argptr, fmt);
	vsprintf(buf, fmt, argptr);
	va_end(argptr);

	return dt_parse_command(mCommand, _on_parse_error, buf);
}

void Command::clear(void) {
	dt_clear_command(mCommand);
}

Value Command::query(dt_value_t t) {
	Value value;
	query(t, value);

	return value;
}

dt_query_status_t Command::query(dt_value_t t, Value &ret) {
	dt_value_t value = NULL;
	dt_query_status_t result = query(t, value);
	if(result == DTQ_GOT_REF)
		ret.cloneFrom(DT_DUMMY_DATATREE, value);
	else if(result == DTQ_GOT_NOREF)
		ret = Value(DT_DUMMY_DATATREE, value);

	return result;
}

dt_query_status_t Command::query(dt_value_t t, dt_value_t &ret) {
	dt_query_status_t result = dt_query(t, mCommand, &ret);

	return result;
}

}
