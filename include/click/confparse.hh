// -*- c-basic-offset: 4; related-file-name: "../../lib/confparse.cc" -*-
#ifndef CLICK_CONFPARSE_HH
#define CLICK_CONFPARSE_HH
#include <click/string.hh>
#include <click/vector.hh>
struct in_addr;
CLICK_DECLS
class ErrorHandler;
class StringAccum;
class Timestamp;
/** @cond never */
#ifndef CLICK_TOOL
class Element;
class Router;
class Handler;
class HandlerCall;
# define CP_VA_PARSE_ARGS_REST	Element*, ErrorHandler*, ...
# define CP_OPT_CONTEXT		, Element* context = 0
# define CP_CONTEXT		, Element* context
# define CP_PASS_CONTEXT	, context
#else
# define CP_VA_PARSE_ARGS_REST	ErrorHandler*, ...
# define CP_OPT_CONTEXT
# define CP_CONTEXT
# define CP_PASS_CONTEXT
#endif
#ifndef CLICK_COMPILING_CONFPARSE_CC
# define CLICK_CONFPARSE_DEPRECATED CLICK_DEPRECATED
#else
# define CLICK_CONFPARSE_DEPRECATED /* */
#endif
/** @endcond */

/** @file <click/confparse.hh>
 * @brief Configuration string parsing functions. */

const char* cp_skip_space(const char* begin, const char* end);
const char* cp_skip_comment_space(const char* begin, const char* end);
bool cp_eat_space(String &str);
inline bool cp_is_space(const String &str);
bool cp_is_word(const String &str);
bool cp_is_click_id(const String &str);

String cp_uncomment(const String &str);
String cp_unquote(const String &str);
const char* cp_process_backslash(const char* begin, const char* end, StringAccum &sa);
String cp_quote(const String &str, bool allow_newlines = false);

// argument lists <-> lists of arguments
void cp_argvec(const String&, Vector<String>&);
String cp_unargvec(const Vector<String>&);
void cp_spacevec(const String&, Vector<String>&);
String cp_unspacevec(const String* begin, const String* end);
inline String cp_unspacevec(const Vector<String>&);
String cp_pop_spacevec(String&);

enum CpErrors {
    CPE_OK = 0,
    CPE_FORMAT,
    CPE_NEGATIVE,
    CPE_OVERFLOW,
    CPE_INVALID,
    CPE_MEMORY,
    CPE_NOUNITS
};
extern int cp_errno;

// strings and words
bool cp_string(const String&, String*, String *rest = 0);
bool cp_word(const String&, String*, String *rest = 0);
bool cp_keyword(const String&, String*, String *rest = 0);

// numbers
bool cp_bool(const String&, bool*);

const char *cp_integer(const char* begin, const char* end, int base, int*);
bool cp_integer(const String&, int base, int*);
inline bool cp_integer(const String&, int*);
const char *cp_integer(const char* begin, const char* end, int base, unsigned int*);
inline const unsigned char *cp_integer(const unsigned char* begin, const unsigned char* end, int base, unsigned int*);
bool cp_integer(const String&, int base, unsigned int*);
inline bool cp_integer(const String&, unsigned int*);

#if SIZEOF_LONG == SIZEOF_INT
inline const char *cp_integer(const char* begin, const char* end, int base, long*);
inline bool cp_integer(const String&, int base, long*);
inline bool cp_integer(const String&, long*);
inline const char *cp_integer(const char* begin, const char* end, int base, unsigned long*);
inline bool cp_integer(const String&, int base, unsigned long*);
inline bool cp_integer(const String&, unsigned long*);
#elif SIZEOF_LONG != 8
# error "long has an odd size"
#endif

