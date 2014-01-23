#ifndef SPEEX_HELPER_BY_KOLONSE
#define SPEEX_HELPER_BY_KOLONSE
#include <string>
#include "boost/function.hpp"
#include "boost/bind.hpp"
using namespace std ;
/**
*	depend: speex	
*	
*/
class speex_helper
{
public:
	enum oper_type
	{
		pcm2speex,
		speex2pcm,
	};
	speex_helper( oper_type type );
	~speex_helper();

	inline void oper_one( const string& src, string& dst )
	{
		_OPER_FUN( src, dst );
	}

	inline void oper_one( const char* src_ptr, int src_len, string& dst )
	{
		_OPER_FUN( string( src_ptr, src_len) , dst );
	}
private:
	oper_type _type ;
	/*
	*		接口函数
	*/
	typedef boost::function< void ( const string&, string& ) > OPER_FUN;
	OPER_FUN _OPER_FUN ;

	/*
	*		操作接口
	*/
	void* oper_int ;
};

#endif
