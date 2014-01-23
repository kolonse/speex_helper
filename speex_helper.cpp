#include "speex_helper.h"
#include <speex.h>

//SPEEX_SET_QUALITY��0     SPEEX_GET_BITRATE��2150        SPEEX_OUT_SIZE_PER160��6
//SPEEX_SET_QUALITY��1     SPEEX_GET_BITRATE��3950        SPEEX_OUT_SIZE_PER160��10
//SPEEX_SET_QUALITY��2     SPEEX_GET_BITRATE��5950        SPEEX_OUT_SIZE_PER160��15
//SPEEX_SET_QUALITY��3     SPEEX_GET_BITRATE��8000        SPEEX_OUT_SIZE_PER160��20
//SPEEX_SET_QUALITY��4     SPEEX_GET_BITRATE��8000        SPEEX_OUT_SIZE_PER160��20
//SPEEX_SET_QUALITY��5     SPEEX_GET_BITRATE��11000       SPEEX_OUT_SIZE_PER160��28
//SPEEX_SET_QUALITY��6     SPEEX_GET_BITRATE��11000       SPEEX_OUT_SIZE_PER160��28
//SPEEX_SET_QUALITY��7     SPEEX_GET_BITRATE��15000       SPEEX_OUT_SIZE_PER160��38
//SPEEX_SET_QUALITY��8     SPEEX_GET_BITRATE��15000       SPEEX_OUT_SIZE_PER160��38
//SPEEX_SET_QUALITY��9     SPEEX_GET_BITRATE��18200       SPEEX_OUT_SIZE_PER160��46
//SPEEX_SET_QUALITY��10    SPEEX_GET_BITRATE��24600       SPEEX_OUT_SIZE_PER160��62

#define QUALITY_0_SPEEX_OUT_SIZE_PER160 6
#define QUALITY_1_SPEEX_OUT_SIZE_PER160 10
#define QUALITY_2_SPEEX_OUT_SIZE_PER160 15
#define QUALITY_3_SPEEX_OUT_SIZE_PER160 20
#define QUALITY_4_SPEEX_OUT_SIZE_PER160 20
#define QUALITY_5_SPEEX_OUT_SIZE_PER160 28
#define QUALITY_6_SPEEX_OUT_SIZE_PER160 28
#define QUALITY_7_SPEEX_OUT_SIZE_PER160 38
#define QUALITY_8_SPEEX_OUT_SIZE_PER160 38
#define QUALITY_9_SPEEX_OUT_SIZE_PER160 46
#define QUALITY_10_SPEEX_OUT_SIZE_PER160 62

#define MAX_FRAME_BYTES 2000 
#define QUALITY 8

#define DEC_BLOCK_SIZE(quality,frame) QUALITY_##quality##_SPEEX_OUT_SIZE_PER##frame

inline int dec_block_size( int quality, int frame )
{
	switch( quality )
	{
	case 8:
		{
			switch( frame )
			{
			case 160:
				return DEC_BLOCK_SIZE( 8, 160 );
			}
		}
		break ;
	}

	return 0;
}

class pcm2speex_int
{
public:
	pcm2speex_int()
	{
		state = speex_encoder_init(&speex_nb_mode); 
		int quality = QUALITY ; 
		speex_encoder_ctl(state, SPEEX_SET_QUALITY, &quality); 
		speex_bits_init(&bits);

		speex_encoder_ctl(state, SPEEX_GET_FRAME_SIZE, &frame_size);
		in  = new short[ frame_size ];
	}

	~pcm2speex_int()
	{
		speex_encoder_destroy(state); 
		speex_bits_destroy(&bits); 
		delete[] in ;
	}

	void oper_one( const string& src, string& dst )
	{
		dst = "" ;
		int src_len = src.size();
		int block_size = sizeof(short) * frame_size ;
		if( src_len % sizeof(short) != 0 && src_len % block_size != 0 )
		{	
			return ;
		}

		int times = src_len / block_size ;
		const char* buff = src.data() ;
		for( int i = 0 ; i < times; ++ i )
		{
			speex_bits_reset( &bits );
			memcpy( (void*)in,buff + i * block_size, block_size );
			speex_encode_int(state, in, &bits);
			int nbBytes = speex_bits_write(&bits, cbits, MAX_FRAME_BYTES);
			dst.append( cbits, nbBytes );
		}
	}

private:
	SpeexBits bits; 
	void *state; 
	int frame_size ;
	short* in; 
	char cbits[ MAX_FRAME_BYTES ];
};

class speex2pcm_int
{
public:
	speex2pcm_int()
	{
		state = speex_decoder_init(&speex_nb_mode);
		int tmp=1;
		speex_decoder_ctl(state, SPEEX_SET_ENH, &tmp);
		speex_bits_init(&bits);
		speex_encoder_ctl(state, SPEEX_GET_FRAME_SIZE, &frame_size);

		out  = new short[ frame_size ];
	}

	~speex2pcm_int()
	{
		speex_decoder_destroy(state); 
		speex_bits_destroy(&bits); 
		delete[] out ;
	}

	void oper_one( const string& src, string& dst )
	{
		int block_size = dec_block_size( QUALITY, frame_size );
		int src_len = src.size();
		if( src_len % block_size != 0 )
		{	
			return ;
		}

		int times = src_len / block_size ;
		const char* buff = src.data() ;
		for( int i = 0 ; i < times; ++ i )
		{
			speex_bits_read_from(&bits, (char*)(buff + i * block_size), block_size );
			speex_decode_int(state, &bits, out);

			dst.append( (char*)out, frame_size * sizeof(short) );
		}
	}
private:
	SpeexBits bits; 
	void *state; 
	int frame_size ;
	short* out; 
	char cbits[ MAX_FRAME_BYTES ];
};

speex_helper::speex_helper( oper_type type )
	: _type( type )
{
	switch( _type )
	{
	case pcm2speex:
		{
			oper_int = (void*) new pcm2speex_int;
			if( NULL == oper_int )
			{
				std::exception("memory leak");
			}
			_OPER_FUN = boost::bind( &pcm2speex_int::oper_one,( pcm2speex_int *)oper_int , _1, _2 );
		}

		break;
	case speex2pcm:
		{
			oper_int = (void*) new speex2pcm_int;
			if( NULL == oper_int )
			{
				std::exception("memory leak");
			}
			_OPER_FUN = boost::bind( &speex2pcm_int::oper_one,( speex2pcm_int *)oper_int, _1, _2 );
		}

		break ;
	}
}

speex_helper::~speex_helper()
{
	switch( _type )
	{
	case pcm2speex:
		if( NULL != oper_int )
		{
			delete ( pcm2speex_int *)oper_int;
		}
		break;
	case speex2pcm:
		if( NULL != oper_int )
		{
			delete ( speex2pcm_int *)oper_int;
		}
		break ;
	}	
}