#ifdef HAVE_INT64_TYPES
const char *cp_integer(const char* begin, const char* end, int base, int64_t*);
bool cp_integer(const String&, int base, int64_t*);
inline bool cp_integer(const String&, int64_t*);
const char *cp_integer(const char* begin, const char* end, int base, uint64_t*);
inline const unsigned char *cp_integer(const unsigned char* begin, const unsigned char* end, int base, uint64_t*);
bool cp_integer(const String&, int base, uint64_t*);
inline bool cp_integer(const String&, uint64_t*);
#endif

#ifdef CLICK_USERLEVEL
bool cp_file_offset(const String&, off_t*);
#endif

#define CP_REAL2_MAX_FRAC_BITS 28
bool cp_real2(const String&, int frac_bits, int32_t*);
bool cp_real2(const String&, int frac_bits, uint32_t*);
bool cp_real10(const String&, int frac_digits, int32_t*);
bool cp_real10(const String&, int frac_digits, uint32_t*);
bool cp_real10(const String&, int frac_digits, uint32_t*, uint32_t*);
#ifdef HAVE_FLOAT_TYPES
bool cp_double(const String&, double*);
#endif

bool cp_seconds_as(int want_power, const String&, uint32_t*);
bool cp_seconds_as_milli(const String&, uint32_t*);
bool cp_seconds_as_micro(const String&, uint32_t*);
bool cp_time(const String&, struct timeval*);
bool cp_time(const String&, Timestamp*);

bool cp_bandwidth(const String&, uint32_t*);

String cp_unparse_bool(bool);
String cp_unparse_real2(int32_t, int frac_bits);
String cp_unparse_real2(uint32_t, int frac_bits);
#ifdef HAVE_INT64_TYPES
String cp_unparse_real2(int64_t, int frac_bits);
String cp_unparse_real2(uint64_t, int frac_bits);
#endif
String cp_unparse_real10(int32_t, int frac_digits);
String cp_unparse_real10(uint32_t, int frac_digits);
String cp_unparse_milliseconds(uint32_t);
String cp_unparse_microseconds(uint32_t);
String cp_unparse_interval(const struct timeval&);
String cp_unparse_interval(const Timestamp&);
String cp_unparse_bandwidth(uint32_t);

// network addresses
class IPAddress;
class IPAddressList;
bool cp_ip_address(const String&, unsigned char*  CP_OPT_CONTEXT);
bool cp_ip_address(const String&, IPAddress*  CP_OPT_CONTEXT);
inline bool cp_ip_address(const String&, struct in_addr*  CP_OPT_CONTEXT);
bool cp_ip_prefix(const String&, unsigned char*, unsigned char*, bool allow_bare_address  CP_OPT_CONTEXT);
bool cp_ip_prefix(const String&, IPAddress*, IPAddress*, bool allow_bare_address  CP_OPT_CONTEXT);
bool cp_ip_prefix(const String&, unsigned char*, unsigned char*  CP_OPT_CONTEXT);
bool cp_ip_prefix(const String&, IPAddress*, IPAddress*  CP_OPT_CONTEXT);
bool cp_ip_address_list(const String&, Vector<IPAddress>*  CP_OPT_CONTEXT);

#ifdef HAVE_IP6
class IP6Address;
bool cp_ip6_address(const String&, unsigned char*  CP_OPT_CONTEXT);
bool cp_ip6_address(const String&, IP6Address*  CP_OPT_CONTEXT);
bool cp_ip6_prefix(const String&, unsigned char*, int*, bool allow_bare_address  CP_OPT_CONTEXT);
bool cp_ip6_prefix(const String&, IP6Address*, int*, bool allow_bare_address  CP_OPT_CONTEXT);
bool cp_ip6_prefix(const String&, unsigned char*, unsigned char*, bool allow_bare_address  CP_OPT_CONTEXT);
bool cp_ip6_prefix(const String&, IP6Address*, IP6Address*, bool allow_bare_address  CP_OPT_CONTEXT);
#endif

class EtherAddress;
bool cp_ethernet_address(const String&, unsigned char*  CP_OPT_CONTEXT);
bool cp_ethernet_address(const String&, EtherAddress*  CP_OPT_CONTEXT);

bool cp_tcpudp_port(const String&, int ip_p, uint16_t*  CP_OPT_CONTEXT);

#ifndef CLICK_TOOL
Element *cp_element(const String&, Element*, ErrorHandler*);
Element *cp_element(const String&, Router*, ErrorHandler*);
bool cp_handler_name(const String&, Element**, String*, Element*, ErrorHandler*);
bool cp_handler(const String&, int flags, Element**, const Handler**, Element*, ErrorHandler*);
#endif

#ifdef HAVE_IPSEC
bool cp_des_cblock(const String&, unsigned char*);
#endif

#ifndef CLICK_LINUXMODULE
bool cp_filename(const String&, String*);
#endif

typedef const char *CpVaParseCmd;
extern const CpVaParseCmd
    cpEnd,
    cpOptional,
    cpKeywords,
    cpConfirmKeywords,
    cpMandatoryKeywords,
    cpIgnore,
    cpIgnoreRest,
			// Helpers		Results
    cpArgument,		//			String*
    cpArguments,	//			Vector<String>*
    cpString,		//			String*
    cpWord,		//			String*
    cpKeyword,		//			String*
    cpBool,		//			bool*
    cpByte,		//			unsigned char*
    cpShort,		//			short*
    cpUnsignedShort,	//			unsigned short*
    cpInteger,		//			int*
    cpUnsigned,		//			unsigned*
    cpNamedInteger,	// uint32_t type	int32_t*
#ifdef HAVE_INT64_TYPES
    cpInteger64,	//			int64_t*
    cpUnsigned64,	//			uint64_t*
#endif
#ifdef CLICK_USERLEVEL
    cpFileOffset,	// 			off_t*
#endif
    cpUnsignedReal2,	// int frac_bits	unsigned*
    cpReal10,		// int frac_digits	int*
    cpUnsignedReal10,	// int frac_digits	unsigned*
#ifdef HAVE_FLOAT_TYPES
    cpDouble,		//			double*
#endif
    cpSeconds,		//			uint32_t*
    cpSecondsAsMilli,	//			uint32_t* milliseconds
    cpSecondsAsMicro,	//			uint32_t* microseconds
    cpTimeval,		//			timeval*
    cpTimestamp,	//			Timestamp*
    cpInterval,		//			timeval*
    cpBandwidth,	//			uint32_t* bandwidth (in B/s)
    cpIPAddress,	//			IPAddress*
    cpIPPrefix,		//			IPAddress* a, IPAddress* mask
    cpIPAddressOrPrefix,//			IPAddress* a, IPAddress* mask
    cpIPAddressList,	//			IPAddressList*
    cpEthernetAddress,	//			EtherAddress*
    cpTCPPort,		// 			uint16_t*
    cpUDPPort,		// 			uint16_t*
    cpElement,		//			Element**
    cpHandlerName,	//			Element**, String* hname
    cpHandlerCallRead,	//			HandlerCall*
    cpHandlerCallWrite,	//			HandlerCall*
    cpHandlerCallPtrRead, //			HandlerCall**
    cpHandlerCallPtrWrite, //			HandlerCall**
    cpIP6Address,	//			IP6Address*
    cpIP6Prefix,	//			IP6Address* a, IP6Address* mask
    cpIP6AddressOrPrefix,//			IP6Address* a, IP6Address* mask
    cpDesCblock,	//			uint8_t[8]
    cpFilename,		//			String*
    // old names, here for compatibility:
    cpEtherAddress,	//			EtherAddress*
    cpReadHandlerCall,	//			HandlerCall**
    cpWriteHandlerCall;	//			HandlerCall**

enum CpKparseFlags {
    cpkNormal = 0, cpkN = 0,
    cpkMandatory = 1, cpkM = 1,
    cpkPositional = 2, cpkP = 2,
    cpkConfirm = 4, cpkC = 4
};

int cp_va_kparse(const Vector<String>& argv, CP_VA_PARSE_ARGS_REST);
int cp_va_kparse(const String& arg, CP_VA_PARSE_ARGS_REST);
int cp_va_space_kparse(const String& arg, CP_VA_PARSE_ARGS_REST);
int cp_va_kparse_keyword(const String& arg, CP_VA_PARSE_ARGS_REST);
int cp_va_kparse_remove_keywords(Vector<String>& argv, CP_VA_PARSE_ARGS_REST);
// Argument syntax:
// cp_va_arg ::= cpEnd		// terminates argument list (not 0!)
//    |   cpIgnoreRest		// terminates argument list
//    |   const char *keyword,
//	  int flags (0 or more of cpkMandatory|cpkPositional),
//	  CpVaParseCmd cmd,
//	  [Optional Helpers], Results
//				// Helpers and Results depend on 'cmd';
//				// see table above
//    |   const char *keyword,
//	  int flags (cpkConfirm + 0 or more of cpkMandatory|cpkPositional),
//	  CpVaParseCmd cmd,
//	  bool *keyword_given,
//	  [Optional Helpers], Results
//				// Helpers and Results depend on 'cmd';
//				// see table above
// Returns the number of result arguments set, or negative on error.
// Stores no values in the result arguments on error.

int cp_va_parse(const Vector<String>& argv, CP_VA_PARSE_ARGS_REST);
int cp_va_parse(const String& arg, CP_VA_PARSE_ARGS_REST);
int cp_va_space_parse(const String& arg, CP_VA_PARSE_ARGS_REST);
int cp_va_parse_keyword(const String& arg, CP_VA_PARSE_ARGS_REST);
int cp_va_parse_remove_keywords(Vector<String>& argv, int first, CP_VA_PARSE_ARGS_REST);
// Argument syntax:
// cp_va_arg ::= cpEnd		// terminates argument list (not 0!)
//    |   cpOptional | cpKeywords | cpIgnore...		// manipulators
//    |   CpVaParseCmd cmd, const char *description,
//	  [Optional Helpers], Results
//				// Helpers and Results depend on 'cmd';
//				// see table above
//    |   const char *keyword, CpVaParseCmd cmd, const char *description,
//	  [Optional Helpers], Results
//				// keyword argument, within cpKeywords or
//				// cpMandatoryKeywords
//    |   const char *keyword, CpVaParseCmd cmd, const char *description,
//	  bool *keyword_given, [Optional Helpers], Results
//				// keyword argument, within cpConfirmKeywords
// Returns the number of result arguments set, or negative on error.
// Stores no values in the result arguments on error.

int cp_assign_arguments(const Vector<String>& argv, const String *keys_begin, const String *keys_end, Vector<String>* values = 0);

void cp_va_static_initialize();
void cp_va_static_cleanup();

// adding and removing types suitable for cp_va_parse and friends
struct cp_value;
struct cp_argtype;

typedef void (*cp_parsefunc)(cp_value*, const String& arg,
			     ErrorHandler*, const char* argdesc  CP_CONTEXT);
typedef void (*cp_storefunc)(cp_value*  CP_CONTEXT);

enum { cpArgNormal = 0, cpArgStore2 = 1, cpArgExtraInt = 2, cpArgAllowNumbers = 4 };
int cp_register_argtype(const char* name, const char* description,
			int flags, cp_parsefunc, cp_storefunc, void* user_data = 0);
void cp_unregister_argtype(const char* name);

int cp_register_stringlist_argtype(const char* name, const char* description,
				   int flags);
int cp_extend_stringlist_argtype(const char* name, ...);
// Takes: const char* name, int value, ...., const char* ender = 0

struct cp_argtype {
    const char* name;
    cp_argtype* next;
    cp_parsefunc parse;
    cp_storefunc store;
    void* user_data;
    int flags;
    const char* description;
    int internal;
    int use_count;
};

struct cp_value {
    // set by cp_va_parse:
    const cp_argtype* argtype;
    const char* keyword;
    const char* description CLICK_CONFPARSE_DEPRECATED;
    int extra;
    void* store;
    void* store2;
    bool* store_confirm;
    // set by parsefunc, used by storefunc:
    union {
	bool b;
	int32_t i;
	uint32_t u;
#ifdef HAVE_INT64_TYPES
	int64_t i64;
	uint64_t u64;
#endif
#ifdef HAVE_FLOAT_TYPES
	double d;
#endif
	unsigned char address[16];
	int is[4];
#ifndef CLICK_TOOL
	Element* element;
#endif
    } v, v2;
    String v_string;
    String v2_string;
};

/** @cond never */
#define cp_integer64 cp_integer
#define cp_unsigned64 cp_integer
#define cp_unsigned cp_integer
#define cp_unsigned_real10 cp_real10
#define cp_unsigned_real2 cp_real2
/** @endcond */

inline String cp_unspacevec(const Vector<String>& conf)
{
    return cp_unspacevec(conf.begin(), conf.end());
}

/** @brief  Test if @a str is all spaces.
 *  @return True when every character in @a str is a space. */
inline bool cp_is_space(const String& str)
{
    return cp_skip_space(str.begin(), str.end()) == str.end();
}

inline const unsigned char *cp_integer(const unsigned char *begin, const unsigned char *end, int base, unsigned int* return_value)
{
    return (const unsigned char *) cp_integer((const char *) begin, (const char *) end, base, return_value);
}

inline bool cp_integer(const String& str, unsigned int* return_value)
{
    return cp_integer(str, 0, return_value);
}

inline bool cp_integer(const String& str, int* return_value)
{
    return cp_integer(str, 0, return_value);
}

#ifdef HAVE_INT64_TYPES
inline const unsigned char *cp_integer(const unsigned char *begin, const unsigned char *end, int base, uint64_t* return_value)
{
    return (const unsigned char *) cp_integer((const char *) begin, (const char *) end, base, return_value);
}

inline bool cp_integer(const String& str, uint64_t* return_value)
{
    return cp_integer(str, 0, return_value);
}

inline bool cp_integer(const String& str, int64_t* return_value)
{
    return cp_integer(str, 0, return_value);
}
#endif

#if SIZEOF_LONG == SIZEOF_INT
inline const char* cp_integer(const char* begin, const char* end, int base, long* return_value)
{
    return cp_integer(begin, end, base, reinterpret_cast<int*>(return_value));
}

inline bool cp_integer(const String& str, int base, long* return_value)
{
    return cp_integer(str, base, reinterpret_cast<int*>(return_value));
}

inline bool cp_integer(const String& str, long* return_value)
{
    return cp_integer(str, reinterpret_cast<int*>(return_value));
}

inline const char* cp_integer(const char* begin, const char* end, int base, unsigned long* return_value)
{
    return cp_integer(begin, end, base, reinterpret_cast<unsigned int*>(return_value));
}

inline bool cp_integer(const String& str, int base, unsigned long* return_value)
{
    return cp_integer(str, base, reinterpret_cast<unsigned int*>(return_value));
}

inline bool cp_integer(const String& str, unsigned long* return_value)
{
    return cp_integer(str, reinterpret_cast<unsigned int*>(return_value));
}
#endif

inline bool cp_ip_address(const String& str, struct in_addr* ina  CP_CONTEXT)
{
    return cp_ip_address(str, reinterpret_cast<IPAddress*>(ina)  CP_PASS_CONTEXT);
}

#undef CP_VA_ARGS_REST
#undef CP_OPT_CONTEXT
#undef CP_CONTEXT
#undef CP_PASS_CONTEXT
#undef CLICK_CONFPARSE_DEPRECATED
CLICK_ENDDECLS
#endif
